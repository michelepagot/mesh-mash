# ESP-WIFI-MESH

This project is an attempt to use the official Espressif MESH library, the official implementation of ESP-WIFI-MESH.



## Prerequisite
In order to build this project you need a esp-idf environment. Start from: https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/get-started/index.html

I decided to go for the cli base installation is described in https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/get-started/linux-macos-setup.html

I also decided to have all the tools in a specific folder, by using `IFD_TOOLS_PATH` env variable

```
% cd <SOMEWHERE>
% mkdir idf_tools
% export IDF_TOOLS_PATH=<SOMEWHERE>/idf_tools
% git clone -b v5.0 --recursive https://github.com/espressif/esp-idf.git
% cd esp-idf
% ./install.sh esp32
% source ./export.sh
```

I also tested the VSCode ES-IFD plugin and it is pretty simple to use too: it has a dedicated command to download and install all the tools.


## Project boostrap
This part is documented in https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/get-started/linux-macos-setup.html#get-started-linux-macos-first-steps

The project structure has been bootstraped using the `idf.py` tool like:

```
% export IDF_TOOLS_PATH=<SOMEWHERE>/idf_tools
% source <SOMEWERE>/esp-idf/export.sh
% idf.py create-project esp-wifi-mesh
% cd esp-wifi-mesh
% idf.py set-target esp32
% idf.py menuconfig
```

I just edited `esp-wifi-mesh/main/esp-wifi-mesh.c` to print something and then I build

## Run the code

```
% export IDF_TOOLS_PATH=<SOMEWHERE>/idf_tools
% idf.py build
```

Load and run the SW is as easy ( I need to manually press some button on my board due to https://github.com/espressif/esptool/issues/831 ) as:

```
% idf.py --port /dev/ttyUSB0 flash
% idf.py --port /dev/ttyUSB0 monitor
```

`CTRL-]` to exit from the monitor.

In case you need more control during the flashing it is possible to check the command in the output of `idf.py flash` and run it manually like

```
% python ${IDF_PATH}/components/esptool_py/esptool/esptool.py -p /dev/ttyUSB0 -b 921600 --before=default_reset --after=hard_reset write_flash --flash_mode dio --flash_freq 40m --flash_size detect 0x1000 build/bootloader/bootloader.bin 0x10000 build/esp-wifi-mesh.bin 0x8000 build/partition_table/partition-table.bin
```
