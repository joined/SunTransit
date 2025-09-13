The project is composed of three main parts:

-   The ESP32 project, developed with the [ESP-IDF framework](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
-   The Linux simulator for the GUI (based on [LVGL](https://lvgl.io/))
-   The frontend React SPA served by the ESP32 which allows to configure it and get system information

## Dependencies

Install the Python dependencies via `pip install -r requirements.txt`.
Either install them globally or in a virtual environment. If you use `idf.py` from the command line, you might want to install them
in the IDF virtual environment, so that they are available when activating the IDF virtual environment.

## VSCode
It is recommended to install the [VScode C++ Extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) and to have a look at `.vscode/c_cpp_properties.sample.json` for a possible configuration.

## ESP32

Install the [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) framework, preferably via the [VSCode Extension](https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/tutorial/install.md).

## Simulator

The simulator code lives in `simulator` and is used to develop/design the UI.
It is developed using the [PlatformIO](https://platformio.org/) framework.
Follow [these instructions](https://platformio.org/install/cli) to get started with PlatformIO core.
We don't recommend using the corresponding VSCode extension because it insists on owning `.vscode/c_cpp_properties.json` which makes developing for both the ESP and the simulator difficult.
The simulator uses [libsdl](https://github.com/libsdl-org/SDL), make sure to install it (e.g. `sudo apt-get install libsdl2-dev`).

## Frontend

The frontend is developed using React.
You'll need to use to use [pnpm](https://pnpm.io/) to build it.

Useful commands:

-   `pnpm i`: installs the dependencies
-   `pnpm start`: starts a dev server with hot reloading and the mock backend API (via msw)
-   `pnpm build`: builds the gzipped production version of the app, to be stored in the data partition of the ESP
