# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SunTransit is an ESP32-based IoT project that displays Berlin public transport (BVG) departure information on Sunton LCD displays.
The project consists of three main components:

1. **ESP32 Firmware** - ESP-IDF based C++ application
2. **React Frontend** - Web interface for configuration
3. **LVGL Simulator** - Desktop GUI development environment

## Common Commands

### Frontend Development
- `pnpm i` - Install dependencies (pnpm is required, enforced by preinstall hook)
- `pnpm start` - Start development server with hot reload + mock backend API (via msw)
- `pnpm build` - Build production version (creates gzipped files in frontend_dist/)
- `pnpm test` - Run Jest tests for frontend components
- `pnpm run lint:frontend:eslint` - Run ESLint on frontend code
- `pnpm run lint:frontend:ts` - TypeScript type checking
- `pnpm run lint:prettier` - Check Prettier formatting

### ESP32 Development
- `idf.py build` - Build ESP32 firmware (requires ESP-IDF environment)
- `idf.py flash` - Flash firmware to device
- `idf.py monitor` - Open serial monitor
- `idf.py flash monitor` - Flash and immediately start monitoring
- `idf.py menuconfig` - Configure project settings

### Simulator Development
- Install libsdl2 dependency: `sudo apt-get install libsdl2-dev`
- `pio run` - Build simulator (runs from project root, includes UI from `esp/ui/` directly)
- `pio run -t upload -e 3248s035c` - Build and run simulator for 480x320px screen
- `pio run -t upload -e 8048s070c` - Build and run simulator for 800x480px screen

### Code Quality
- `clang-format -i esp/**/*.{c,cpp,h,hpp}` - Format C++ code
- `prettier --write .` - Format all supported files
- Pre-commit hooks automatically run prettier, clang-format, and ESLint

## Architecture and Key Components

### ESP32 Firmware Structure (`esp/` directory)
- **Main Controller** (`main.cpp`) - WiFi provisioning, task orchestration
- **BVG API Client** (`bvg_api_client.*`) - Fetches Berlin transport departure data
- **HTTP Server** (`http_server.*`) - Serves web interface and REST API
- **LCD Controller** (`lcd.*`) - Display and touch screen initialization 
- **UI Manager** (`ui/ui.*`) - LVGL-based screen management and rendering
- **NVS Engine** (`nvs_engine.*`) - Non-volatile storage for configuration
- **Time Management** (`time.*`) - SNTP synchronization and timezone handling

### Frontend Structure (`frontend/` directory)
- React 19 SPA with Material-UI components
- Uses SWR for API state management
- TypeScript throughout
- Parcel for bundling
- Automatically gzipped during build for ESP32 embedding

### Development Workflow Notes
- The frontend is built first and embedded into ESP32 firmware
- Copy `.vscode/c_cpp_properties.sample.json` to `.vscode/c_cpp_properties.json` for ESP-IDF development
- Pre-commit hooks enforce formatting and linting

### Hardware Target
Sunton 3248S035C (ESP32 + 3.5" LCD 480x320px) and 8048S070C (ESP32-S3 + 7.0" LCD 800x480px)

### Memory and Performance Considerations
- LVGL configuration optimized for ESP32 memory constraints
- Web assets are gzipped and served from SPIFFS partition (512KB)
- Custom sdkconfig.defaults contains ESP-IDF optimizations

### Testing and Quality Assurance
- Jest configured for frontend testing with TypeScript support and jsdom environment
- Test files located in `frontend/__tests__/` directory
- Testing libraries include @testing-library/react, @testing-library/jest-dom, and @testing-library/user-event
- No unit tests currently exist for ESP32 firmware
- CI/CD focuses on build verification and code formatting
- Use simulator for UI development without physical hardware
