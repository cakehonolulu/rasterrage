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

static semaphore_t video_initted;
static semaphore_t drawing_semaphore;

Vec3 cube_vertices[8] = {
    {CENTER_X - 20 * SCALE_FACTOR, CENTER_Y - 20 * SCALE_FACTOR, -20 * SCALE_FACTOR},   // Vertex 0
    {CENTER_X + 20 * SCALE_FACTOR, CENTER_Y - 20 * SCALE_FACTOR, -20 * SCALE_FACTOR},   // Vertex 1
    {CENTER_X + 20 * SCALE_FACTOR, CENTER_Y + 20 * SCALE_FACTOR, -20 * SCALE_FACTOR},   // Vertex 2
    {CENTER_X - 20 * SCALE_FACTOR, CENTER_Y + 20 * SCALE_FACTOR, -20 * SCALE_FACTOR},   // Vertex 3
    {CENTER_X - 20 * SCALE_FACTOR, CENTER_Y - 20 * SCALE_FACTOR,  20 * SCALE_FACTOR},   // Vertex 4
    {CENTER_X + 20 * SCALE_FACTOR, CENTER_Y - 20 * SCALE_FACTOR,  20 * SCALE_FACTOR},   // Vertex 5
    {CENTER_X + 20 * SCALE_FACTOR, CENTER_Y + 20 * SCALE_FACTOR,  20 * SCALE_FACTOR},   // Vertex 6
    {CENTER_X - 20 * SCALE_FACTOR, CENTER_Y + 20 * SCALE_FACTOR,  20 * SCALE_FACTOR}    // Vertex 7
};

// Rotation angles
float rotation_x = 0.02f;
float rotation_y = 0.03f;
float rotation_z = 0.04f;

uint8_t example_sprite_data[] = {
    0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,
    0xF,0xF,0xF,0xF,0xF,1,1,1,1,1,0xF,0xF,0xF,0xF,0xF,0xF,
	0xF,0xF,0xF,0xF,1,1,1,1,1,1,1,1,1,0xF,0xF,0xF,
	0xF,0xF,0xF,0xF,4,4,4,9,9,4,9,0xF,0xF,0xF,0xF,0xF,
	0xF,0xF,0xF,4,9,4,9,9,9,4,9,9,9,0xF,0xF,0xF,
	0xF,0xF,0xF,4,9,4,4,9,9,9,4,9,9,9,0xF,0xF,
	0xF,0xF,0xF,4,4,9,9,9,9,4,4,4,4,0xF,0xF,0xF,
	0xF,0xF,0xF,0xF,0xF,9,9,9,9,9,9,9,0xF,0xF,0xF,0xF,
	0xF,0xF,0xF,0xF,4,4,1,4,4,4,0xF,0xF,0xF,0xF,0xF,0xF,
	0xF,0xF,0xF,4,4,4,1,4,4,1,4,4,4,0xF,0xF,0xF,
	0xF,0xF,4,4,4,4,1,1,1,1,4,4,4,4,0xF,0xF,
	0xF,0xF,9,9,4,1,9,1,1,9,1,4,9,9,0xF,0xF,
	0xF,0xF,9,9,9,1,1,1,1,1,1,9,9,9,0xF,0xF,
	0xF,0xF,9,9,1,1,1,1,1,1,1,1,9,9,0xF,0xF,
	0xF,0xF,0xF,0xF,1,1,1,0xF,0xF,1,1,1,0xF,0xF,0xF,0xF,
	0xF,0xF,0xF,4,4,4,0xF,0xF,0xF,0xF,4,4,4,0xF,0xF,0xF,
	0xF,0xF,4,4,4,4,0xF,0xF,0xF,0xF,4,4,4,4,0xF,0xF,
    0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,
};

// Initialize a sprite
Sprite example_sprite = {
    .x = 120,
    .y = 50,
    .width = 16,
    .height = 18,
    .pixels = example_sprite_data,
    .scale = 1.0f,
    .rotation = 0.0f,
    .flip_x = 0,
    .flip_y = 0
};

// Timing variables
absolute_time_t last_time;
float frame_time = 0.0f;
float fps = 0.0f;

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
        //swap_framebuffers();
        scanvideo_end_scanline_generation(scanline_buffer);
    }
}

int main(void) {
    //set_sys_clock_khz(200000, true);
    stdio_init_all();
    // Initialize the pixel framebuffer with black pixels
    memset(pixel_framebuffer, 0, sizeof(pixel_framebuffer));

    write_string_to_framebuffer("BitBang'd VGA - RP2040 Base Clock\n", 0x0F);
    write_string_to_framebuffer("Running at 320x240 60Hz\n", 0x0F);
    write_string_to_framebuffer("40x40 text mode - 16 colours\n", 0x0F);
#if 0
    // Draw a white line from top-left to bottom-right corner
    draw_line(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT, color_palette[0x01]); // Red

    // Draw a red line from top-right to bottom-left corner
    draw_line(SCREEN_WIDTH - 1, 0, 0, SCREEN_HEIGHT, color_palette[0x0F]); // White

    draw_filled_triangle(50, 50, 150, 50, 100, 150, color_palette[0x02]); // White triangle

    draw_triangle(52, 52, 152, 52, 102, 152, color_palette[0x04]); // White triangle

    // Draw a white rectangle
    draw_rectangle(200, 50, 280, 100, color_palette[0x0F]); // White rectangle

    // Draw a red square
    draw_square(200, 120, 40, color_palette[0x01]); // Red square

    // Draw a filled blue rectangle
    draw_filled_rectangle(50, 180, 130, 230, color_palette[0x04]); // Blue filled rectangle

    // Draw a filled green square
    draw_filled_square(200, 180, 40, color_palette[0x02]); // Green filled square
#endif
    // initialize video and interrupts on core 0
    scanvideo_setup(&vga_mode);
    scanvideo_timing_enable(true);
    sem_release(&video_initted);

    // create a semaphore to be posted when video init is complete
    sem_init(&video_initted, 0, 1);

    // launch all the video on core 1
    multicore_launch_core1(core1_func);

    // wait for initialization of video to be complete
    //sem_acquire_blocking(&video_initted);

    float rotation = 0.0f;

        set_clear_colour(PICO_SCANVIDEO_PIXEL_FROM_RGB5(148, 148, 148));

    // Ball properties
    float ball_x = SCREEN_WIDTH / 2.0f;
    float ball_y = SCREEN_HEIGHT / 2.0f;
    float ball_vx = 1.0f;  // Horizontal velocity
    float ball_vy = 0.0f;  // Initial vertical velocity
    float gravity = 0.2f;  // Constant downward force
    float damping = 0.95f;  // Damping factor to simulate energy loss on bounce

        
    while (true) {
        // Wait for core 1 to signal it is done with the current framebuffer
        // Signal core 1 that the framebuffer has been swapped

        absolute_time_t start_time = get_absolute_time();
        /*cur_x = 0; cur_y = 0;
        write_string_to_framebuffer("BitBang VGA - RP2040\n", 0x0F);
        write_string_to_framebuffer("   3D Raster Demo\n", 0x0F);
        write_string_to_framebuffer("Some text\n", 0x0F);
        write_string_to_framebuffer("Some more text\n", 0x0F);
        write_string_to_framebuffer("Some more clipping text\n", 0x0F);
        cur_x = 0; cur_y = 13;
        write_string_to_framebuffer("    Made with ", 0x0F);
        write_string_to_framebuffer("<3\n", 0x01);
        write_string_to_framebuffer("  by @cakehonolulu\n", 0x0F);

        // Optionally, draw the filled cube
        rotate_cube(cube_vertices, rotation_x, rotation_y, rotation_z);

        // Project and draw the cube
        draw_filled_cube(cube_vertices);

        // Draw the wireframe cube
        draw_wireframe_cube(cube_vertices);

        draw_sprite(&example_sprite);
        update_sprite_scale(&example_sprite);

        absolute_time_t now = get_absolute_time();
        frame_time = (float)absolute_time_diff_us(start_time, now) / 1000.0f; // Convert to milliseconds
        fps = 1000.0f / frame_time;

        // Display frame time and FPS
        char info[40];
        snprintf(info, sizeof(info), "%.2f ms", frame_time);
        cur_x = 0; cur_y = 17;
        write_smallstring_to_framebuffer(info, 0x0F);
        snprintf(info, sizeof(info), "%.2f FPS", fps);
        cur_x = 0; cur_y = 19;
        write_smallstring_to_framebuffer(info, 0x0F);
        cur_x = 0; cur_y = 0;*/

        // Update ball position based on velocity
        ball_x += ball_vx;
        ball_y += ball_vy;

        // Apply gravity to the vertical velocity
        ball_vy += gravity;

        // Check for collisions with screen edges and bounce
        if (ball_x - 25 < 0 || ball_x + 25 > SCREEN_WIDTH) {
            ball_vx *= -1; // Invert horizontal velocity
        }
        if (ball_y + 25 > SCREEN_HEIGHT) {
            ball_y = SCREEN_HEIGHT - 25; // Ensure the ball doesn't go below the screen
            ball_vy = -ball_vy * damping; // Reverse and reduce vertical velocity to simulate bounce

            // Stop bouncing when the velocity is too small
            if (fabs(ball_vy) < 1.0f) {
                ball_vy = 0.0f;
            }
        }

        draw_perspective_grid(PERSPECTIVE_FACTOR);

        int shadow_offset_x = 5;  // Horizontal offset of the shadow
        int shadow_offset_y = 5;  // Vertical offset of the shadow
        int radius = 20;  // Shadow size (same as the sphere)
        uint16_t shadow_color = 0x8410; // Gray color for shadow (R=128, G=128, B=128)

        // Draw the shadow as a 2D filled circle
        draw_filled_circle((int)ball_x + shadow_offset_x, (int)ball_y + shadow_offset_y, radius, shadow_color);

        // Draw the sphere with the updated position
        float tilt_angle = 10.5f * (PI / 180.0f); 
        float right_tilt_angle = -75.5f * (PI / 180.0f);  // Tilt around Y-axis (tilt to the right)
        draw_sphere(8, 16, radius, (int)ball_x, (int)ball_y, rotation, tilt_angle, right_tilt_angle);

        absolute_time_t now = get_absolute_time();
        frame_time = (float)absolute_time_diff_us(start_time, now) / 1000.0f; // Convert to milliseconds
        fps = 1000.0f / frame_time;

        char info[40];
        /*snprintf(info, sizeof(info), "%.2f ms", frame_time);
        cur_x = 0; cur_y = 0;
        write_smallstring_to_framebuffer(info, 0x0F);*/
        snprintf(info, sizeof(info), "%.2f FPS", fps);
        cur_x = 0; cur_y = 0;
        write_smallstring_to_framebuffer(info, 0x0F);
        snprintf(info, sizeof(info), "%d tri", drawn_triangles);
        cur_x = 0; cur_y = 2;
        write_smallstring_to_framebuffer(info, 0x0F);
        snprintf(info, sizeof(info), "@cakehonolulu");
        cur_x = 7; cur_y = 0;
        write_string_to_framebuffer(info, 0x0F);


        // Increment rotation angle
        rotation += 0.05f; 

        if (rotation > 2 * PI) {
            rotation -= 2 * PI;
        }


        scanvideo_wait_for_vblank();   // Wait for vertical blanking
        swap_framebuffers(); 

        //clear_framebuffer(current_framebuffer); // Clear the new "dirty" framebuffer
    }
}
