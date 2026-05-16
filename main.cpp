#include "assert.h"
#include "math.h"
#include "raylib.h"
#include "stdlib.h"
#include <cstdint>
#include <vector>

#define game_name "YAS - Yet Another Shooter"

#define ZERO 0.00
#define ONE 1.00
#define aspect_ratio 16.0 / 9.0
#define screen_width 800
#define screen_height ((int)(screen_width / (aspect_ratio)))

#define black_pixel create_pixel_(ZERO, ZERO, ZERO, ONE)
#define create_pixel(r, g, b) create_pixel_(r, g, b, ONE)
#define focal_length 200.00
#define screen_center create_vec(0, 0, -focal_length)
#define camera create_vec(0, 0, 0)
#define screen_upper_left                                                      \
  subtract(screen_center, create_vec(screen_width / 2, screen_height / 2, 0))
#define pixel00_loc add(screen_upper_left, multiply(create_vec(1, 1, 0), 0.5))
#define subtract(a, b) add(a, multiply(b, -1.00))
#define divide(a, b) multiply(a, 1 / b)

typedef struct {
  size_t count;
  size_t capacity;
} Header;

#define ARR_INIT_CAPACITY 1
#define arr_push(arr, x)                                                       \
  do {                                                                         \
    if (arr == NULL) {                                                         \
      Header *header =                                                         \
          (Header *)malloc(sizeof(*arr) * ARR_INIT_CAPACITY + sizeof(Header)); \
      header->count = 0;                                                       \
      header->capacity = ARR_INIT_CAPACITY;                                    \
      arr = (decltype(arr))(header + 1);                                       \
    }                                                                          \
    Header *header = (Header *)(arr) - 1;                                      \
    if (header->count >= header->capacity) {                                   \
      header->capacity *= 2;                                                   \
      header = (Header *)realloc(header, sizeof(*arr) * header->capacity +     \
                                             sizeof(Header));                  \
      arr = (decltype(arr))(header + 1);                                       \
    }                                                                          \
    (arr)[header->count++] = (x);                                              \
  } while (0)

#define arr_len(arr) ((Header *)(arr) - 1)->count

#define arr_free(arr) free((Header *)(arr) - 1)

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

typedef struct {
  vec3 center;
  double radius;
} sphere;

typedef struct {
  vec3 point;
  vec3 normal; // assumed unit vector
  double t;
  bool front_face;
} hit_record;

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

void print_vec(vec3 v) {
  printf("v.x -> %f\n"
         "v.y -> %f\n"
         "v.z -> %f\n\n",
         v.x, v.y, v.z);
};

sphere create_sphere(vec3 center, double radius) {
  return ((sphere){.center = center, .radius = radius});
};

double length_squared(vec3 v) { return (v.x * v.x + v.y * v.y + v.z * v.z); };
double length(vec3 v) { return sqrt(length_squared(v)); };
vec3 unit_vector(vec3 v) { return divide(v, length(v)); };
double dot(vec3 a, vec3 b) { return (a.x * b.x + a.y * b.y + a.z * b.z); };

sphere *spheres = NULL;
sphere *player_1 = NULL;
sphere *player_2 = NULL;

// normals always assumed to be facing outwards so we can use to determine
// if the ray hit the object from inside the surface or from outside the surface
bool is_front_face(ray r, vec3 outward_normal) {
  return (dot(r.direction, outward_normal) < 0);
};

bool hit_sphere(ray r, const sphere *s, double t_min, double t_max,
                hit_record *rec) {
  vec3 oc = subtract(s->center, r.origin);
  double a = length_squared(r.direction);
  double h = dot(r.direction, oc);
  double c = length_squared(oc) - s->radius * s->radius;
  double discr = h * h - a * c;
  if (discr < 0) {
    return false;
  }
  double sqrtd = sqrt(discr);
  double root = (h - sqrtd) / a;
  if (root <= t_min || t_max <= root) {
    root = (h + sqrtd) / a;
    if (root <= t_min || t_max <= root) {
      return false;
    }
  }
  rec->t = root;
  rec->point = ray_at_t(r, rec->t);

  // dividing by radius will give us a unit vector for the normal
  vec3 outward_normal = divide(subtract(rec->point, s->center), s->radius);
  rec->front_face = is_front_face(r, outward_normal);
  rec->normal = rec->front_face ? outward_normal : multiply(outward_normal, -1);
  return true;
};

bool world_hit(ray r, hit_record *rec, double t_min, double t_max) {
  bool hit_anything = false;
  double closest_so_far = t_max;

  for (int i = 0; i < arr_len(spheres); i++) {
    if (hit_sphere(r, &spheres[i], t_min, closest_so_far, rec)) {
      hit_anything = true;
      closest_so_far = rec->t;
    }
  }
  return hit_anything;
};

pixel color_ray(ray r) {
  hit_record rec;
  if (world_hit(r, &rec, 0, INFINITY)) {
    vec3 smthg_2 = multiply(add_constant(rec.normal, 1.0), 0.5);
    return create_pixel(smthg_2.x, smthg_2.y, smthg_2.z);
  }

  vec3 unit_dir = unit_vector(r.direction);
  double a = 0.5 * (unit_dir.y + 1.0);
  vec3 smthg = add(multiply(create_vec(1.0, 1.0, 1.0), (1.0 - a)),
                   multiply(create_vec(0.5, 0.7, 1.0), a));
  return create_pixel(smthg.x, smthg.y, smthg.z);
};

int main() {
  arr_push(spheres, create_sphere(create_vec(100.00, 0, -180.00), 50));
  arr_push(spheres, create_sphere(create_vec(-100.00, 0, -200.00), 50));
  player_1 = &spheres[0];
  player_2 = &spheres[1];
  InitWindow(screen_width, screen_height, game_name);
  SetTargetFPS(20);

  std::vector<pixel> cpuBuffer(screen_width * screen_height,
                               black_pixel); // Initialize with Black (RGBA)

  Texture2D displayTexture =
      LoadTextureFromImage({.data = cpuBuffer.data(),
                            .width = screen_width,
                            .height = screen_height,
                            .mipmaps = 1,
                            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8});

  while (!WindowShouldClose()) {
    vec3 *p1c = &player_1->center;
    if (IsKeyDown(KEY_RIGHT))
      p1c->x -= 10.0f;
    if (IsKeyDown(KEY_LEFT))
      p1c->x += 10.0f;
    if (IsKeyDown(KEY_UP))
      p1c->y -= 10.0f;
    if (IsKeyDown(KEY_DOWN))
      p1c->y += 10.0f;
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

  arr_free(spheres);

  return 0;
}
