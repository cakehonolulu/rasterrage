#include "primitive.h"
#include "framebuffer.h"

#include "rasterrage.h"

static semaphore_t video_initted;
static semaphore_t drawing_semaphore;

// Timing variables
absolute_time_t last_time;
float frame_time = 0.0f;
float fps = 0.0f;

#define SCALE_FACTOR 1.5

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

int main(void) {
    //set_sys_clock_khz(200000, true);
    stdio_init_all();
    // Initialize the pixel framebuffer with black pixels
    memset(pixel_framebuffer, 0, sizeof(pixel_framebuffer));

    // initialize video and interrupts on core 0
    scanvideo_setup(&vga_mode);
    scanvideo_timing_enable(true);
    sem_release(&video_initted);

    // create a semaphore to be posted when video init is complete
    sem_init(&video_initted, 0, 1);

    // launch all the video on core 1
    multicore_launch_core1(core1_func);
  
    while (true) {
        absolute_time_t start_time = get_absolute_time();
        
        cur_x = 0; cur_y = 0;
        write_string_to_framebuffer("BitBang VGA - RP2040\n", 0x0F);
        write_string_to_framebuffer("   3D Raster Demo\n", 0x0F);
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
        cur_x = 0; cur_y = 0;

        scanvideo_wait_for_vblank();   // Wait for vertical blanking
        swap_framebuffers(); 
    }
}
