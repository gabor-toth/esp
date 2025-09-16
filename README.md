Links:
  * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html#get-started-first-steps
  * https://github.com/espressif/idf-eclipse-plugin/blob/master/README.md#GettingStarted
  * https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32s2/hw-reference/chip-series-comparison.html

Install
=======

```
sudo apt -y install bison ccache cmake curl dfu-util flex gperf libffi-dev libssl-dev libusb-1.0-0 
sudo apt -y install ninja-build python3 python-is-python3 python3-pip python3-setuptools python3-venv
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

NPM
---

- https://github.com/nvm-sh/nvm

```
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.3/install.sh | bash
# run it on a new terminal
nvm install 22 --save
```

NG
--

```
cd ~/projects/ecp/ontozo/frontend
npm install
npm link @angular/cli
```

Upgrade
=======

Upgrade esp-idf
---------------

```
pushd ../../esp-idf
git pull
git checkout v5.1.2
git submodule update --init --recursive
rm -rf $HOME/.espressif/
./install.sh esp32 esp32s2
. ./export.sh
cd tools
./idf_tools.py install cmake install-python-env
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
sudo usermod -a -G dialout gabor
```

If there's no ttyUSB0
```
sudo apt remove brltty
```

New project
-----------

Copy init.sh, run it

```
. ./init.sh

idf.py set-target esp32s2
idf.py menuconfig
idf.py -p /dev/ttyUSB0 flash -b 3000000 monitor
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
idf.py add-dependency espressif/esp_websocket_client
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
IDF_PATH=/home/gabor/projects/esp/esp-idf;PATH=home/gabor/projects/esp/esp-idf/components/espcoredump:/home/gabor/projects/esp/esp-idf/components/partition_table:/home/gabor/projects/esp/esp-idf/components/app_update:/home/gabor/.espressif/tools/xtensa-esp-elf-gdb/14.2_20240403/xtensa-esp-elf-gdb/bin:/home/gabor/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20241119/xtensa-esp-elf/bin:/home/gabor/.espressif/tools/riscv32-esp-elf/esp-14.2.0_20241119/riscv32-esp-elf/bin:/home/gabor/.espressif/tools/esp32ulp-elf/2.38_20240113/esp32ulp-elf/bin:/home/gabor/.espressif/tools/cmake/3.30.2/bin:/home/gabor/.espressif/tools/openocd-esp32/v0.12.0-esp32-20250226/openocd-esp32/bin:/home/gabor/.espressif/python_env/idf5.4_py3.12_env/bin:/home/gabor/projects/esp/esp-idf/tools:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/snap/bin
```

Windows
-------

```
[Environment]::GetEnvironmentVariable("IDF_PATH")
$Env:Path
python C:\Espressif\frameworks\esp-idf-v5.2.1-2\tools\idf_tools.py install-python-env
```

Something else

```
PATH += /home/tothg/.espressif/tools/xtensa-esp32-elf/esp-2021r2-patch3-8.4.0/xtensa-esp32-elf/bin:/home/tothg/.espressif/tools/xtensa-clang/12.0.1-d9341b81fc/xtensa-esp32-elf-clang/bin:/home/tothg/.espressif/tools/esp32ulp-elf/2.28.51-esp-20191205/esp32ulp-elf-binutils/bin:/home/tothg/.espressif/tools/cmake/3.20.3/bin:/home/tothg/.espressif/tools/openocd-esp32/v0.11.0-esp32-20211220/openocd-esp32/bin:/home/tothg/.espressif/tools/ninja/1.10.2/:/home/tothg/.espressif/python_env/idf5.0_py3.8_env/bin:/home/gabor/projects/esp-idf/tools:
IDF_PATH = /home/gabor/projects/esp-idf
```

USB on Windows
--------------

- https://blog.manzelseet.com/fixing-cp2102-with-custom-vidpid.html
- https://community.silabs.com/s/article/downloading-cp210x-drivers-from-windows-update?language=en_US
  The PIDs that must be programmed to the CP210x device are listed below: 0x10C4 0xEA63
- https://github.com/DiUS/cp210x-cfg/blob/master/README.md
- https://blog.manzelseet.com/fixing-cp2102-with-custom-vidpid.html
- https://community.silabs.com/s/question/0D51M00007xeNnTSAU/an721-cp21xxcustomizationutilityexe-not-found?language=en_US


Hardware
========

Display
-------

  * https://electropeak.com/learn/interfacing-2-8-inch-tft-lcd-touch-screen-with-esp32/
  * http://www.lcdwiki.com/2.8inch_SPI_Module_ILI9341_SKU:MSP2807

KiCad
=====

* search parts: https://componentsearchengine.com/
* library loader: https://componentsearchengine.com/ga/libraryLoaderSetup.php?flow=ll

links
* https://ms.componentsearchengine.com/library/kicad


TODO
====

fatal: remote error: upload-pack: not our ref 04b38e68fdf662cc866fca628e3e67a9714209d1
fatal: remote error: upload-pack: not our ref e1ba2bd2f61cd81c16b31b09772967fbfd72edd2
fatal: remote error: upload-pack: not our ref 11aa66e7f39c9402039882acb14a993e390f8893
fatal: remote error: upload-pack: not our ref 848df55b08dcdad33c7dd9dbc25deacd2fc5e489
fatal: remote error: upload-pack: not our ref 30e5c4f95bff1a17e06ce1042a165b0aa2bc4ca5
Errors during submodule fetch:
	components/lwip/lwip
	components/esp_wifi/lib
	components/bt/controller/lib_esp32h2/esp32h2-bt-lib
	components/bt/controller/lib_esp32c6/esp32c6-bt-lib
	components/bt/controller/lib_esp32c2/esp32c2-bt-lib
