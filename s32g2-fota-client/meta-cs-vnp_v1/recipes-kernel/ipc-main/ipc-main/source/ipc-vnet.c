/*
 * Copyright 2019-2021 NXP
 *
 * SPDX-License-Identifier:  GPL-2.0
*/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>

#include "ipc.h"


struct ipc_vnet {
	struct platform_device		*pdev;
	struct net_device		*dev;
	int chan_id; /* IPC channel id */
	
	/* PHY stuff */
	unsigned int			link;
	unsigned int			speed;
	unsigned int			duplex;
};

static void __ipc_vnet_set_hwaddr(struct ipc_vnet *bp)
{

}

static void ipc_vnet_get_hwaddr(struct ipc_vnet *bp)
{
	
}

static netdev_tx_t ipc_vnet_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct ipc_vnet *bp = netdev_priv(dev);
	void *ipc_buf;
	int i;
	unsigned int aligned_len;
	
	if (skb_shinfo(skb)->nr_frags) {
		pr_err("ipc-vnet: meet frags");
		dev->stats.tx_dropped++;
		dev_kfree_skb(skb);
		return NETDEV_TX_OK;
	}
	aligned_len = (skb->len + 3) >> 2 << 2;
	ipc_buf = ipc_shm_acquire_buf(bp->chan_id, aligned_len);
	if (!ipc_buf) {
		/* no  more buffer*/
		dev->stats.tx_errors++;
		dev_kfree_skb(skb);
		return NETDEV_TX_OK;
	}

	/* copy with 4 bytes unit */
	aligned_len >>= 2;
	for (i = 0 ; i < aligned_len; i++)
		*((uint32_t*)ipc_buf + i) = *((uint32_t*)skb->data + i);
	
	if (ipc_shm_tx(bp->chan_id, ipc_buf, skb->len))
		return NETDEV_TX_BUSY;

	dev->stats.tx_bytes += skb->len;
	dev->stats.tx_packets++;

	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}

static void ipc_vnet_rx_proc(void *arg, int ch, void *buff, uint32_t size)
{
	struct ipc_vnet *bp = arg;
	struct sk_buff *skb;
	
	if (!netif_running(bp->dev))
		return;

	skb = netdev_alloc_skb_ip_align(bp->dev, size);
	if (!skb) {
		bp->dev->stats.rx_dropped++;
		return;
	}
	
	bp->dev->stats.rx_packets++ ;
	bp->dev->stats.rx_bytes += size;
	
	skb_put_data(skb, buff, size);
	skb->protocol = eth_type_trans(skb, bp->dev);
	netif_receive_skb(skb);
}

static int ipc_vnet_open(struct net_device *dev)
{
	struct ipc_vnet *bp = netdev_priv(dev);

	netif_carrier_on(dev);
	netif_start_queue(dev);
	return ipc_reg_rx_callback(bp->chan_id, ipc_vnet_rx_proc, bp);
}

static int ipc_vnet_close(struct net_device *dev)
{
	struct ipc_vnet *bp = netdev_priv(dev);

	ipc_reg_rx_callback(bp->chan_id, NULL, bp);
	netif_stop_queue(dev);
	netif_carrier_off(dev);
	return 0;
}


static void ipc_vnet_get_drvinfo(struct net_device *dev,
			     struct ethtool_drvinfo *info)
{
	strlcpy(info->driver, "ipc-vnet", sizeof(info->driver));
	strlcpy(info->version, "1.0", sizeof(info->version));
	strlcpy(info->bus_info, "0", sizeof(info->bus_info));
}

static const struct ethtool_ops ipc_vnet_ethtool_ops = {
	.get_drvinfo		= ipc_vnet_get_drvinfo,
	.get_link		= ethtool_op_get_link,
	.get_ts_info		= ethtool_op_get_ts_info,
};

static const struct net_device_ops ipc_vnet_netdev_ops = {
	.ndo_open		= ipc_vnet_open,
	.ndo_stop		= ipc_vnet_close,
	.ndo_start_xmit		= ipc_vnet_start_xmit,
	.ndo_set_mac_address	= eth_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
};

static int ipc_vnet_probe(struct platform_device *pdev)
{
	struct resource res;
	struct net_device *dev;
	struct ipc_vnet *bp;
	int ret;
	
	if (of_address_to_resource(pdev->dev.of_node, 0, &res)) {
		pr_err("ipc-vnet: no resource");
		return -EINVAL;
	}
	
	dev = alloc_netdev(sizeof(*bp), "ipc%d", NET_NAME_ENUM, ether_setup);
	if (!dev)
		return -ENOMEM;

	bp = netdev_priv(dev);
	bp->dev = dev;

	platform_set_drvdata(pdev, dev);
	SET_NETDEV_DEV(dev, &pdev->dev);
	
	bp->chan_id = (int)res.start;
	pr_info("ipv-vnet: chan id is %d\n", bp->chan_id);
	
	dev->irq = 0;
	
	dev->netdev_ops = &ipc_vnet_netdev_ops;
	dev->ethtool_ops = &ipc_vnet_ethtool_ops;

	dev->base_addr = bp->chan_id;


	ipc_vnet_get_hwaddr(bp);

	if (!is_valid_ether_addr(dev->dev_addr)) {
		/* choose a random ethernet address */
		eth_hw_addr_random(dev);
		__ipc_vnet_set_hwaddr(bp);
	}

	ret = register_netdev(dev);
	if (ret) {
		dev_err(&pdev->dev, "Cannot register net device, aborting.\n");
		goto err_out_free_dev;
	}

	return 0;
err_out_free_dev:
	free_netdev(dev);
	return ret;
}

static int ipc_vnet_remove(struct platform_device *pdev)
{

	struct net_device *dev;
	struct ipc_vnet *bp;

	dev = platform_get_drvdata(pdev);

	if (dev) {
		bp = netdev_priv(dev);
		unregister_netdev(dev);
		free_netdev(dev);
	}

	return 0;
}

static const struct of_device_id ipc_vnet_id_table[] = {
	{ .compatible = "fsl,s32xx-ipc-vnet" },
	{ /* sentinel */ }
};

static struct platform_driver ipc_vnet_driver = {
	.probe		= ipc_vnet_probe,
	.remove		= ipc_vnet_remove,
	.driver		= {
		.name		= "ipc_vnet",
		.of_match_table = of_match_ptr(ipc_vnet_id_table),
	},
};

module_platform_driver(ipc_vnet_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("NXP Ethernet driver based on IPC");
MODULE_AUTHOR("ryder gong <ryder.gong@nxp.com>");
