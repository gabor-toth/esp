Links:
  * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html#get-started-first-steps
  * https://github.com/espressif/idf-eclipse-plugin/blob/master/README.md#GettingStarted
  * https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32s2/hw-reference/chip-series-comparison.html

Install
=======

```
sudo apt-get install bison ccache cmake dfu-util flex gperf libffi-dev libssl-dev libusb-1.0-0 ninja-build python3 python-is-python3 python3-pip python3-setuptools python3-venv
#sudo apt-get install git wget 
#sudo apt-mark auto libusb-1.0-0 python3 python3-setuptools wget 
```

```
cd ~/own/projects
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32 esp32s2
```

LVGL
----

 * https://github.com/lvgl/lv_port_esp32
 
```
cd esp-idf

git submodule add https://github.com/lvgl/lvgl.git components/lvgl
pushd components/lvgl
git checkout release/v8.3
popd

git submodule add https://github.com/lvgl/lvgl_esp32_drivers.git components/lvgl_esp32_drivers
```

NMEA2000
--------

In menuconfig check "Enable C++ extension" before adding the component

```
mkdir -p components
cd components
git submodule add https://github.com/ttlappalainen/NMEA2000
```

Upgrade
=======

Upgrade esp-idf
---------------

```
pushd ../../esp-idf
git pull
git submodule update --init --recursive
rm -rf /home/tothg/.espressif/
./install.sh esp32 esp32s2
. ./export.sh
cd tools
./idf_tools.py install cmake
popd
```

Upgrade project in command line
-------------------------------

```
idf.py fullclean
idf.py menuconfig
idf.py build
```

Upgrade project in CLion
------------------------

  * ``echo $PATH``
  * Change IDF_PATH and PATH in CLion
  * File/Reload CMake project

Compile & more
==============

init.sh does this
```
. ~/own/projects/esp-idf/export.sh
```

Find out device

```
$ l /dev/ttyUSB*
crw-rw---- 1 root dialout 188, 0 ápr    5 20:01 /dev/ttyUSB0
sudo usermod -a -G dialout tothg
```

New project
-----------

Copy init.sh, run it

```
. ./init.sh

idf.py set-target esp32s2
idf.py menuconfig
idf.py -p /dev/ttyUSB0 flash monitor
```

In CLion
  * File, Open
  * CMakeLists.txt

Change target 
-------------

```
idf.py set-target esp32s2
rm -rf cmake-build-debug/
idf.py fullclean
```

Reload CMake project in CLion

Add managed components

```
idf.py add-dependency espressif/mdns
idf.py add-dependency espressif/mpu6050
idf.py build
```

Git
===

Use different SSL private key

```
export GIT_SSH_COMMAND="ssh -i $HOME/.ssh/id_gabtoth -o IdentitiesOnly=yes"
```

CLion
=====

CMake environment (Settings / Build, Execution, Deployment / CMake / Environment)

```
IDF_PATH=/home/tothg/own/projects/esp-idf;PATH=/home/tothg/own/projects/esp-idf/components/esptool_py/esptool:/home/tothg/own/projects/esp-idf/components/espcoredump:/home/tothg/own/projects/esp-idf/components/partition_table:/home/tothg/own/projects/esp-idf/components/app_update:/home/tothg/.espressif/tools/xtensa-esp-elf-gdb/12.1_20221002/xtensa-esp-elf-gdb/bin:/home/tothg/.espressif/tools/xtensa-esp32-elf/esp-2022r1-11.2.0/xtensa-esp32-elf/bin:/home/tothg/.espressif/tools/xtensa-esp32s2-elf/esp-2022r1-11.2.0/xtensa-esp32s2-elf/bin:/home/tothg/.espressif/tools/riscv32-esp-elf/esp-2022r1-11.2.0/riscv32-esp-elf/bin:/home/tothg/.espressif/tools/esp32ulp-elf/2.35_20220830/esp32ulp-elf/bin:/home/tothg/.espressif/tools/cmake/3.24.0/bin:/home/tothg/.espressif/tools/openocd-esp32/v0.11.0-esp32-20221026/openocd-esp32/bin:/home/tothg/.espressif/tools/xtensa-esp-elf-gdb/12.1_20221002/xtensa-esp-elf-gdb/bin:/home/tothg/.espressif/tools/xtensa-esp32-elf/esp-2022r1-11.2.0/xtensa-esp32-elf/bin:/home/tothg/.espressif/tools/xtensa-esp32s2-elf/esp-2022r1-11.2.0/xtensa-esp32s2-elf/bin:/home/tothg/.espressif/tools/riscv32-esp-elf/esp-2022r1-11.2.0/riscv32-esp-elf/bin:/home/tothg/.espressif/tools/esp32ulp-elf/2.35_20220830/esp32ulp-elf/bin:/home/tothg/.espressif/tools/cmake/3.24.0/bin:/home/tothg/.espressif/tools/openocd-esp32/v0.11.0-esp32-20221026/openocd-esp32/bin:/home/tothg/.espressif/python_env/idf5.1_py3.10_env/bin:/home/tothg/own/projects/esp-idf/tools:/home/tothg/.local/bin:/home/tothg/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/snap/bin:/home/tothg/jpm/bin:/opt/mssql-tools/bin
```

Something else

```
PATH += /home/tothg/.espressif/tools/xtensa-esp32-elf/esp-2021r2-patch3-8.4.0/xtensa-esp32-elf/bin:/home/tothg/.espressif/tools/xtensa-clang/12.0.1-d9341b81fc/xtensa-esp32-elf-clang/bin:/home/tothg/.espressif/tools/esp32ulp-elf/2.28.51-esp-20191205/esp32ulp-elf-binutils/bin:/home/tothg/.espressif/tools/cmake/3.20.3/bin:/home/tothg/.espressif/tools/openocd-esp32/v0.11.0-esp32-20211220/openocd-esp32/bin:/home/tothg/.espressif/tools/ninja/1.10.2/:/home/tothg/.espressif/python_env/idf5.0_py3.8_env/bin:/home/tothg/own/projects/esp-idf/tools:
IDF_PATH = /home/tothg/own/projects/esp-idf
```

Hardware
========

Display
-------

  * https://electropeak.com/learn/interfacing-2-8-inch-tft-lcd-touch-screen-with-esp32/
  * http://www.lcdwiki.com/2.8inch_SPI_Module_ILI9341_SKU:MSP2807

