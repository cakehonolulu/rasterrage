#include "vega6502.h"

void __time_critical_func(draw_pixels)(scanvideo_scanline_buffer_t *buffer) {
    int scanline = scanvideo_scanline_number(buffer->scanline_id);
    uint16_t *p = (uint16_t *)(buffer->data + 1);  // Start at the third word of the buffer

    // Calculate the starting pixel position in the pixel_framebuffer
    uint16_t *pixel_ptr = pixel_framebuffer[1] + scanvideo_scanline_number(buffer->scanline_id) * 160;

    // Insert them into the buffer using the appropriate VGA format
    buffer->data[0] = COMPOSABLE_RAW_RUN | (SCREEN_WIDTH + 1 - 3 << 16);
    
    
    buffer->data[SCREEN_WIDTH / 2 + 2] = 0x0000u | (COMPOSABLE_EOL_ALIGN << 16);
    buffer->data_used = SCREEN_WIDTH / 2 + 2;

    memcpy(p, pixel_ptr, sizeof(uint16_t) * SCREEN_WIDTH);

    uint32_t first = buffer->data[0];
    uint32_t second = buffer->data[1];
    buffer->data[0] = (first & 0x0000ffffu) | ((second & 0x0000ffffu) << 16);
    buffer->data[1] = (second & 0xffff0000u) | ((first & 0xffff0000u) >> 16);
    buffer->status = SCANLINE_OK;
}

void core1_func() {
    while (true) {
        scanvideo_scanline_buffer_t *scanline_buffer = scanvideo_begin_scanline_generation(true);
        draw_pixels(scanline_buffer);
        scanvideo_end_scanline_generation(scanline_buffer);
    }
}
