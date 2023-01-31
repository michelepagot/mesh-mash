Initialized with 

```
pio init -d $(pwd) -b esp32dev --ide vscode
```

Add `painlessMesh` to the project dependency, in `platformio.ini` file.

Create a main.cpp in `src/main.cpp`

Build and upload

```
pio run -t upload
```

