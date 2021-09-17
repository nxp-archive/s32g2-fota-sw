# Copyright 2021 NXP
#
# SPDX-License-Identifier:  GPL-2.0
#
obj-m := ipc-main.o ipc-vnet.o
#kload-m7-y := kload-m7_main.o

modules:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

modules_install: modules
	$(MAKE) -C $(KDIR) M=$(CURDIR) INSTALL_MOD_PATH=$(INSTALL_DIR) $@

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean