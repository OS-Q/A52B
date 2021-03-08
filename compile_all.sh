#!/bin/bash

set -e  # exit if a command fails

current=$(pwd)
if [ ! -d ../arduino-1.8.7 ]; then
	cd ..
	wget http://downloads.arduino.cc/arduino-1.8.7-linux64.tar.xz
	tar xf arduino-1.8.7-linux64.tar.xz
	arduino-1.8.7/arduino --pref "boardsmanager.additional.urls=https://www.briki.org/download/resources/package_briki_index.json,https://dl.espressif.com/dl/package_esp32_dev_index.json" --install-boards briki:mbc-wb > /dev/null
	wget https://github.com/dycodex/LSM6DSL-Arduino/archive/master.zip
	tar xvf LSM6DSL-Arduino-master.zip -C ~/Arduino/libraries
	wget https://github.com/adafruit/Adafruit_VL6180X/archive/master.zip
	tar xvf Adafruit_VL6180X-master.zip -C ~/Arduino/libraries
	cd $current
fi

cd ../arduino-1.8.7
arduinoPath=$(pwd)
export PATH=$PATH:$arduinoPath
cd $current

rm -R -f ~/.arduino15/packages/briki/hardware/mbc-wb/*/* 
mv ./* ~/.arduino15/packages/briki/hardware/mbc-wb/*

for library in `ls ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries`; do
	if [[ $library == *"ABC"* ]]; then # SAMD ABC library
		for example in `ls ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples`; do
			(set -x; arduino --verify --board briki:mbc-wb:abc:mcu=samd ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$example.ino)
		done
		continue
	fi
	if grep -q "(samd21)" ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/library.properties; then # SAMD library
		for example in `ls ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples`; do
			if [ -e ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$example.ino ]; then
				(set -x; arduino --verify --board briki:mbc-wb:mbc:mcu=samd ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$example.ino)
				(set -x; arduino --verify --board briki:mbc-wb:abc:mcu=samd ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$example.ino)
			else # two level folder example?
				for innerExample in `ls ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example`; do
					if [ -e ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$innerExample/$innerExample.ino ]; then
						(set -x; arduino --verify --board briki:mbc-wb:mbc:mcu=samd ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$innerExample/$innerExample.ino)
						(set -x; arduino --verify --board briki:mbc-wb:abc:mcu=samd ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$innerExample/$innerExample.ino)
					fi
				done
			fi
		done
	elif grep -q "(esp32)" ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/library.properties; then # ESP library
		for example in `ls ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples`; do
			if [ -e ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$example.ino ]; then
				(set -x; arduino --verify --board briki:mbc-wb:mbc:mcu=esp,partitions=ffat ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$example.ino)
				(set -x; arduino --verify --board briki:mbc-wb:abc:mcu=esp,partitions=ffat ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$example.ino)
			else # two level folder example?
				for innerExample in `ls ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example`; do
					if [ -e ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$innerExample/$innerExample.ino ]; then
						(set -x; arduino --verify --board briki:mbc-wb:mbc:mcu=esp,partitions=ffat ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$innerExample/$innerExample.ino)
						(set -x; arduino --verify --board briki:mbc-wb:abc:mcu=esp,partitions=ffat ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$innerExample/$innerExample.ino)
					fi
				done
			fi
		done
	else				# generic library
		for example in `ls ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples`; do
			if [[ $example == *"_samd21"* ]] || [ $example = "UartBridge" ] ; then # samd only
				(set -x; arduino --verify --board briki:mbc-wb:mbc:mcu=samd ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$example.ino)
				(set -x; arduino --verify --board briki:mbc-wb:abc:mcu=samd ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$example.ino)
				continue
			fi
			if [[ $example == *"_esp32"* ]]; then # esp only
				(set -x; arduino --verify --board briki:mbc-wb:mbc:mcu=esp,partitions=ffat ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$example.ino)
				(set -x; arduino --verify --board briki:mbc-wb:abc:mcu=esp,partitions=ffat ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$example.ino)
				continue
			fi
			# no restriction found in library or example name. compile for all targets
			if [ -e ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$example.ino ]; then
				(set -x; arduino --verify --board briki:mbc-wb:mbc:mcu=samd ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$example.ino)
				(set -x; arduino --verify --board briki:mbc-wb:abc:mcu=samd ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$example.ino)
				(set -x; arduino --verify --board briki:mbc-wb:mbc:mcu=esp,partitions=ffat ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$example.ino)
				(set -x; arduino --verify --board briki:mbc-wb:abc:mcu=esp,partitions=ffat ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$example.ino)
			else # two level folder example?
				for innerExample in `ls ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example`; do
					if [ -e ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$innerExample/$innerExample.ino ]; then
						(set -x; arduino --verify --board briki:mbc-wb:mbc:mcu=samd ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$innerExample/$innerExample.ino)
						(set -x; arduino --verify --board briki:mbc-wb:abc:mcu=samd ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$innerExample/$innerExample.ino)
						(set -x; arduino --verify --board briki:mbc-wb:mbc:mcu=esp,partitions=ffat ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$innerExample/$innerExample.ino)
						(set -x; arduino --verify --board briki:mbc-wb:abc:mcu=esp,partitions=ffat ~/.arduino15/packages/briki/hardware/mbc-wb/*/libraries/$library/examples/$example/$innerExample/$innerExample.ino)
					fi
				done
			fi
		done
	fi
done

echo "Compilation completed with no error"
