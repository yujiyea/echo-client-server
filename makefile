.PHONY : tc ts clean install uninstall android-install android-uninstall

all: tc ts

tc:
	cd tc; make; cd ..

ts:
	cd ts; make; cd ..

clean:
	cd tc; make clean; cd ..
	cd ts; make clean; cd ..

install:
	sudo cp bin/tc /usr/sbin
	sudo cp bin/ts /usr/sbin

uninstall:
	sudo rm /usr/sbin/tc /usr/sbin/ts 

android-install:
	adb push bin/tc bin/ts /data/local/tmp
	adb exec-out "su -c 'mount -o rw,remount /system'"
	adb exec-out "su -c 'cp /data/local/tmp/tc /data/local/tmp/ts /system/xbin'"
	adb exec-out "su -c 'chmod 755 /system/xbin/tc'"
	adb exec-out "su -c 'chmod 755 /system/xbin/ts'"

	adb exec-out "su -c 'mount -o ro,remount /system'"
	adb exec-out "su -c 'rm /data/local/tmp/tc /data/local/tmp/ts '"

android-uninstall:
	adb exec-out "su -c 'mount -o rw,remount /system'"
	adb exec-out "su -c 'rm /system/xbin/tc /system/xbin/ts'"
	adb exec-out "su -c 'mount -o ro,remount /system'"
