#include "headers.h"
#include "physics.h"

bool phys_collision_circles(Physics* phys1, Physics* phys2, CollisionInfo* ci)
{
    Vector2f p1 = {phys1->collision_rect.x, phys1->collision_rect.y};
    Vector2f p2 = {phys2->collision_rect.x, phys2->collision_rect.y};

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

        //printf("Colliding! Overlap: %f %f\n", o1.x, o1.y);
    }

    return colliding;
}

void phys_collision_correct(Physics* phys1, Physics* phys2, CollisionInfo* ci)
{
    // collision correct
    float cratio = phys2->mass/(phys1->mass+phys2->mass);

    if(phys2->mass > 100.0*phys1->mass)
        cratio = 1.0; // mass is sufficiently large to not move at all

    float c1x = phys1->collision_rect.x - ci->overlap.x*cratio;
    float c1y = phys1->collision_rect.y - ci->overlap.y*cratio;

    float c2x = phys2->collision_rect.x + ci->overlap.x*(1.01 - cratio);
    float c2y = phys2->collision_rect.y + ci->overlap.y*(1.01 - cratio);

    phys_set_collision_pos(phys1,c1x, c1y);
    phys_set_collision_pos(phys2,c2x, c2y);

    // collision response
    // update velocities based on elastic collision

    float m1 = phys1->mass;
    float m2 = phys2->mass;

    Vector2f x1 = {phys1->collision_rect.x, phys1->collision_rect.y};
    Vector2f x2 = {phys2->collision_rect.x, phys2->collision_rect.y};

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

    phys1->vel.x = phys2->elasticity == 0.0 ? 0.0 : v1x;
    phys1->vel.y = phys2->elasticity == 0.0 ? 0.0 : v1y;

    phys2->vel.x = phys1->elasticity == 0.0 ? 0.0 : v2x;
    phys2->vel.y = phys1->elasticity == 0.0 ? 0.0 : v2y;

    // update
    phys_calc_collision_rect(phys1);
    phys_calc_collision_rect(phys2);
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

void phys_calc_collision_rect(Physics* phys)
{
    if(!phys) return;

    memcpy(&phys->collision_rect_prior, &phys->collision_rect, sizeof(Rect));

    bool horizontal = (phys->rotation_deg == 0.0 || phys->rotation_deg == 180.0) && phys->crawling;

    phys->collision_rect.w = horizontal ? phys->length : phys->width;
    phys->collision_rect.h = horizontal ? phys->width  : phys->length;


    float vrh = horizontal ? phys->vr.w : phys->vr.h;

    phys->collision_rect.w *= phys->scale;
    phys->collision_rect.h *= phys->scale;
    vrh *= phys->scale;

    phys->collision_rect.x = phys->pos.x;
    phys->collision_rect.y = phys->pos.y - vrh*0.10;

    if(phys->floating)
    {
        phys->collision_rect.y -= phys->pos.z/2.0;
    }

    if(phys->crawling)
    {
        phys->collision_rect.y = phys->pos.y - vrh/2.0;
    }
}

void phys_set_collision_pos(Physics* phys, float new_x, float new_y)
{
    float dx = phys->pos.x - phys->collision_rect.x;
    float dy = phys->pos.y - phys->collision_rect.y;

    phys->collision_rect.x = new_x;
    phys->collision_rect.y = new_y;

    phys->pos.x = phys->collision_rect.x + dx;
    phys->pos.y = phys->collision_rect.y + dy;
}

void phys_print(Physics* phys)
{
    printf("    phys: {\n");
    printf("        pos:           %.2f %.2f %.2f\n", phys->pos.x, phys->pos.y, phys->pos.z);
    printf("        prior_pos:     %.2f %.2f %.2f\n", phys->prior_pos.x, phys->prior_pos.y, phys->prior_pos.z);
    printf("        vel:           %.2f %.2f %.2f\n", phys->vel.x, phys->vel.y, phys->vel.z);
    printf("        prior_vel:     %.2f %.2f %.2f\n", phys->prior_vel.x, phys->prior_vel.y, phys->prior_vel.z);
    printf("        max_vel:       %.2f\n",phys->max_velocity);
    printf("        base_friction: %.2f\n",phys->base_friction);
    printf("        speed:         %.2f\n",phys->speed);
    printf("        speed_factor:  %.2f\n",phys->speed_factor);
    printf("        radius:        %.2f\n",phys->radius);
    printf("        mass:          %.2f\n",phys->mass);
    printf("        width:         %.2f\n",phys->width);
    printf("        length:        %.2f\n",phys->length);
    printf("        height:        %.2f\n",phys->height);
    printf("        elasticity:    %.2f\n",phys->elasticity);
    printf("        rotation_deg:  %.2f\n",phys->rotation_deg);
    printf("        hp:            %d\n",phys->hp);
    printf("        hp_max:        %d\n",phys->hp_max);
    printf("        circular_dt:   %.2f\n",phys->circular_dt);
    printf("        dead:          %s\n", phys->dead ? "true" : "false");
    printf("        amorphous:     %s\n", phys->amorphous ? "true" : "false");
    printf("        ethereal:      %s\n", phys->ethereal ? "true" : "false");
    printf("        falling:       %s\n", phys->falling ? "true" : "false");
    printf("        floating:      %s\n", phys->floating ? "true" : "false");
    printf("        crawling:      %s\n", phys->crawling ? "true" : "false");
    printf("        status_effects: {\n");
    for(int i = 0; i < phys->status_effects_count; ++i)
    {
        printf("      [%d]\n", i);
        printf("          type:           %d\n", phys->status_effects[i].type);
        printf("          lifetime:       %.2f\n", phys->status_effects[i].lifetime);
        printf("          lifetime_max:   %.2f\n", phys->status_effects[i].lifetime_max);
        printf("          periodic:       %s\n", phys->status_effects[i].periodic ? "true" : "false");
        printf("          period:         %.2f\n", phys->status_effects[i].period);
        printf("          periods_passed: %d\n", phys->status_effects[i].periods_passed);
        printf("          applied:        %s\n", phys->status_effects[i].applied ? "true" : "false");
    }
    printf("        };\n");

    Rect* r = &phys->collision_rect;
    Rect* rp = &phys->collision_rect_prior;
    printf("        collision_rect:       [%.2f %.2f %.2f %.2f]\n",r->x, r->y, r->w, r->h);
    printf("        collision_rect_prior: [%.2f %.2f %.2f %.2f]\n",rp->x, rp->y, rp->w, rp->h);
    printf("  };\n");
}

void phys_print_dimensions(Physics* phys)
{
    printf("  radius:  %.2f\n", phys->radius);
    printf("  height:  %.2f\n", phys->height);
    printf("  width:   %.2f\n", phys->width);
    printf("  length:  %.2f\n", phys->length);
}
