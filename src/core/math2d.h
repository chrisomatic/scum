#pragma once

#include <stdbool.h>

#define PI         3.14159265358f
#define PI_OVER_2  1.57079632679f
#define SQRT2OVER2 0.707106781187

#define RAD(x) (((x) * PI) / 180.0f)
#define DEG(x) (((x) * 180.0f) / PI)
#define ABS(x) ((x) < 0 ? -1*(x) : (x))

#define RAND_RANGE(min, max) ((rand()%((max)-(min)+1)) + (min))
#define RAND_FLOAT(min, max) ((float)((double)rand()/(double)(RAND_MAX/((max)-(min))))+(min))
#define RAND8() (rand()%256)
#define RAND_PN() (rand()%2 == 0 ? -1 : 1)

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define RANGE(x,y,z) MIN(MAX((x),(y)),(z))
#define BETWEEN(x,y,z) (x > y && x < z)

#define SQ(x) ((x)*(x))
#define FEQ(a, b) (ABS(a-b) <= 0.00001f)
#define FEQ0(a) (ABS(a) <= 0.00001f)

#define BOUND(a,l,u)    MAX(MIN(a,u),l)
#define IS_BIT_SET(x,b) (((x) & (b)) == (b))

#define IS_RECT_EMPTY(r) (FEQ((r)->w,0.0) || FEQ((r)->h,0.0))

typedef struct
{
    float x,y;
} Vector2f;

typedef struct
{
    int x,y;
} Vector2i;

typedef struct
{
    Vector2f a;
    Vector2f b;
} LineSeg;

typedef struct
{
    float x,y,z;
} Vector3f;

typedef struct
{
    float x,y,z,w;
} Vector4f;

typedef struct
{
    Vector2f position;
    Vector2f tex_coord;
} Vertex;

typedef struct
{
    float x,y,z; // center
    float w,l,h; // dimensions
} Box;

typedef struct
{
    Vector2f pos;
    Vector3f color;
} LinePoint;

typedef struct
{
    float m[4][4];
} Matrix;

typedef struct
{
    float x,y;  //center
    float w,h;
} Rect;

// top left, top right, bottom right, bottom left
typedef struct
{
    float x[4];
    float y[4];
} RectXY;

typedef enum
{
    TL,
    TR,
    BR,
    BL
} RXYpoint;

typedef struct
{
    uint64_t lrand_next;
} RndGen;

extern Matrix IDENTITY_MATRIX;

void get_model_transform(Vector3f* pos, Vector3f* rotation, Vector3f* scale, Matrix* model);
void ortho(Matrix* m, float left, float right, float bottom, float top, float znear, float zfar);
float normalize_angle_deg(float angle);
float calc_angle_dif(float angle1, float angle2);
float calc_angle_deg(float x0, float y0, float x1, float y1);
float calc_angle_rad(float x0, float y0, float x1, float y1);
float dist_squared(float x0, float y0, float x1, float y1);
float dist(float x0, float y0, float x1, float y1);
float dist3f_squared(float x0, float y0, float z0, float x1, float y1, float z1);
float dist3f(float x0, float y0, float z0, float x1, float y1, float z1);
float magn(Vector3f v);
float magn2f(float x, float y);
float magn_fast(Vector2f v);
void normalize(Vector2f* v);
void normalize3f(Vector3f* v);
void get_scale_transform(Matrix* mat, Vector3f* scale);
void get_rotation_transform(Matrix* mat, Vector3f* rotation);
void get_translate_transform(Matrix* mat, Vector3f* position);
void dot_product_mat(Matrix a, Matrix b, Matrix* result);
void mult_mat4(Matrix* m1, Matrix* m2, Matrix* result);
void print_matrix(Matrix* mat);

Vector2f vec2(float x, float y);
Vector3f vec3(float x, float y, float z);

float vec_dot(Vector2f a, Vector2f b);
Vector2f vec_cross(Vector2f a, Vector2f b);

float get_angle_between_vectors_rad(Vector3f* a, Vector3f* b);


bool is_line_seg_intersecting_rect(LineSeg* l, Rect* r);
bool are_line_segs_intersecting_rect(LineSeg* segs, int seg_count, Rect* check);
bool are_line_segs_intersecting(LineSeg* l1, LineSeg* l2);
void rects_to_ling_segs(Rect* a, Rect* b, LineSeg out[5]);
bool are_rects_colliding(Rect* prior_s, Rect* curr_s, Rect* check);
bool rectangles_colliding(Rect* a, Rect* b);
bool rectangles_colliding2(Rect* a, Rect* b);
bool circles_colliding(Vector2f* p1, float r1, Vector2f* p2, float r2, float *distance);
bool are_spheres_colliding(Vector4f* prior_s, Vector4f* curr_s, Vector4f* check);
Vector2f limit_rect_pos(Rect* limit, Rect* rect);
bool is_rect_inside(Rect* a, Rect* b);
bool is_point_in_rect(Vector2f* p, Rect* rect);
float calc_radius_from_rect(Rect* r);

bool boxes_colliding(Box* b1, Box* b2);

int angle_sector(float angle, int num_sectors);
Vector2f angle_sector_range(int num_sectors, int sector);
float rangef(float arr[], int n, float* fmin, float* fmax);

void rotate_rect(Rect* in, float rotation, float rotation_x, float rotation_y, RectXY* out);
void rect_to_rectxy(Rect* in, RectXY* out);
void rectxy_to_rect(RectXY* in, Rect* out);
bool rects_equal(Rect* r1, Rect* r2);
float rect_tlx(Rect* r);
float rect_tly(Rect* r);
void print_rect(Rect* r);
void print_rectxy(RectXY* r);

float lerp_angle_deg(float a, float b, float t);
float lerp(float a, float b, float t);
Vector2f lerp2f(Vector2f* a, Vector2f* b, float t);
Vector3f lerp3f(Vector3f* a, Vector3f* b, float t);

float exp_decay(float a, float b, float decay, float dt);

Vector2f exp_decay2f(Vector2f a, Vector2f b, float decay, float dt);
Vector3f exp_decay3f(Vector3f a, Vector3f b, float decay, float dt);

// only works with positives
float rand_float_between(float lower, float upper);

// random numbers
void slrand(RndGen* rg, uint32_t seed);
uint32_t lrand(RndGen* rg);
