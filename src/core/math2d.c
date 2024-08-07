#include "headers.h"
#include "gfx.h"

#include "math2d.h"

Matrix IDENTITY_MATRIX = {
    .m = {
        {1.0, 0.0, 0.0, 0.0},
        {0.0, 1.0, 0.0, 0.0},
        {0.0, 0.0, 1.0, 0.0},
        {0.0, 0.0, 0.0, 1.0}
    }
};

float lerp_angle_deg(float a, float b, float t)
{
    t = RANGE(t,0.0,1.0);
    float dif = calc_angle_dif(a, b);
    float l = a + dif*t;
    // return l;
    return normalize_angle_deg(l);
}

float lerp(float a, float b, float t)
{
    t = RANGE(t,0.0,1.0);
    float r = (1.0-t)*a+(t*b);
    return r;
}

Vector2f lerp2f(Vector2f* a, Vector2f* b, float t)
{
    t = RANGE(t,0.0,1.0);
    float rx = lerp(a->x,b->x,t);
    float ry = lerp(a->y,b->y,t);

    Vector2f r = {rx,ry};
    return r;
}

Vector3f lerp3f(Vector3f* a, Vector3f* b, float t)
{
    t = RANGE(t,0.0,1.0);
    float rx = lerp(a->x,b->x,t);
    float ry = lerp(a->y,b->y,t);
    float rz = lerp(a->z,b->z,t);

    Vector3f r = {rx,ry,rz};
    return r;
}

float exp_decay(float a, float b, float decay, float dt)
{
    return b + (a - b)*exp(-decay*dt);
}

Vector2f exp_decay2f(Vector2f a, Vector2f b, float decay, float dt)
{
    float rx = exp_decay(a.x, b.x, decay, dt);
    float ry = exp_decay(a.y, b.y, decay, dt);

    Vector2f r = {rx, ry};
    return r;
}

Vector3f exp_decay3f(Vector3f a, Vector3f b, float decay, float dt)
{
    float rx = exp_decay(a.x, b.x, decay, dt);
    float ry = exp_decay(a.y, b.y, decay, dt);
    float rz = exp_decay(a.z, b.z, decay, dt);

    Vector3f r = {rx, ry, rz};
    return r;
}

void ortho(Matrix* m, float left, float right, float bottom, float top, float znear, float zfar)
{
    memcpy(m,&IDENTITY_MATRIX,sizeof(Matrix));

    m->m[0][0] = 2.0f/(right-left);
    m->m[1][1] = 2.0f/(top-bottom);
    m->m[2][2] = -2.0f/(zfar-znear);
    m->m[0][3] = -(right+left) / (right - left);
    m->m[1][3] = -(top+bottom) / (top-bottom);
    m->m[3][2] = (zfar+znear) / (zfar-znear);
}

void get_model_transform(Vector3f* pos, Vector3f* rotation, Vector3f* scale, Matrix* model)
{
    Matrix scale_trans            = {0};
    Matrix rotation_trans         = {0};
    Matrix translate_trans        = {0};

    get_scale_transform(&scale_trans, scale);
    get_rotation_transform(&rotation_trans, rotation);
    get_translate_transform(&translate_trans, pos);

    memcpy(model,&IDENTITY_MATRIX,sizeof(Matrix));

    dot_product_mat(*model, translate_trans, model);
    dot_product_mat(*model, rotation_trans,  model);
    dot_product_mat(*model, scale_trans,     model);

}

float normalize_angle_deg(float angle)
{
    if(angle >= 0 && angle <= 360.0) return angle;

    while(angle < 0)
    {
        angle += 360.0;
    }
    int a = angle/360.0;
    angle -= (360.0*a);
    return angle;
}

float calc_angle_dif(float angle1, float angle2)
{
    float dif = angle2 - angle1;
    float adif = ABS(dif);

    if(adif > 180.0)
    {
        if(dif < 0.0)
        {
            return (360.0+dif);
        }
        else
        {
            return -(angle1 + (360.0-angle2));
        }
    }
    return dif;
}

float calc_angle_deg(float x0, float y0, float x1, float y1)
{
    return DEG(calc_angle_rad(x0,y0,x1,y1));
}

float calc_angle_rad(float x0, float y0, float x1, float y1)
{

    // printf("%.2f, %.2f   %.2f, %.2f", x0, y0, x1, y1);
    y0 = 10000.0-y0;
    y1 = 10000.0-y1;
    // printf("--->    %.2f, %.2f   %.2f, %.2f\n", x0, y0, x1, y1);
    bool xeq = FEQ(x0, x1);
    bool yeq = FEQ(y0, y1);

    if(xeq && yeq)
    {
        // printf("ret: %d\n",__LINE__);
        return 0.0f;
    }

    if(xeq)
    {
        if(y1 > y0)
        {
            // printf("ret: %d\n",__LINE__);
            return PI_OVER_2;
        }
        else
        {
            // printf("ret: %d\n",__LINE__);
            return PI_OVER_2*3;
        }
    }
    else if(yeq)
    {
        if(x1 > x0)
        {
            // printf("ret: %d\n",__LINE__);
            return 0;
        }
        else
        {
            // printf("ret: %d\n",__LINE__);
            return PI;
        }
    }
    else
    {
        if(y1 > y0)
        {
            float opp = y1-y0;
            float adj = x1-x0;
            float a = atanf(opp/adj);
            if(x1 > x0)
            {
                // printf("opp: %.2f, adj: %.2f\n", opp, adj);
                // printf("ret: %d\n",__LINE__);
                return a;
            }
            // printf("ret: %d\n",__LINE__);
            return PI+a;
        }

        float opp = x1-x0;
        float adj = y1-y0;
        float a = atanf(opp/adj);
        if(x1 > x0)
        {
            // printf("opp: %.2f, adj: %.2f\n", opp, adj);
            // printf("ret: %d\n",__LINE__);
            return PI_OVER_2*3-a;
        }
        // printf("ret: %d\n",__LINE__);
        return PI_OVER_2*3-a;
    }
}

float dist_squared(float x0, float y0, float x1, float y1)
{
    Vector2f s = {
        x1 - x0,
        y1 - y0
    };

    float d_sq = (s.x * s.x) + (s.y * s.y);
    return d_sq;
}

float dist(float x0, float y0, float x1, float y1)
{
    if(x0 == x1 && y0 == y1) return 0.0;

    float d = sqrt(dist_squared(x0,y0,x1,y1));
    return d;
}

float dist3f_squared(float x0, float y0, float z0, float x1, float y1, float z1)
{
    Vector3f s = {
        x1 - x0,
        y1 - y0,
        z1 - z0
    };

    float d_sq = (s.x * s.x) + (s.y * s.y) + (s.z * s.z);
    return d_sq;
}

float dist3f(float x0, float y0, float z0, float x1, float y1, float z1)
{
    if(x0 == x1 && y0 == y1 && z0 == z1) return 0.0;

    float d = sqrt(dist3f_squared(x0,y0,z0,x1,y1,z1));
    return d;
}

static float Q_rsqrt(float number)
{
	union {
		float    f;
		uint32_t i;
	} conv = { .f = number };
	conv.i  = 0x5f3759df - (conv.i >> 1);
	conv.f *= 1.5F - (number * 0.5F * conv.f * conv.f);
	return conv.f;
}

float magn_fast(Vector2f v)
{
    return (0.96*v.x + 0.40*v.y);
}

float magn2f(float x, float y)
{
    return sqrt(x * x + y*y);
}

float magn(Vector3f v)
{
    return sqrt(v.x * v.x + v.y*v.y + v.z*v.z);
}

void normalize(Vector2f* v)
{
    float magn_squared = v->x*v->x + v->y*v->y;
    float r = Q_rsqrt(magn_squared);

    v->x *= r;
    v->y *= r;
}

void normalize3f(Vector3f* v)
{
    float magn_squared = v->x*v->x + v->y*v->y + v->z*v->z;
    float r = Q_rsqrt(magn_squared);

    v->x *= r;
    v->y *= r;
    v->z *= r;
}

void print_matrix(Matrix* mat)
{
    printf("Matrix:\n");
    for(int i = 0; i < 4; ++i)
    {
        printf("[ %f %f %f %f]"
                ,mat->m[i][0]
                ,mat->m[i][1]
                ,mat->m[i][2]
                ,mat->m[i][3]
              );
        printf("\n");
    }
}

void dot_product_mat(Matrix a, Matrix b, Matrix* result)
{
    for(int i = 0; i < 4; ++i)
    {
        for(int j = 0; j < 4; ++j)
        {
            result->m[i][j] =
                a.m[i][0] * b.m[0][j] + 
                a.m[i][1] * b.m[1][j] + 
                a.m[i][2] * b.m[2][j] + 
                a.m[i][3] * b.m[3][j];
        }
    }
}


void mult_v2f_mat4(Vector2f* v, Matrix* m, Vector2f* result)
{
    // assuming w is 1.0 for Vector
    result->x = (m->m[0][0] * v->x + m->m[0][1] * v->y + m->m[0][3]);
    result->y = (m->m[1][0] * v->x + m->m[1][1] * v->y + m->m[1][3]);
}

bool onSegment(Vector2f p, Vector2f q, Vector2f r)
{
    if (q.x <= MAX(p.x, r.x) && q.x >= MIN(p.x, r.x) &&
        q.y <= MAX(p.y, r.y) && q.y >= MIN(p.y, r.y))
       return true;
  
    return false;
}
  
int orientation(Vector2f p, Vector2f q, Vector2f r)
{
    int val = (q.y - p.y) * (r.x - q.x) -
              (q.x - p.x) * (r.y - q.y);
  
    if (val == 0) return 0;  // collinear
  
    return (val > 0)? 1: 2; // clock or counterclock wise
}
  
bool doIntersect(Vector2f p1, Vector2f q1, Vector2f p2, Vector2f q2)
{
    // Find the four orientations needed for general and
    // special cases
    int o1 = orientation(p1, q1, p2);
    int o2 = orientation(p1, q1, q2);
    int o3 = orientation(p2, q2, p1);
    int o4 = orientation(p2, q2, q1);
  
    // General case
    if (o1 != o2 && o3 != o4)
        return true;
  
    // Special Cases
    if (o1 == 0 && onSegment(p1, p2, q1)) return true;
    if (o2 == 0 && onSegment(p1, q2, q1)) return true;
    if (o3 == 0 && onSegment(p2, p1, q2)) return true;
    if (o4 == 0 && onSegment(p2, q1, q2)) return true;
  
    return false; // Doesn't fall in any of the above cases
}

bool is_line_seg_intersecting_rect(LineSeg* l, Rect* r)
{
    float x0 = r->x - r->w/2.0;
    float x1 = r->x + r->w/2.0;
    float y0 = r->y - r->h/2.0;
    float y1 = r->y + r->h/2.0;
    // rect segs
    LineSeg r00 = {{x0,y0},{x1,y0}}; // top
    LineSeg r01 = {{x0,y0},{x0,y1}}; // left
    LineSeg r10 = {{x1,y0},{x1,y1}}; // right
    LineSeg r11 = {{x0,y1},{x1,y1}}; // bottom

    bool b00 = doIntersect(r00.a,r00.b,l->a,l->b);//are_line_segs_intersecting(&r00,l);
    if(b00) return true;

    bool b01 = doIntersect(r01.a,r01.b,l->a,l->b);//are_line_segs_intersecting(&r01,l);
    if(b01) return true;

    bool b10 = doIntersect(r10.a,r10.b,l->a,l->b);//are_line_segs_intersecting(&r10,l);
    if(b10) return true;

    bool b11 = doIntersect(r11.a,r11.b,l->a,l->b);//are_line_segs_intersecting(&r11,l);
    if(b11) return true;

    return false;
}


bool are_line_segs_intersecting_rect(LineSeg* segs, int seg_count, Rect* check)
{
    for(int i = 0; i < seg_count; ++i)
    {
        if(is_line_seg_intersecting_rect(&segs[i], check))
        {
            return true;
        }
    }
    return false;
}


void rects_to_ling_segs(Rect* a, Rect* b, LineSeg out[5])
{
    float px = a->x;
    float py = a->y;
    float px0 = a->x - a->w/2.0;
    float px1 = a->x + a->w/2.0;
    float py0 = a->y - a->h/2.0;
    float py1 = a->y + a->h/2.0;

    float cx = b->x;
    float cy = b->y;
    float cx0 = b->x - b->w/2.0;
    float cx1 = b->x + b->w/2.0;
    float cy0 = b->y - b->h/2.0;
    float cy1 = b->y + b->h/2.0;

    LineSeg sc =  {{px, py},{cx, cy}};
    LineSeg s00 = {{px0, py0},{cx0, cy0}};
    LineSeg s01 = {{px1, py0},{cx1, cy0}};
    LineSeg s10 = {{px0, py1},{cx0, cy1}};
    LineSeg s11 = {{px1, py1},{cx1, cy1}};

    out[0] = sc;
    out[1] = s00;
    out[2] = s01;
    out[3] = s10;
    out[4] = s11;
}


bool are_rects_colliding(Rect* prior_s, Rect* curr_s, Rect* check)
{
    LineSeg segs[5] = {0};
    rects_to_ling_segs(prior_s, curr_s, segs);
    return are_line_segs_intersecting_rect(segs, 5, check);
}

// for centered rectangles
bool rectangles_colliding(Rect* a, Rect* b)
{
    int x1 = a->x-(a->w/2.0);
    int y1 = a->y-(a->h/2.0);
    int w1 = a->w;
    int h1 = a->h;

    int x2 = b->x-(b->w/2.0);
    int y2 = b->y-(b->h/2.0);
    int w2 = b->w;
    int h2 = b->h;

    // Check if one rectangle is on the left side of the other
    if (x1 + w1 <= x2 || x2 + w2 <= x1) {
        return false;
    }

    // Check if one rectangle is above the other
    if (y1 + h1 <= y2 || y2 + h2 <= y1) {
        return false;
    }

    // If neither of the above checks are true, then the
    // two rectangles must be overlapping.
    return true;
}


// for top left rectangles
bool rectangles_colliding2(Rect* a, Rect* b)
{
    int x1 = a->x;
    int y1 = a->y;
    int w1 = a->w;
    int h1 = a->h;

    int x2 = b->x;
    int y2 = b->y;
    int w2 = b->w;
    int h2 = b->h;

    // Check if one rectangle is on the left side of the other
    if (x1 + w1 <= x2 || x2 + w2 <= x1) {
        return false;
    }

    // Check if one rectangle is above the other
    if (y1 + h1 <= y2 || y2 + h2 <= y1) {
        return false;
    }

    // If neither of the above checks are true, then the
    // two rectangles must be overlapping.
    return true;
}

bool circles_colliding(Vector2f* p1, float r1, Vector2f* p2, float r2, float *distance)
{
    *distance = dist(p1->x,p1->y,p2->x,p2->y);
    float r  = (r1 + r2);

    return (*distance < r);
}

bool are_spheres_colliding(Vector4f* prior_s, Vector4f* curr_s, Vector4f* check)
{
    Vector4f p = {0};
    memcpy(&p, prior_s, sizeof(Vector4f));

    // get vector to progress by
    Vector3f v = {curr_s->x - p.x, curr_s->y - p.y, curr_s->z - p.z};
    normalize3f(&v);

    //printf("radius proj: %f, radius check: %f\n", p.w, check->w);

    for(;;)
    {
        //printf("p: %f, %f, %f\n", p.x, p.y, p.z);

        // check
        float d_check = dist3f(p.x, p.y, p.z, check->x, check->y, check->z);
        float r = p.w + check->w;

        //printf("p: %f %f %f, check: %f %f %f, dist: %f, total_r: %f\n",p.x,p.y,p.z,check->x, check->y, check->z, d_check, r);

        bool colliding = (d_check <= r);
        if(colliding)
        {
            //printf("collision!\n");
            return true;
        }

        float d_curr = dist3f(p.x, p.y, p.z, curr_s->x, curr_s->y, curr_s->z);
        if(d_curr < 0.01)
            break;

        float r_curr = p.w + curr_s->w;

        // update p sphere
        float factor = (d_curr >= prior_s->w ? prior_s->w : d_curr);
        //printf("factor: %f\n",factor);

        p.x += v.x * factor;
        p.y += v.y * factor;
        p.z += v.z * factor;
    }

    return false;
}

// rect is contained inside of limit
Vector2f limit_rect_pos(Rect* limit, Rect* rect)
{
    // printf("before: "); print_rect(rect);
    float lx0 = limit->x - limit->w/2.0;
    float lx1 = lx0 + limit->w;
    float ly0 = limit->y - limit->h/2.0;
    float ly1 = ly0 + limit->h;

    float px0 = rect->x - rect->w/2.0;
    float px1 = px0 + rect->w;
    float py0 = rect->y - rect->h/2.0;
    float py1 = py0 + rect->h;

    float _x = rect->x;
    float _y = rect->y;
    Vector2f adj = {0};

    if(px0 < lx0)
    {
        rect->x = lx0+rect->w/2.0;
        adj.x = rect->x - _x;
    }
    if(px1 > lx1)
    {
        rect->x = lx1-rect->w/2.0;
        adj.x = rect->x - _x;
    }
    if(py0 < ly0)
    {
        rect->y = ly0+rect->h/2.0;
        adj.y = rect->y - _y;
    }
    if(py1 > ly1)
    {
        rect->y = ly1-rect->h/2.0;
        adj.y = rect->y - _y;
    }
    // printf("after: "); print_rect(rect);

    // printf("adj: %.2f, %.2f\n", adj.x, adj.y);
    return adj;
}

// is 'a' fully inside of 'b'
bool is_rect_inside(Rect* a, Rect* b)
{
    float ax0 = a->x - a->w/2.0;
    float ax1 = ax0 + a->w;
    float ay0 = a->y - a->h/2.0;
    float ay1 = ay0 + a->h;

    float bx0 = b->x - b->w/2.0;
    float bx1 = bx0 + b->w;
    float by0 = b->y - b->h/2.0;
    float by1 = by0 + b->h;

    if(ax0 >= bx0 && ax1 <= bx1 && ay0 >= by0 && ay1 <= by1)
        return true;

    return false;
}


bool is_point_in_rect(Vector2f* p, Rect* rect)
{
    return (p->x >= rect->x && p->x <= rect->x + rect->w)
        && (p->y >= rect->y && p->y <= rect->y + rect->h);
}

float calc_radius_from_rect(Rect* r)
{
    float hw = r->w/2.0;
    float hh = r->h/2.0;

    return sqrt(hw*hw + hh*hh);
}

void get_scale_transform(Matrix* mat, Vector3f* scale)
{
    memset(mat,0,sizeof(Matrix));

    mat->m[0][0] = scale->x;
    mat->m[1][1] = scale->y;
    mat->m[2][2] = scale->z;
    mat->m[3][3] = 1.0f;
}

void get_rotation_transform(Matrix* mat, Vector3f* rotation)
{
    Matrix rx = {0};
    Matrix ry = {0};
    Matrix rz = {0};

    const float x = RAD(rotation->x);
    const float y = RAD(rotation->y);
    const float z = RAD(rotation->z);

    rx.m[0][0] = 1.0f;
    rx.m[1][1] = cosf(x);
    rx.m[1][2] = -sinf(x);
    rx.m[2][1] = sinf(x);
    rx.m[2][2] = cosf(x);
    rx.m[3][3] = 1.0f;

    ry.m[0][0] = cosf(y);
    ry.m[0][2] = -sinf(y); 
    ry.m[1][1] = 1.0f;  
    ry.m[2][0] = sinf(y);
    ry.m[2][2] = cosf(y);
    ry.m[3][3] = 1.0f;

    rz.m[0][0] = cosf(z);
    rz.m[0][1] = -sinf(z);
    rz.m[1][0] = sinf(z);
    rz.m[1][1] = cosf(z);
    rz.m[2][2] = 1.0f;
    rz.m[3][3] = 1.0f;

    memcpy(mat,&IDENTITY_MATRIX,sizeof(Matrix));

    dot_product_mat(*mat,rz,mat);
    dot_product_mat(*mat,ry,mat);
    dot_product_mat(*mat,rx,mat);
}

void get_translate_transform(Matrix* mat, Vector3f* position)
{
    memset(mat,0,sizeof(Matrix));

    mat->m[0][0] = 1.0f;
    mat->m[0][3] = position->x;
    mat->m[1][1] = 1.0f;
    mat->m[1][3] = position->y;
    mat->m[2][2] = 1.0f;
    mat->m[2][3] = position->z;
    mat->m[3][3] = 1.0f;
}

float vec_dot3(Vector3f a, Vector3f b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

float vec_dot(Vector2f a, Vector2f b)
{
    return (a.x * b.x) + (a.y * b.y);
}

Vector2f vec2(float x, float y)
{
    Vector2f v = {x,y};
    return v;
}

Vector3f vec3(float x, float y, float z)
{
    Vector3f v = {x,y,z};
    return v;
}

float get_angle_between_vectors_rad(Vector3f* a, Vector3f* b)
{
    float ma = magn(*a);
    float mb = magn(*b);

    if(ma == 0.0 || mb == 0.0)
        return 0.0;

    float d  = vec_dot3(*a,*b);
    
    float angle = acosf(d/(ma*mb));
    return angle;
}

// angle_deg: 0-360
int angle_sector(float angle_deg, int num_sectors)
{
    if(num_sectors <= 1) return 0;
    float sector_range = 360.0 / num_sectors;
    int sector = (angle_deg) / sector_range;
    // printf("Num sectors: %d (%.1f), angle: %.1f, sector: %d\n", num_sectors, sector_range, angle, sector);
    return sector;
}

Vector2f angle_sector_range(int num_sectors, int sector)
{
    Vector2f angle_range = {0};
    float sector_range = 360.0 / num_sectors;

    angle_range.x = sector * sector_range;
    angle_range.y = (sector+1) * sector_range;
    return angle_range;
}

float rangef(float arr[], int n, float* fmin, float* fmax)
{
    *fmin = arr[0];
    *fmax = arr[0];
    for(int i = 0; i < n; ++i)
    {
        if(arr[i] < *fmin)
            *fmin = arr[i];
        if(arr[i] > *fmax)
            *fmax = arr[i];
    }
    return (*fmax-*fmin);
}

void rotate_rect(Rect* in, float rotation, float rotation_x, float rotation_y, RectXY* out)
{
    float x0 = in->x - in->w/2.0;
    float x1 = in->x + in->w/2.0;
    float y0 = in->y - in->h/2.0;
    float y1 = in->y + in->h/2.0;
    // top left, top right, bottom right, bottom left
    float xcoords[4] = {x0, x1, x1, x0};
    float ycoords[4] = {y0, y0, y1, y1};

    float a = RAD(360-rotation);
    float xa = cos(a);
    float ya = sin(a);

    for(int i = 0; i < 4; ++i)
    {
        out->x[i] = (xa * (xcoords[i] - rotation_x) - ya * (ycoords[i] - rotation_y)) + rotation_x;
        out->y[i] = (ya * (xcoords[i] - rotation_x) - xa * (ycoords[i] - rotation_y)) + rotation_y;
    }
}

void rect_to_rectxy(Rect* in, RectXY* out)
{
    float x0 = in->x - in->w/2.0;
    float x1 = in->x + in->w/2.0;
    float y0 = in->y - in->h/2.0;
    float y1 = in->y + in->h/2.0;
    // top left, top right, bottom right, bottom left
    float xcoords[4] = {x0, x1, x1, x0};
    float ycoords[4] = {y0, y0, y1, y1};
    memcpy(out->x, xcoords, 4*sizeof(out->x[0]));
    memcpy(out->y, ycoords, 4*sizeof(out->y[0]));
}

void rectxy_to_rect(RectXY* in, Rect* out)
{
    float xmin,xmax,ymin,ymax;
    float xrange = rangef(in->x, 4, &xmin, &xmax);
    float yrange = rangef(in->y, 4, &ymin, &ymax);
    out->w = xrange;
    out->h = yrange;
    out->x = xmin + out->w/2.0;
    out->y = ymin + out->h/2.0;
}

bool rects_equal(Rect* r1, Rect* r2)
{
    if(!FEQ(r1->x, r2->x)) return false;
    if(!FEQ(r1->y, r2->y)) return false;
    if(!FEQ(r1->w, r2->w)) return false;
    if(!FEQ(r1->h, r2->h)) return false;
    return true;
}

float rect_tlx(Rect* r)
{
    return r->x - r->w/2.0;
}

float rect_tly(Rect* r)
{
    return r->y - r->h/2.0;
}

void print_rect(Rect* r)
{
    // printf("Rectangle (x,y,w,h): %.3f, %.3f, %.3f, %.3f\n", r->x, r->y, r->w, r->h);
    printf("Rectangle (x,y,w,h): %.8f, %.8f, %.8f, %.8f\n", r->x, r->y, r->w, r->h);
}

void print_rectxy(RectXY* r)
{
    printf("Rectangle XY:\n");

    printf("  x: ");
    for(int i = 0; i < 4; ++i)
        printf("%.2f%s", r->x[i], i != 3 ? ", " : "\n");

    printf("  y: ");
    for(int i = 0; i < 4; ++i)
        printf("%.2f%s", r->y[i], i != 3 ? ", " : "\n");
}

// only works with positives
float rand_float_between(float lower, float upper)
{
    float range = upper - lower;
    float value = ((float)rand()/(float)(RAND_MAX)) * range + lower;
    return value;

}

bool boxes_colliding(Box* b1, Box* b2)
{
    float hw = b1->w/2.0;
    float hl = b1->l/2.0;
    float hh = b1->h/2.0;

    Vector3f b1_min = {b1->x - hw, b1->y - hl, b1->z - hh};
    Vector3f b1_max = {b1->x + hw, b1->y + hl, b1->z + hh};

    float hw2 = b2->w/2.0;
    float hl2 = b2->l/2.0;
    float hh2 = b2->h/2.0;

    Vector3f b2_min = {b2->x - hw2, b2->y - hl2, b2->z - hh2};
    Vector3f b2_max = {b2->x + hw2, b2->y + hl2, b2->z + hh2};

    bool colliding = (
           b1_max.x >= b2_min.x &&
           b1_min.x <= b2_max.x &&
           b1_max.y >= b2_min.y &&
           b1_min.y <= b2_max.y &&
           b1_max.z >= b2_min.z &&
           b1_min.z <= b2_max.z);

    /*
    printf("b1_min: x: %5.2f, y: %5.2f, z: %5.2f\n", b1_min.x, b1_min.y, b1_min.z);
    printf("b1_max: x: %5.2f, y: %5.2f, z: %5.2f\n", b1_max.x, b1_max.y, b1_max.z);

    printf("b2_min: x: %5.2f, y: %5.2f, z: %5.2f\n", b2_min.x, b2_min.y, b2_min.z);
    printf("b2_max: x: %5.2f, y: %5.2f, z: %5.2f\n", b2_max.x, b2_max.y, b2_max.z);

    #define B(b) b ? "true" : "false"
    printf("%d) %s\n", __LINE__, B(b1_max.x >= b2_min.x));
    printf("%d) %s\n", __LINE__, B(b1_min.x <= b2_max.x));
    printf("%d) %s\n", __LINE__, B(b1_max.y >= b2_min.y));
    printf("%d) %s\n", __LINE__, B(b1_min.y <= b2_max.y));
    printf("%d) %s\n", __LINE__, B(b1_max.z >= b2_min.z));
    printf("%d) %s\n", __LINE__, B(b1_min.z <= b2_max.z));
    */

    return colliding;

}

void slrand(RndGen* rg, uint32_t seed)
{
    rg->lrand_next = seed;
}

uint32_t lrand(RndGen* rg)
{
    rg->lrand_next = rg->lrand_next * 1103515245 + 12345;
    return (uint32_t)(rg->lrand_next / 65536) % 32768;
}
