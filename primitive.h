#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include "framebuffer.h"

#define PI 3.14159265358979323846

#define CENTER_X 80
#define CENTER_Y 60
#define SCALE_FACTOR 1.5

#define GRID_COLOR 0xF81F // Pink color (R=255, G=0, B=255)
#define GRID_SPACING 10    // Distance between lines in the grid
#define GRID_ROWS 10       // Number of rows in the grid
#define GRID_COLUMNS 12    // Number of columns in the grid
#define PERSPECTIVE_FACTOR 0.05f  // How much the grid "zooms" as it gets closer

typedef struct {
    float x, y;
} Vec2;


typedef struct {
    float x, y, z;
} Vec3;

// Structure to hold face information
typedef struct {
    int indices[4];
    float depth; // Average Z value or any other depth metric
} Face;

typedef struct {
    int x, y; // Sprite position
    int width, height; // Sprite dimensions
    uint8_t *pixels; // Pointer to sprite pixel data
    float scale; // Scaling factor
    float rotation; // Rotation angle in radians
    int flip_x, flip_y; // Flip flags
} Sprite;

extern int drawn_triangles;

void swap(int *a, int *b);
void cross_product(float x1, float y1, float z1, float x2, float y2, float z2, float *nx, float *ny, float *nz);
void rotate_x_(float angle, float *x, float *y, float *z);
void rotate_y_(float angle, float *x, float *y, float *z);
void rotate_z_(float angle, float *x, float *y, float *z);
void rotate_x(Vec3 *p, float angle);
void rotate_y(Vec3 *p, float angle);
void rotate_z(Vec3 *p, float angle);
void translate_point(Vec3 *p, float tx, float ty);
Vec2 project_3d_to_2d(Vec3 p);
void sort_faces_by_depth(Face faces[], int count);
void draw_line(int x0, int y0, int x1, int y1, uint16_t color);
void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);
void draw_filled_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);
void draw_rectangle(int x0, int y0, int x1, int y1, uint16_t color);
void draw_square(int x, int y, int size, uint16_t color);
void draw_filled_rectangle(int x0, int y0, int x1, int y1, uint16_t color);
void draw_filled_square(int x, int y, int size, uint16_t color);
void draw_wireframe_cube(Vec3 *vertices);
void draw_circle_with_lines(int centerX, int centerY, int radius);
void draw_sprite(const Sprite *sprite);
void update_sprite_scale(Sprite *sprite);
void update_sprite(Sprite *sprite);
void draw_filled_cube(Vec3 *vertices);
void rotate_cube(Vec3 *vertices, float angle_x, float angle_y, float angle_z);
void move_cube(Vec3 *vertices, float move_x, float move_y);
void draw_perspective_grid(float perspective_factor);
void draw_wavy_gradient();
void draw_filled_circle(int cx, int cy, int radius, uint16_t color);
void draw_sphere(int stacks, int sectors, int radius, int cx, int cy, float rotation, float tilt, float right_tilt);
