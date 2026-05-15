#include "math.h"
#include "raylib.h"
#include <cstdint>
#include <vector>

#define ZERO 0.00
#define ONE 1.00
#define aspect_ratio 16.0 / 9.0
#define screen_width 800
#define screen_height ((int)(screen_width / (aspect_ratio)))

#define black_pixel create_pixel_(ZERO, ZERO, ZERO, ONE)
#define create_pixel(r, g, b) create_pixel_(r, g, b, ONE)
#define focal_length 1.00
#define screen_center create_vec(0, 0, -focal_length)
#define camera create_vec(0, 0, 0)
#define screen_upper_left                                                      \
  subtract(screen_center, create_vec(screen_width, screen_height, 0))
#define pixel00_loc add(screen_upper_left, multiply(create_vec(1, 1, 0), 0.5))
#define subtract(a, b) add(a, multiply(b, -1.00))
#define divide(a, b) multiply(a, 1 / b);

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} pixel;

typedef struct {
  double x;
  double y;
  double z;
} vec3;

typedef struct {
  vec3 origin;
  vec3 direction;
} ray;

vec3 add_constant(vec3 v, double c) {
  return {.x = v.x + c, .y = v.y + c, .z = v.z + c};
};

vec3 multiply(vec3 v, double a) {
  return {.x = v.x * a, .y = v.y * a, .z = v.z * a};
};
vec3 add(vec3 a, vec3 b) {
  return {.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z};
};
vec3 scale_and_add(vec3 a, vec3 b, double c) { return add(a, multiply(b, c)); };
vec3 ray_at_t(ray r, double t) {
  return scale_and_add(r.origin, r.direction, t);
};

pixel create_pixel_(double r, double g, double b, double a) {
  return (pixel){.r = (uint8_t)(r * 255),
                 .g = (uint8_t)(g * 255),
                 .b = (uint8_t)(b * 255),
                 .a = (uint8_t)(a * 255)};
};
ray create_ray(vec3 origin, vec3 direction) {
  return (ray){.origin = origin, .direction = direction};
}

vec3 create_vec(double x, double y, double z) {
  return (vec3){.x = x, .y = y, .z = z};
};

double length_squared(vec3 v) { return (v.x * v.x + v.y * v.y + v.z * v.z); };
double length(vec3 v) { return sqrt(length_squared(v)); };
vec3 unit_vector(vec3 v) { return divide(v, length(v)); };

pixel color_ray(ray r) {
  vec3 unit_dir = unit_vector(r.direction);
  double a = 0.5 * (unit_dir.y + 1.0);
  vec3 smthg = add(multiply(create_vec(1.0, 1.0, 1.0), (1.0 - a)),
                   multiply(create_vec(0.5, 0.7, 1.0), a));
  return create_pixel(smthg.x, smthg.y, smthg.z);
};

int main() {
  InitWindow(screen_width, screen_height, "M1 Software Renderer");
  SetTargetFPS(60);

  std::vector<pixel> cpuBuffer(screen_width * screen_height,
                               black_pixel); // Initialize with Black (RGBA)

  Texture2D displayTexture =
      LoadTextureFromImage({.data = cpuBuffer.data(),
                            .width = screen_width,
                            .height = screen_height,
                            .mipmaps = 1,
                            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8});

  while (!WindowShouldClose()) {
    // main loop
    for (int y = 0; y < screen_height; y++) {
      for (int x = 0; x < screen_width; x++) {
        uint8_t r = (uint8_t)(x + GetTime() * 100);
        uint8_t g = (uint8_t)(y + GetTime() * 100);
        // Pack into 0xRRGGBBAA (Little Endian depends on target, Raylib expects
        // RGBA)
        vec3 pixel_center =
            add(pixel00_loc, (vec3){.x = (double)x, .y = (double)y, .z = 0.00});
        vec3 ray_direction = subtract(pixel_center, camera);
        ray pixel_ray = create_ray(camera, ray_direction);
        cpuBuffer[y * screen_width + x] = color_ray(pixel_ray);
      }
    }

    // Transfer the CPU RAM buffer to the GPU VRAM texture
    // .data gives pointer to the actual location of data inside the vector
    UpdateTexture(displayTexture, cpuBuffer.data());

    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawTexture(displayTexture, 0, 0, WHITE);
    DrawFPS(10, 10);
    EndDrawing();
  }

  UnloadTexture(displayTexture);
  CloseWindow();

  return 0;
}
