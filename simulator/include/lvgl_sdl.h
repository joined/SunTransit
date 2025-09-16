#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

#include "lvgl.h"
#include "ui.hpp"

#define SDL_HOR_RES 480
#define SDL_VER_RES 320
#define SDL_ZOOM 3

namespace LVGL_SDL {

static void init() {
    setvbuf(stdout, NULL, _IONBF, 0); // Disable stdout buffering, so that logs appear instantly

    lv_init();

    lv_display_t *disp = lv_sdl_window_create(SDL_HOR_RES, SDL_VER_RES);
    lv_sdl_window_set_zoom(disp, SDL_ZOOM);

    lv_indev_t *mouse_indev = lv_sdl_mouse_create();
    lv_indev_set_display(mouse_indev, disp);
}
} // namespace LVGL_SDL
