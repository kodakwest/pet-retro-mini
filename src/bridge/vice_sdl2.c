#include "vice_bridge.h"

/*
 * Placeholder boundary for the real VICE SDL2 video adapter.
 *
 * The launcher talks only to vice_bridge.c. When libxpet is linked in, this
 * file should hold the VICE-facing raster callback glue and copy VICE's pixel
 * buffer into the SDL texture exposed by vice_bridge_frame().
 */
int vice_sdl2_video_driver_link_anchor(void)
{
    return VICE_BRIDGE_SCREEN_W + VICE_BRIDGE_SCREEN_H;
}
