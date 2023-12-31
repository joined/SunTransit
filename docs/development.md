The project is composed of three main parts:

-   The ESP32 project, developed with the [ESP-IDF framework](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
-   The Linux simulator for the GUI (based on [LVGL](https://lvgl.io/))
-   The frontend React SPA served by the ESP32 which allows to configure it and get system information

## Dependencies

Install the Python dependencies via `pip install -r requirements.txt`.
Either install them globally or in a virtual environment. If you use `idf.py` from the command line, you might want to install them
in the IDF virtual environment, so that they are available when activating the IDF virtual environment.

## ESP32

Install the [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) framework, preferably via the [VSCode Extension](https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/tutorial/install.md).

## Simulator

The simulator code lives in `simulator` and is used to develop the UI. The UI library is symlinked from the ESP folder.
It is developed using the [PlatformIO](https://platformio.org/) framework.
Follow [these instructions](https://platformio.org/install/ide?install=vscode) to get started using it within VSCode.
It uses [libsdl](https://github.com/libsdl-org/SDL), make sure to install it (e.g. `sudo apt-get install libsdl2-dev`).

## Frontend

The frontend is developed using React.
You'll need to use to use [pnpm](https://pnpm.io/) to build it.

Useful commands:

-   `pnpm i`: installs the dependencies
-   `pnpm start`: starts a dev server with hot reloading and the mock backend API server
-   `pnpm build`: builds the gzipped production version of the app, to be stored in the data partition of the ESP

!!! note "VSCode C++ Extension"

    The `simulator` folder should be handled as separate VSCode workspace.
    This is due to PlatformIO insisting on owning and overwriting `.vscode/c_cpp_properties.json` which makes it impossible to have a functioning IDE
    if you want to use the "ESP-IDF" extension rather than PlatformIO's own build system.
    We don't use PlatformIO's build system due to the poor integration with `idf.py`.

    Please copy `.vscode/c_cpp_properties.sample.json` to `.vscode/c_cpp_properties.json` and read the instructions for ESP-IDF development.
    PlatformIO will generate `c_cpp_properties.json` automatically based on `platformio.io` for the simulator.
