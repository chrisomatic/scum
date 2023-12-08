#include "headers.h"
#include "physics.h"

bool phys_collision_circles(Physics* phys1, Physics* phys2, CollisionInfo* ci)
{
    Vector2f p1 = {CPOSX(*phys1), CPOSY(*phys1)};
    Vector2f p2 = {CPOSX(*phys2), CPOSY(*phys2)};

    float d = dist(p1.x,p1.y,p2.x,p2.y);
    float r  = (phys1->radius + phys2->radius);

    bool colliding = (d < r);

    if(colliding)
    {
        // get overlap
        float overlap = (r - d);

        Vector2f o1 = {p2.x - p1.x, p2.y - p1.y};
        normalize(&o1);

        if(o1.x == 0.0 && o1.y == 0.0)
        {
            o1.x = RAND_FLOAT(0.0,1.0);
            o1.y = 1.0 - o1.x;
        }

        o1.x *= overlap;
        o1.y *= overlap;

        ci->overlap.x = o1.x;
        ci->overlap.y = o1.y;
    }
    return colliding;
}

void phys_collision_correct(Physics* phys1, Physics* phys2, CollisionInfo* ci)
{
    // collision correct
    float cratio = phys2->mass/(phys1->mass+phys2->mass);

    if(phys2->mass > 100.0*phys1->mass)
        cratio = 1.0; // mass is sufficiently large to not move at all

    phys1->pos.x -= ci->overlap.x*cratio;
    phys1->pos.y -= ci->overlap.y*cratio;

    phys2->pos.x += ci->overlap.x*(1.0-cratio);
    phys2->pos.y += ci->overlap.y*(1.0-cratio);

    // collision response
    // update velocities based on elastic collision

    float m1 = phys1->mass;
    float m2 = phys2->mass;

    Vector2f x1 = {CPOSX(*phys1), CPOSY(*phys1)};
    Vector2f x2 = {CPOSX(*phys2), CPOSY(*phys2)};

    Vector2f v1 = {phys1->vel.x, phys1->vel.y};
    Vector2f v2 = {phys2->vel.x, phys2->vel.y};

    float m_total = m1+m2;

    Vector2f v1mv2 = {v1.x - v2.x, v1.y - v2.y};
    Vector2f x1mx2 = {x1.x - x2.x, x1.y - x2.y};

    Vector2f v2mv1 = {v2.x - v1.x, v2.y - v1.y};
    Vector2f x2mx1 = {x2.x - x1.x, x2.y - x1.y};

    float dot1 = vec_dot(v1mv2,x1mx2);
    float dot2 = vec_dot(v2mv1,x2mx1);

    float mag1   = magn2f(x1mx2.x, x1mx2.y);
    float mag1sq = mag1*mag1;

    float mag2   = magn2f(x2mx1.x, x2mx1.y);
    float mag2sq = mag2*mag2;

    float fac1 = (2*m2/m_total) * (dot1/mag1sq);
    float fac2 = (2*m1/m_total) * (dot2/mag2sq);

    float v1x = v1.x - fac1*x1mx2.x;
    float v1y = v1.y - fac1*x1mx2.y;

    float v2x = v2.x - fac2*x2mx1.x;
    float v2y = v2.y - fac2*x2mx1.y;

    phys1->vel.x = v1x;
    phys1->vel.y = v1y;

    phys2->vel.x = v2x;
    phys2->vel.y = v2y;
}

void phys_apply_gravity(Physics* phys, float gravity_factor, float dt)
{
    if(phys->floating)
        return;

    if(phys->pos.z > 0.0)
        phys->vel.z -= (gravity_factor*600.0*dt);

    phys->pos.z += dt*phys->vel.z;

    if(phys->pos.z <= 0.0)
    {
        phys->pos.z = 0.0;
        phys->vel.z = -phys->elasticity*phys->vel.z;
        if(ABS(phys->vel.z) < 10.0)
            phys->vel.z = 0.0;
    }
}


void phys_add_circular_time(Physics* phys, float dt)
{
    phys->circular_dt += dt;
    phys->circular_dt = fmod(phys->circular_dt,CIRCULAR_DT_MAX);
}

void phys_apply_friction(Physics* phys, float friction, float dt)
{
    Vector2f f = {-phys->vel.x, -phys->vel.y};
    normalize(&f);

    float vel_magn = magn(phys->vel);
    float applied_friction = MIN(vel_magn,friction);

    f.x *= applied_friction;
    f.y *= applied_friction;

    phys->vel.x += f.x;
    phys->vel.y += f.y;
    
    if(ABS(phys->vel.x) < 1.0) phys->vel.x = 0.0;
    if(ABS(phys->vel.y) < 1.0) phys->vel.y = 0.0;
}

void phys_apply_friction_x(Physics* phys, float friction, float dt)
{
    Vector2f f = {-phys->vel.x, -phys->vel.y};
    normalize(&f);

    float vel_magn = magn(phys->vel);
    float applied_friction = MIN(vel_magn,friction);

    f.x *= applied_friction;
    phys->vel.x += f.x;
    
    if(ABS(phys->vel.x) < 1.0) phys->vel.x = 0.0;
}

void phys_apply_friction_y(Physics* phys, float friction, float dt)
{
    Vector2f f = {-phys->vel.x, -phys->vel.y};
    normalize(&f);

    float vel_magn = magn(phys->vel);
    float applied_friction = MIN(vel_magn,friction);

    f.y *= applied_friction;
    phys->vel.y += f.y;
    
    if(ABS(phys->vel.y) < 1.0) phys->vel.y = 0.0;
}
