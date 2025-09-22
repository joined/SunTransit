file(GLOB_RECURSE FONT_SRCS ui/fonts/*.c)

# Select LCD implementation based on target
if(IDF_TARGET STREQUAL "esp32")
    set(LCD_IMPL "lcd_3248s035c.cpp")
elseif(IDF_TARGET STREQUAL "esp32s3")
    set(LCD_IMPL "lcd_8048s070c.cpp")
else()
    message(FATAL_ERROR "Unsupported target: ${IDF_TARGET}")
endif()

idf_component_register(
    SRCS "nvs_engine.cpp" "utils.cpp" "bvg_api_client.cpp" ${LCD_IMPL} "main.cpp" "http_server.cpp" "ui/ui.cpp" "time.cpp" ${FONT_SRCS}
    INCLUDE_DIRS "." "ui"
    PRIV_REQUIRES esp_app_format esp_http_client esp_http_server esp_timer esp_wifi json nvs_flash spiffs vfs wifi_provisioning lwip
)

set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

if(DEFINED ENV{GITHUB_ACTIONS})
    message(STATUS "Running on GitHub Actions. Assuming that web UI is already built.")

    spiffs_create_partition_image(www ${ROOT_DIR}/frontend_dist FLASH_IN_PROJECT)
else()
    find_program(PNPM_EXECUTABLE pnpm)
    if(NOT PNPM_EXECUTABLE)
        message(FATAL_ERROR "`pnpm` not found, please install it.")
    endif()

    # TODO This breaks if the dev server is running, fix it
    # TODO This is executed also when running `idf.py flash`, it shouldn't
    ExternalProject_Add(
        build_frontend
        PREFIX ${CMAKE_BINARY_DIR}/build_frontend
        SOURCE_DIR ${ROOT_DIR}
        CONFIGURE_COMMAND ${PNPM_EXECUTABLE} i
        USES_TERMINAL_BUILD true
        BUILD_COMMAND ${PNPM_EXECUTABLE} build
        INSTALL_COMMAND ""
        # TODO Ideally we would be building the frontend only if any files changed since the last build,
        # but that seems to be impossible with ExternalProject_Add.
        BUILD_ALWAYS ON
        BUILD_IN_SOURCE TRUE
    )

    spiffs_create_partition_image(www ${ROOT_DIR}/frontend_dist FLASH_IN_PROJECT DEPENDS build_frontend)
endif()
