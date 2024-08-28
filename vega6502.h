#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "pico.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/sync.h"

#include "primitive.h"
#include "framebuffer.h"

#define vga_mode vga_mode_160x120_60

void __time_critical_func(draw_pixels)(scanvideo_scanline_buffer_t *buffer);
void core1_func();
