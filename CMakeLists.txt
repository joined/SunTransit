cmake_minimum_required(VERSION 3.16)
# TODO Set `PROJECT_VER`
# TODO Set `PROJECT_NAME`

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# "Trim" the build. Include the minimal set of components, main and anything it depends on.
# This requires us to list all the components required by main in its CMakeLists.txt, via `REQUIRES`.
set(COMPONENTS esp)
set(EXTRA_COMPONENT_DIRS esp)

# For the LCD/Touch initializers in `lcd.h`
idf_build_set_property(COMPILE_OPTIONS "-Wno-missing-field-initializers" APPEND)

# TODO Fix deprecated enum conversion warning
# Seems like it's fixed on `master`, but not in the latest release (v8.3)

# TODO The following should be handled via menuconfig, but they're missing in the KConfig
# TODO These cause warnings like:
# `warning: ignoring attribute 'section (".iram1.4")' because it conflicts with previous 'section (".iram1.1")' [-Wattributes]`
# because IRAM_ATTR expands to a macro that uses __COUNTER__, do something about it?
idf_build_set_property(COMPILE_OPTIONS "-DLV_ATTRIBUTE_TIMER_HANDLER=IRAM_ATTR" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DLV_ATTRIBUTE_TICK_INC=IRAM_ATTR" APPEND)

# For the fonts
idf_build_set_property(COMPILE_OPTIONS "-DLV_LVGL_H_INCLUDE_SIMPLE" APPEND)

project(suntransit)
