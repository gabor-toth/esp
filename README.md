https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html#get-started-first-steps
https://github.com/espressif/idf-eclipse-plugin/blob/master/README.md#GettingStarted

PATH += /home/tothg/.espressif/tools/xtensa-esp32-elf/esp-2021r2-patch3-8.4.0/xtensa-esp32-elf/bin:/home/tothg/.espressif/tools/xtensa-clang/12.0.1-d9341b81fc/xtensa-esp32-elf-clang/bin:/home/tothg/.espressif/tools/esp32ulp-elf/2.28.51-esp-20191205/esp32ulp-elf-binutils/bin:/home/tothg/.espressif/tools/cmake/3.20.3/bin:/home/tothg/.espressif/tools/openocd-esp32/v0.11.0-esp32-20211220/openocd-esp32/bin:/home/tothg/.espressif/tools/ninja/1.10.2/:/home/tothg/.espressif/python_env/idf5.0_py3.8_env/bin:/home/tothg/own/projects/esp-idf/tools:
IDF_PATH = /home/tothg/own/projects/esp-idf

Install
=======

```sudo apt-get install git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
sudo apt-mark auto libusb-1.0-0 python3 python3-setuptools wget 
```

$ l /dev/ttyUSB*
crw-rw---- 1 root dialout 188, 0 ápr    5 20:01 /dev/ttyUSB0
sudo usermod -a -G dialout tothg

```
git clone --recursive https://github.com/espressif/esp-idf.git .

cd ~/own/projects/esp-idf
./install.sh esp32
```

Compile & more
==============

```
. ~/own/projects/esp-idf/export.sh

#su - $USER

idf.py set-target esp32
idf.py menuconfig
idf.py -p /dev/ttyUSB0 flash monitor
```

Git
===

```
export GIT_SSH_COMMAND="ssh -i $HOME/.ssh/id_gabtoth -o IdentitiesOnly=yes"
```

CLion
=====

CMake environment

```
IDF_PATH=/home/tothg/own/projects/esp-idf;PATH=/home/tothg/.espressif/tools/xtensa-esp32-elf/esp-2021r2-patch3-8.4.0/xtensa-esp32-elf/bin:/home/tothg/.espressif/tools/xtensa-clang/12.0.1-d9341b81fc/xtensa-esp32-elf-clang/bin:/home/tothg/.espressif/tools/esp32ulp-elf/2.28.51-esp-20191205/esp32ulp-elf-binutils/bin:/home/tothg/.espressif/tools/cmake/3.20.3/bin:/home/tothg/.espressif/tools/openocd-esp32/v0.11.0-esp32-20211220/openocd-esp32/bin:/home/tothg/.espressif/tools/ninja/1.10.2/:/home/tothg/.espressif/python_env/idf5.0_py3.8_env/bin:/home/tothg/own/projects/esp-idf/tools:/home/tothg/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin
```

