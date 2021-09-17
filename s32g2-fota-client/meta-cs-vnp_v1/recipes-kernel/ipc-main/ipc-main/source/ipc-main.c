/*
 * Copyright 2019-2021 NXP
 *
 * SPDX-License-Identifier:  GPL-2.0
*/
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */
#include <linux/io.h>	
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include "ipc.h"

#define DT_IPC_MAIN_NODE_COMP "fsl,s32xx-ipc-main"

#define IPC_SHM_OFFSET 0x1000

struct pool_info {
	uint32_t buffer_num;
	uint32_t buffer_size;
};

#define IPC_INFO_MAGIC 0x4950432E
#define IPC_VERSION 1
#define IPC_STATE_WORKING 0x12345678
#define IPC_STATE_SUSPENDING 0x55555555

struct ipc_info_table {
	uint32_t magic;
	uint32_t version;
	uint32_t state;
	uint32_t reserved[4];
	uint32_t pool_num;
	struct pool_info  pools[0];
};

struct ipc_channel_info {
	ipc_rx_func_t func;
	void *arg;
	uint32_t rx_cnt;
	uint32_t tx_cnt;
};

/*
 * channel 0 for virtual network interface 
 * channle 1 for test
 */
#define MAX_CHANNEL_NUM 16
#define MAX_POOL_NUM 128

enum IPC_STATE {
	IPC_CHECK_MAGIC,
	IPC_CHECK_VERSION,
	IPC_CHECK_POOL,
	IPC_READY
};

static enum IPC_STATE g_ipc_state;
static uint32_t g_ipc_channel_num;
static uint32_t g_ipc_pool_num;
static struct ipc_shm_pool_cfg *g_ipc_pools;
static struct ipc_shm_channel_cfg *g_ipc_channels;
static struct ipc_channel_info *g_ipc_channel_priv;
static void *g_local_iomem;
static void *g_remote_iomem;

int ipc_reg_rx_callback(uint32_t ch, ipc_rx_func_t func, void *arg)
{
	if (ch >= g_ipc_channel_num)
		return -1;
	
	g_ipc_channel_priv[ch].func = func;
	g_ipc_channel_priv[ch].arg = arg;
 	return 0;
}
EXPORT_SYMBOL(ipc_reg_rx_callback);

bool ipc_ready(void)
{
	struct ipc_info_table *local, *remote;
	int i;
	
	if (g_ipc_state == IPC_READY)
		return true;

	local = g_local_iomem;
	remote = g_remote_iomem;
	
	if (remote->magic != local->magic) {
		g_ipc_state = IPC_CHECK_MAGIC;
		return false;
	}
	
	if (remote->version != local->version) {
		g_ipc_state = IPC_CHECK_VERSION;
		return false;
	}

	if (remote->pool_num != local->pool_num) {
		g_ipc_state = IPC_CHECK_POOL;
		return false;
	}

	if (memcmp(remote->pools, local->pools, local->pool_num * sizeof(local->pools[0])) != 0) {
		g_ipc_state = IPC_CHECK_POOL;
		return false;
	}
	
	g_ipc_state = IPC_READY;
	return true;
}
EXPORT_SYMBOL(ipc_ready);

static void ipc_channel_rx_cb(void *cb_arg, int chan_id, void *buf, size_t size)
{
	ipc_rx_func_t func;
	
	if (chan_id >= g_ipc_channel_num)
		return;
	
	g_ipc_channel_priv[chan_id].rx_cnt++;
	if ((func = g_ipc_channel_priv[chan_id].func))
		func(g_ipc_channel_priv[chan_id].arg, chan_id, buf, size);
	
	ipc_shm_release_buf(chan_id, buf);
}

static void init_channel(struct ipc_shm_channel_cfg *cfg, struct ipc_shm_pool_cfg *pool)
{
	cfg->type = IPC_SHM_MANAGED;
	cfg->ch.managed.rx_cb = ipc_channel_rx_cb;
	cfg->ch.managed.num_pools = 0;
	cfg->ch.managed.pools = pool;
}

static void ipc_set_info_table(unsigned long addr, const struct ipc_shm_cfg * cfg)
{
	struct ipc_info_table *info = (void*)addr;
	int ch, pool;
	
	memset(info, 0, sizeof(*info));
	info->version = 1;

	for (pool = 0, ch = 0; ch < cfg->num_channels; ch++) {
		int i;
		
		for (i = 0; i < cfg->channels[ch].ch.managed.num_pools; i++) {
			info->pools[pool].buffer_num = cfg->channels[ch].ch.managed.pools[i].num_bufs;
			info->pools[pool].buffer_size = cfg->channels[ch].ch.managed.pools[i].buf_size;
			pool++;
		}
		info->pools[pool].buffer_num = 0;
		info->pools[pool].buffer_size = 0;
		pool++;
	}
	info->pool_num = pool;
	info->state = IPC_STATE_SUSPENDING;
	info->magic = IPC_INFO_MAGIC;
}

static int __init ipc_init(struct device_node *node)
{
	int ret;
	uint32_t local_mem;
	uint32_t remote_mem;
	uint32_t shm_size;
	uint32_t tx_irq, rx_irq;
	struct property *prop;
	struct ipc_shm_cfg ipc_cfg;
	
	if (of_property_read_u32(node, "local-mem", &local_mem)) {
		pr_err("ipc-main: local-mem");
		return -ENODEV;
	}
	pr_info("ipc-main: local-mem: 0x%x", local_mem);
	if (of_property_read_u32(node, "remote-mem", &remote_mem)) {
		pr_err("ipc-main: remote-mem");
		return -ENODEV;
	}
	pr_info("ipc-main: remote-mem: 0x%x", remote_mem);
	if (of_property_read_u32(node, "shm-size", &shm_size)) {
		pr_err("ipc-main: shm-size");
		return -ENODEV;
	}
	pr_info("ipc-main: shm-size: 0x%x", shm_size);
	if (of_property_read_u32(node, "tx-irq", &tx_irq)) {
		pr_err("ipc-main: tx-irq");
		return -ENODEV;
	}
	pr_info("ipc-main: tx-irq: 0x%x", tx_irq);
	if (of_property_read_u32(node, "rx-irq", &rx_irq)) {
		pr_err("ipc-main: rx-irq");
		return -ENODEV;
	}
	pr_info("ipc-main: rx-irq: 0x%x", rx_irq);
	prop = of_find_property(node, "channels", NULL);
	if (prop) {
		const __be32 *cur;
		u32 val;
		uint32_t i;
		uint32_t ch;
		uint32_t ch_pool_num;
		
		g_ipc_pool_num = prop->length / (sizeof(u32)*2);
		if (g_ipc_pool_num < 2)
			return -EINVAL;
		
		g_ipc_pools = kmalloc(sizeof(*g_ipc_pools) * g_ipc_pool_num, GFP_KERNEL);
		g_ipc_channels = kmalloc(sizeof(*g_ipc_channels) * g_ipc_pool_num, GFP_KERNEL);
		g_ipc_channel_priv = kzalloc(sizeof(*g_ipc_channel_priv) * g_ipc_pool_num, GFP_KERNEL);
		
		cur = NULL;
		ch = 0;
		ch_pool_num = 0;
		for (i = 0; i < g_ipc_pool_num; i++) {
			cur = of_prop_next_u32(prop, cur, &val);
			g_ipc_pools[i].num_bufs = (uint16_t)val;
			cur = of_prop_next_u32(prop, cur, &val);
			g_ipc_pools[i].buf_size = val;
			/*
			pr_info("ipc-main: pool%d: 0x%x - 0x%x", i, g_ipc_pools[i].num_bufs, g_ipc_pools[i].buf_size);
			*/
			if (g_ipc_pools[i].num_bufs == 0) {
				/* invalid buffer number,  end a channel */
			
				if (ch_pool_num == 0)
					break; /* end all channel */
				
				g_ipc_channels[ch].ch.managed.num_pools = ch_pool_num;
				g_ipc_channel_num++;
				
				/* start new channel */
				ch_pool_num = 0;
				ch++;
			} else {
				if (ch_pool_num == 0) { /* init new channel */
					init_channel(&g_ipc_channels[ch], &g_ipc_pools[i]);
				}
				ch_pool_num++;
			}
		}
		pr_info("ipc-main: g_ipc_pools=%lx, channel-num: %u", (long)g_ipc_pools, g_ipc_channel_num);
		for (ch = 0; ch < g_ipc_channel_num; ch++)
			pr_info("ipc-main: ch%d, pool=%lx, num=%d", ch, 
			(long)g_ipc_channels[ch].ch.managed.pools, g_ipc_channels[ch].ch.managed.num_pools);
	}
	
	if (g_ipc_channel_num == 0) {
		pr_err("ipc-main: no valid channel configuration");
		ret = -EINVAL;
		goto SAFE_EXIT;
	}
	
	ipc_cfg.local_shm_addr = local_mem + IPC_SHM_OFFSET;
	ipc_cfg.remote_shm_addr = remote_mem + IPC_SHM_OFFSET;
	ipc_cfg.shm_size = shm_size - IPC_SHM_OFFSET;
	ipc_cfg.inter_core_tx_irq = tx_irq;
	ipc_cfg.inter_core_rx_irq = rx_irq;
	ipc_cfg.remote_core.type = IPC_CORE_M7;
	ipc_cfg.remote_core.index = 0;
	ipc_cfg.num_channels = g_ipc_channel_num;
	ipc_cfg.channels = g_ipc_channels;
	ret = ipc_shm_init(&ipc_cfg);
	if (ret == 0) {
		g_local_iomem  = ioremap_nocache(local_mem, IPC_SHM_OFFSET);
		g_remote_iomem  = ioremap_nocache(remote_mem, IPC_SHM_OFFSET);
		if (!g_local_iomem || !g_remote_iomem) {
			ret = -ENOMEM;
			goto SAFE_EXIT;
		}
		ipc_set_info_table((unsigned long)g_local_iomem, &ipc_cfg);
		pr_info("ipc_main: ipc_init ok\n");
	}
SAFE_EXIT:
	if (ret) {
		kfree(g_ipc_pools);
		kfree(g_ipc_channels);
		kfree(g_ipc_channel_priv);
		if (g_local_iomem)
			iounmap(g_local_iomem);
		if (g_remote_iomem)
			iounmap(g_remote_iomem);
		pr_info("ipc_main: ipc_init fail\n");
	}
	return ret;
}

#ifdef CONFIG_PROC_FS

static struct proc_dir_entry *g_ipc_dir;

static int ipc_proc_show(struct seq_file *m, void *v) 
{
	int ch = (int)(long)m->private;

	if (ch >= g_ipc_channel_num)
		return 0;
	seq_printf(m, "ch%d: rx: 0x%x, tx: 0x%x\n", ch, g_ipc_channel_priv[ch].rx_cnt, g_ipc_channel_priv[ch].tx_cnt);
	return 0;
}

static ssize_t ipc_proc_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *pos)
{
	int ch = (int)(long)PDE_DATA(file_inode(file));
	void *buf;

	if (!ipc_ready())
		return -EIO;
	
	if ((buf = ipc_shm_acquire_buf(ch, count))) {
		copy_from_user(buf, buffer, count);
		if (ipc_shm_tx(ch, buf, count)) {
			ipc_shm_release_buf(ch, buf);
			return -EBUSY;
		}
		g_ipc_channel_priv[ch].tx_cnt++;
		return count;
	}
	return -ENOMEM;
}

static int ipc_proc_open(struct inode *inode, struct file *file) 
{
	return single_open(file, ipc_proc_show, PDE_DATA(inode));
}

static const struct file_operations ipc_proc_fops = {
	.owner = THIS_MODULE,
	.open = ipc_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = ipc_proc_write,
};
#endif

static int __init ipc_main_module_init(void)
{
	struct device_node *node;
	int ret;
	
	printk("%s\n", __FUNCTION__);
	node = of_find_compatible_node(NULL, NULL, DT_IPC_MAIN_NODE_COMP);
	if (!node) {
		pr_err("ipc-main: can not find node compatible with " DT_IPC_MAIN_NODE_COMP);
		return -ENODEV;
	}
	ret = ipc_init(node);
	of_node_put(node);
	
#ifdef CONFIG_PROC_FS
	if (ret == 0) {
		int i;
		
		g_ipc_dir = proc_mkdir("s32xx-ipc", NULL);
		if (!g_ipc_dir)
			return 0;
		
		for (i = 0; i < g_ipc_channel_num; i++) {
			char filename[16];

			snprintf(filename, sizeof(filename), "%d", i);
			proc_create_data(filename, 0600, g_ipc_dir, &ipc_proc_fops, (void*)(long)i);
		}
	}
#endif
	return ret;
}

static void __exit ipc_main_module_exit(void)
{
	printk("%s\n", __FUNCTION__);
	
#ifdef CONFIG_PROC_FS
	if (g_ipc_dir)
		proc_remove(g_ipc_dir);
#endif

	ipc_shm_free();

	kfree(g_ipc_pools);
	kfree(g_ipc_channels);
	kfree(g_ipc_channel_priv);
	if (g_local_iomem)
		iounmap(g_local_iomem);
	if (g_remote_iomem)
		iounmap(g_remote_iomem);
}

module_init(ipc_main_module_init);
module_exit(ipc_main_module_exit);

MODULE_AUTHOR("NXP Ltd");
MODULE_DESCRIPTION("ipcf initialization");
MODULE_LICENSE("GPL");

