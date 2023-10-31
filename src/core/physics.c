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

    if(m1 == m2)
    {
        // shortcut, simply switch velocities
        Vector2f t1 = {phys1->vel.x, phys1->vel.y};
        memcpy(&phys1->vel,&phys2->vel,sizeof(Vector2f));
        memcpy(&phys2->vel,&t1,sizeof(Vector2f));
    }
    else
    {
        float m_total = m1+m2;

        float u1x = phys1->vel.x;
        float u2x = phys2->vel.x;

        float u2y = phys2->vel.y;
        float u1y = phys1->vel.y;

        float v1x = u1x*(m1-m2)/m_total + u2x*(2*m2/m_total);
        float v2x = u1x*(2*m1)/m_total + u2x*((m2-m1)/m_total);

        float v1y = u1y*(m1-m2)/m_total + u2y*(2*m2/m_total);
        float v2y = u1y*(2*m1)/m_total + u2y*((m2-m1)/m_total);

        phys1->vel.x = v1x;
        phys1->vel.y = v1y;

        phys2->vel.x = v2x;
        phys2->vel.y = v2y;
    }
}

float phys_get_friction_rate(float friction_factor, float dt)
{
    return 1.0-pow(2.0, -friction_factor/dt);
}

void phys_apply_friction_x(Physics* phys, float rate)
{
    phys->vel.x += (0.0 - phys->vel.x)*rate;
    if(ABS(phys->vel.x) < 1.0) phys->vel.x = 0.0;
}

void phys_apply_friction_y(Physics* phys, float rate)
{
    phys->vel.y += (0.0 - phys->vel.y)*rate;
    if(ABS(phys->vel.y) < 1.0) phys->vel.y = 0.0;
}

void phys_apply_friction_2(Physics* phys, float rate)
{
    phys_apply_friction_x(phys,rate);
    phys_apply_friction_y(phys,rate);
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
