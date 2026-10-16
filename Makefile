obj-m += journal_module.o

PWD := $(CURDIR)
no-dot-config-targets := %menuconfig %config %clean %fullclean
need-config := 1
KCONFIG_CONFIG	?= .config

ifneq ($(wildcard .config),)
need-config :=
include $(KCONFIG_CONFIG)
endif

ifneq ($(filter $(no-dot-config-targets), $(MAKECMDGOALS)),)
	ifeq ($(filter-out $(no-dot-config-targets), $(MAKECMDGOALS)),)
		need-config :=
	endif
endif

ifdef need-config
$(KCONFIG_CONFIG):
	@echo >&2 '***'
	@echo >&2 '*** Configuration file "$@" not found!'
	@echo >&2 '***'
	@echo >&2 '*** Please run "make menuconfig" or "make config".'
	@echo >&2 '***'
	@/bin/false
include $(KCONFIG_CONFIG)
endif

all:
	make -j 8 -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

fullclean: clean cleanconfig

clean:
	make -j 8 -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

cleanconfig:
	@rm -rf .config include/

config:
	/lib/modules/$(shell uname -r)/build/scripts/kconfig/conf Kconfig

menuconfig:
	/lib/modules/$(shell uname -r)/build/scripts/kconfig/mconf Kconfig
