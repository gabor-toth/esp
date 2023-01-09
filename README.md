Links:
  * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html#get-started-first-steps
  * https://github.com/espressif/idf-eclipse-plugin/blob/master/README.md#GettingStarted

Install
=======

```
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-setuptools python3.8-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
sudo apt-mark auto libusb-1.0-0 python3 python3-setuptools wget 
```

```
cd ~/own/projects
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32 esp32s2
```

LVGL
----

```
cd esp-idf
git submodule add https://github.com/lvgl/lvgl.git components/lvgl
pushd components/lvgl
git checkout release/v8.3
popd

cd ..
git clone https://github.com/lvgl/lv_port_esp32.git
mv lv_port_esp32/components/lvgl_esp32_drivers/ esp-idf/components/
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
./install.sh esp32
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

  * Change IDF_PATH and PATH in CLion
  * File/Reload CMake project

Compile & more
==============

Find out device

```
$ l /dev/ttyUSB*
crw-rw---- 1 root dialout 188, 0 ápr    5 20:01 /dev/ttyUSB0
sudo usermod -a -G dialout tothg
```

copy init.sh, run .sh

```
. ./init.sh

idf.py set-target esp32
# or
idf.py set-target esp32s2
idf.py menuconfig
idf.py -p /dev/ttyUSB0 flash monitor
```

New project
-----------

File, Open, CMakeLists.txt

Git
===

```
export GIT_SSH_COMMAND="ssh -i $HOME/.ssh/id_gabtoth -o IdentitiesOnly=yes"
```

CLion
=====

CMake environment (Settings / Build, Execution, Deployment / CMake / Environment)

```
IDF_PATH=/home/tothg/own/projects/esp-idf;PATH=/home/tothg/.espressif/tools/xtensa-esp-elf-gdb/11.2_20220529/xtensa-esp-elf-gdb/bin:/home/tothg/.espressif/tools/xtensa-esp32-elf/esp-2022r1-RC1-11.2.0/xtensa-esp32-elf/bin:/home/tothg/.espressif/tools/esp32ulp-elf/2.28.51-esp-20191205/esp32ulp-elf-binutils/bin:/home/tothg/.espressif/tools/openocd-esp32/v0.11.0-esp32-20220411/openocd-esp32/bin:/home/tothg/.espressif/python_env/idf5.0_py3.8_env/bin:/home/tothg/own/projects/esp-idf/tools:/home/tothg/.local/bin:/home/tothg/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/home/tothg/jpm/bin:/opt/mssql-tools/bin
```

Something else

```
PATH += /home/tothg/.espressif/tools/xtensa-esp32-elf/esp-2021r2-patch3-8.4.0/xtensa-esp32-elf/bin:/home/tothg/.espressif/tools/xtensa-clang/12.0.1-d9341b81fc/xtensa-esp32-elf-clang/bin:/home/tothg/.espressif/tools/esp32ulp-elf/2.28.51-esp-20191205/esp32ulp-elf-binutils/bin:/home/tothg/.espressif/tools/cmake/3.20.3/bin:/home/tothg/.espressif/tools/openocd-esp32/v0.11.0-esp32-20211220/openocd-esp32/bin:/home/tothg/.espressif/tools/ninja/1.10.2/:/home/tothg/.espressif/python_env/idf5.0_py3.8_env/bin:/home/tothg/own/projects/esp-idf/tools:
IDF_PATH = /home/tothg/own/projects/esp-idf
```


