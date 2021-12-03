#include "ultra64.h"
#include "global.h"

/*
#define VEC3F_X(r, p, y) ((r) * (sinf(DEG_TO_RAD(y))) * (cosf(DEG_TO_RAD(p))))
#define VEC3F_Y(r, p, y) ((r) * (cosf(DEG_TO_RAD(y))) * (sinf(DEG_TO_RAD(p))))
#define VEC3F_Z(r, p, y) ((r) * (cosf(DEG_TO_RAD(y))) * (cosf(DEG_TO_RAD(p))))
*/

// Return a point on a 3D Cylinder given yaw and pitch in euler degrees.
Vec3f Vec3f_PointOnCylinder(f32 radius, f32 yaw, f32 pitch) {
    Vec3f return_value;
    return_value.x = VEC3F_X(radius, yaw, 0);
    return_value.y = VEC3F_Y(radius, 0, pitch);
    return_value.z = VEC3F_Z(radius, yaw, 0);
    return return_value;
}

// Return a point on a 3D Sphere given yaw and pitch in euler degrees.
Vec3f Vec3f_PointOnSphere(f32 radius, f32 yaw, f32 pitch) {
    Vec3f return_value;
    return_value.x = VEC3F_X(radius, yaw, pitch);
    return_value.y = VEC3F_Y(radius, yaw, pitch);
    return_value.z = VEC3F_Z(radius, yaw, pitch);
    return return_value;
}