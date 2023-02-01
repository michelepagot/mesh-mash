# PainlessMesh

PainlessMesh is a Wifi MESH library. The official project page is [here](https://gitlab.com/painlessMesh/painlessMesh).
It is also available in the [ProjectIO library register](https://registry.platformio.org/libraries/painlessmesh/painlessMesh)

## Tools

Get PlatformIO CLI as describer [here](https://docs.platformio.org/en/latest/core/installation/index.html)

## Create a new project

Initialized with

``` bash
pio init -d $(pwd) -b esp32dev --ide vscode
```

Add `painlessMesh` to the project dependency, in `platformio.ini` file.

Create a main.cpp in `src/main.cpp`, maybe following one of the [examples](https://gitlab.com/painlessMesh/painlessMesh/-/blob/develop/examples/).

Build and upload

``` bash
pio run -t upload
```
