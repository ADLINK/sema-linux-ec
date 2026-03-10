# SPDX-License-Identifier: BSD-3-Clause
DESTDIR ?=
PREFIX ?= /usr

SEMA_OBJS = $(patsubst %.c,%.o,$(wildcard lib/*.c))
WDOG_OBJS = $(patsubst %.c,%.o,$(wildcard watchdogtest/*.c))
APP_OBJS = $(patsubst %.c,%.o,$(wildcard app/*.c))
obj-m := driver/adl-ec.o \
         driver/adl-ec-bklight.o \
         driver/adl-ec-wdt.o \
         driver/adl-ec-boardinfo.o \
         adl-ec-nvmem-sec.o \
         adl-ec-nvmem.o \
         driver/adl-ec-vm.o \
         driver/adl-ec-hwmon.o \
         driver/adl-ec-i2c.o \
         driver/adl-ec-gpio.o 

obj-m += driver/adl-ec-gpio-irq.o

DIR1 = /lib/modules/$(shell uname -r)/updates
DIR2 = /lib/modules/$(shell uname -r)/extra

adl-ec-nvmem-sec-m := driver/adl-ec-nvmem-sec.o driver/nvmem-common.o	 
adl-ec-nvmem-m := driver/adl-ec-nvmem.o driver/nvmem-common.o	 
all: app_build modules

driver: modules

libsema.so: $(SEMA_OBJS)
	@$(CC) -shared -fPIC -g -o lib/$@ $^

app_build: libsema.so semautil wdogtest

modules:
	@make -C /lib/modules/`uname -r`/build M=`pwd` $@

irq:
	@make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) driver/adl-ec-gpio-irq.ko

clean: driver_clean app_clean

install: all driver_install app_install

check_dir1:
	@if [ ! -d "$(DIR1)" ]; then \
        	mkdir -p "$(DIR1)"; \
        	mkdir -p "$(DIR1)/driver"; \
        fi

check_dir2:
	@if [ ! -d "$(DIR2)" ]; then \
                mkdir -p "$(DIR2)"; \
                mkdir -p "$(DIR2)/driver"; \
        fi


install_irq: irq  check_dir1 check_dir2 irq_install

irq_install: 
	@FILE=/lib/modules/`uname -r`/build/certs; if [ ! -f $FILE ]; then mkdir -p /lib/modules/`uname -r`/build/certs; fi
	@openssl req -new -nodes -utf8 -sha512 -days 36500 -batch -x509 -config x509.genkey -outform PEM -out signing_key.x509 -keyout signing_key.pem > /dev/null
	@cp signing_key.pem /lib/modules/`uname -r`/build/certs/
	@cp signing_key.x509 /lib/modules/`uname -r`/build/certs/
	@make -C /lib/modules/`uname -r`/build M=`pwd` modules_install
	@depmod -a
	@if [ ! -d "$(DIR1)" ]; then \
        	sudo cp driver/adl-ec-gpio-irq.ko /lib/modules/$(shell uname -r)/updates/driver/; \
        	echo "Copied to updates/driver"; \
    	else \
        	sudo cp driver/adl-ec-gpio-irq.ko /lib/modules/$(shell uname -r)/extra/driver/; \
        	echo "Copied to extra/driver"; \
    	fi
	@sudo depmod -a

driver_install:
	@FILE=/lib/modules/`uname -r`/build/certs; if [ ! -f $FILE ]; then mkdir -p /lib/modules/`uname -r`/build/certs; fi 
	@openssl req -new -nodes -utf8 -sha512 -days 36500 -batch -x509 -config x509.genkey -outform PEM -out signing_key.x509 -keyout signing_key.pem > /dev/null
	@cp signing_key.pem /lib/modules/`uname -r`/build/certs/
	@cp signing_key.x509 /lib/modules/`uname -r`/build/certs/
	@make -C /lib/modules/`uname -r`/build M=`pwd` modules_install
	@depmod -a

app_install:
	@install -d $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/bin
	@install -m 755 lib/libsema.so $(DESTDIR)$(PREFIX)/lib
	@install -m 755 wdogtest semautil $(DESTDIR)$(PREFIX)/bin

driver_clean:
	@make -C /lib/modules/`uname -r`/build M=`pwd` clean
	@if [ -d "/lib/modules/`uname -r`/extra" ]; then rm -rf /lib/modules/`uname -r`/extra/adl-bmc*; fi
	@if [ -d "/lib/modules/`uname -r`/extra/driver" ]; then rm -rf /lib/modules/`uname -r`/extra/driver/adl-bmc*; fi
	@if [ -d "/lib/modules/`uname -r`/updates" ]; then rm -rf /lib/modules/`uname -r`/updates/adl-bmc*; fi
	@if [ -d "/lib/modules/`uname -r`/updates/driver" ]; then rm -rf /lib/modules/`uname -r`/updates/driver/adl-bmc*; fi

app_clean:
	@rm -f semautil wdogtest app/*.o lib/*.o lib/*.so

semautil: libsema.so $(APP_OBJS)
	@$(CC) -g -o $@ $(APP_OBJS) -Llib -lsema -luuid

wdogtest: $(WDOG_OBJS)
	@$(CC) $^ -g -o $@

lib/%.o: lib/%.c
	@$(CC) -Wall -I lib -g -fPIC -c $< -o $@

app/%.o: app/%.c
	@$(CC) -Wall -I lib -g -fPIC -c $< -o $@

watchdogtest/%.o: watchdogtest/%.c
	@$(CC) -Wall -I lib -g -fPIC -c $< -o $@
