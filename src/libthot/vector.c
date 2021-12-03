#include "ultra64.h"
#include "global.h"

void Vec3f_ToVec3iAssignment(Vec3f* lhs, Vec3i* rhs) {
    rhs->x = lhs->x;
    rhs->y = lhs->y;
    rhs->z = lhs->z;
}

void Vec3f_ToVec3sAssignment(Vec3f* lhs, Vec3s* rhs) {
    rhs->x = lhs->x;
    rhs->y = lhs->y;
    rhs->z = lhs->z;
}

void Vec3i_ToVec3fAssignment(Vec3i* lhs, Vec3f* rhs) {
    rhs->x = lhs->x;
    rhs->y = lhs->y;
    rhs->z = lhs->z;
}

void Vec3i_ToVec3sAssignment(Vec3i* lhs, Vec3s* rhs) {
    rhs->x = lhs->x;
    rhs->y = lhs->y;
    rhs->z = lhs->z;
}

void Vec3s_ToVec3fAssignment(Vec3s* lhs, Vec3f* rhs) {
    rhs->x = lhs->x;
    rhs->y = lhs->y;
    rhs->z = lhs->z;
}

void Vec3s_ToVec3iAssignment(Vec3s* lhs, Vec3i* rhs) {
    rhs->x = lhs->x;
    rhs->y = lhs->y;
    rhs->z = lhs->z;
}

// Return a new Vec3 (float) from Vec3 (integer)
Vec3f Vec3f_FromVec3i(Vec3i* lhs) {
    Vec3f return_value;
    Vec3i_ToVec3fAssignment(lhs, &return_value);
    return return_value;
}

// Return a new Vec3 (float) from Vec3 (short)
Vec3f Vec3f_FromVec3s(Vec3s* lhs) {
    Vec3f return_value;
    Vec3s_ToVec3fAssignment(lhs, &return_value);
    return return_value;
}

// Return a new Vec3 (integer) from Vec3 (float)
Vec3i Vec3i_FromVec3f(Vec3f* lhs) {
    Vec3i return_value;
    Vec3f_ToVec3iAssignment(lhs, &return_value);
    return return_value;
}

// Return a new Vec3 (integer) from Vec3 (short)
Vec3i Vec3i_FromVec3s(Vec3s* lhs) {
    Vec3i return_value;
    Vec3s_ToVec3iAssignment(lhs, &return_value);
    return return_value;
}

// Return a new Vec3 (short) from Vec3 (float)
Vec3s Vec3s_FromVec3f(Vec3f* lhs) {
    Vec3s return_value;
    Vec3f_ToVec3sAssignment(lhs, &return_value);
    return return_value;
}

// Return a new Vec3 (short) from Vec3 (integer)
Vec3s Vec3s_FromVec3i(Vec3s* lhs) {
    Vec3s return_value;
    Vec3i_ToVec3sAssignment(lhs, &return_value);
    return return_value;
}