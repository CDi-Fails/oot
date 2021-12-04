#ifndef AXIS_VECTOR_H
#define AXIS_VECTOR_H

typedef struct {
    s16 x, y;
} Vec2s;

typedef struct {
    s32 x, y;
} Vec2i;

/*typedef struct {
    f32 x, y;
} Vec2f;*/

/*typedef struct {
    s16 x, y, z;
} Vec3s;*/

/*typedef struct {
    s32 x, y, z;
} Vec3i;*/

/*typedef struct {
    f32 x, y, z;
} Vec3f;*/

typedef struct {
    s16 x, y, z, w;
} Vec4s;

typedef struct {
    s32 x, y, z, w;
} Vec4i;

typedef struct {
    f32 x, y, z, w;
} Vec4f, QuatF;

#define QuatF_AddAssignment             Vec4f_AddAssignment
#define QuatF_SubAssignment             Vec4f_SubAssignment
#define QuatF_MultiplyAssignment        Vec4f_MultiplyAssignment
#define QuatF_DivideAssignment          Vec4f_DivideAssignment
#define QuatF_MultiplyAssignmentF       Vec4f_MultiplyAssignmentF
#define QuatF_DivideAssignmentF         Vec4f_DivideAssignmentF
#define QuatF_Add                       Vec4f_Add
#define QuatF_Sub                       Vec4f_Sub
#define QuatF_Multiply                  Vec4f_Multiply
#define QuatF_Divide                    Vec4f_Divide
#define QuatF_MultiplyF                 Vec4f_MultiplyF
#define QuatF_DivideF                   Vec4f_DivideF
#define QuatF_Dot                       Vec4f_Dot
#define QuatF_Cross                     Vec4f_Cross
#define QuatF_SquareMagnitude           Vec4f_SquareMagnitude
#define QuatF_SquareMagnitudePtr        Vec4f_SquareMagnitudePtr
#define QuatF_MagnitudePtr              Vec4f_MagnitudePtr
#define QuatF_NormalizeAssignment       Vec4f_NormalizeAssignment
#define QuatF_Normalize                 Vec4f_Normalize
#define QuatF_Distance                  Vec4f_Distance
#define QuatF_InverseAssignment         Vec4f_InverseAssignment
#define QuatF_Inverse                   Vec4f_Inverse

#define VEC3F_X(r, p, y)                ((r) * (sinf(DTOR(y))) * (cosf(DTOR(p))))
#define VEC3F_Y(r, p, y)                ((r) * (cosf(DTOR(y))) * (sinf(DTOR(p))))
#define VEC3F_Z(r, p, y)                ((r) * (cosf(DTOR(y))) * (cosf(DTOR(p))))

#define Vec2f_Right                     {1.0f, 0.0f};
#define Vec2f_Up                        {0.0f, 1.0f};
#define Vec2f_Left                      {-1.0f, 0.0f};
#define Vec2f_Down                      {0.0f, -1.0f};
#define Vec2f_Zero                      {0.0f, 0.0f};
#define Vec3f_Right                     {1.0f, 0.0f, 0.0f};
#define Vec3f_Up                        {0.0f, 1.0f, 0.0f};
#define Vec3f_Forward                   {0.0f, 0.0f, 1.0f};
#define Vec3f_Left                      {-1.0f, 0.0f, 0.0f};
#define Vec3f_Down                      {0.0f, -1.0f, 0.0f};
#define Vec3f_Backward                  {0.0f, 0.0f, -1.0f};
#define Vec3f_Zero                      {0.0f, 0.0f, 0.0f};
#define Vec4f_Right                     {1.0f, 0.0f, 0.0f, 0.0f};
#define Vec4f_Up                        {0.0f, 1.0f, 0.0f, 0.0f};
#define Vec4f_Forward                   {0.0f, 0.0f, 1.0f, 0.0f};
#define Vec4f_Identity                  {0.0f, 0.0f, 0.0f, 1.0f};
#define Vec4f_Left                      {-1.0f, 0.0f, 0.0f, 0.0f};
#define Vec4f_Down                      {0.0f, -1.0f, 0.0f, 0.0f};
#define Vec4f_Backward                  {0.0f, 0.0f, -1.0f, 0.0f};
#define Vec4f_Ndentity                  {0.0f, 0.0f, 0.0f, -1.0f};
#define Vec4f_Zero                      {0.0f, 0.0f, 0.0f, 0.0f};
#define Vec2i_Right                     {1, 0};
#define Vec2i_Up                        {0, 1};
#define Vec2i_Left                      {-1, 0};
#define Vec2i_Down                      {0, -1};
#define Vec2i_Zero                      {0, 0};
#define Vec3i_Right                     {1, 0, 0};
#define Vec3i_Up                        {0, 1, 0};
#define Vec3i_Forward                   {0, 0, 1};
#define Vec3i_Left                      {-1, 0, 0};
#define Vec3i_Down                      {0, -1, 0};
#define Vec3i_Backward                  {0, 0, -1};
#define Vec3i_Zero                      {0, 0, 0};
#define Vec4i_Right                     {1, 0, 0, 0};
#define Vec4i_Up                        {0, 1, 0, 0};
#define Vec4i_Forward                   {0, 0, 1, 0};
#define Vec4i_Identity                  {0, 0, 0, 1};
#define Vec4i_Left                      {-1, 0, 0, 0};
#define Vec4i_Down                      {0, -1, 0, 0};
#define Vec4i_Backward                  {0, 0, -1, 0};
#define Vec4i_Ndentity                  {0, 0, 0, -1};
#define Vec4i_Zero                      {0, 0, 0, 0};
#define Vec2s_Right                     {1, 0};
#define Vec2s_Up                        {0, 1};
#define Vec2s_Left                      {-1, 0};
#define Vec2s_Down                      {0, -1};
#define Vec2s_Zero                      {0, 0};
#define Vec3s_Right                     {1, 0, 0};
#define Vec3s_Up                        {0, 1, 0};
#define Vec3s_Forward                   {0, 0, 1};
#define Vec3s_Left                      {-1, 0, 0};
#define Vec3s_Down                      {0, -1, 0};
#define Vec3s_Backward                  {0, 0, -1};
#define Vec3s_Zero                      {0, 0, 0};
#define Vec4s_Right                     {1, 0, 0, 0};
#define Vec4s_Up                        {0, 1, 0, 0};
#define Vec4s_Forward                   {0, 0, 1, 0};
#define Vec4s_Identity                  {0, 0, 0, 1};
#define Vec4s_Left                      {-1, 0, 0, 0};
#define Vec4s_Down                      {0, -1, 0, 0};
#define Vec4s_Backward                  {0, 0, -1, 0};
#define Vec4s_Ndentity                  {0, 0, 0, -1};
#define Vec4s_Zero                      {0, 0, 0, 0};
#define QuatF_Right                     Vec4f_Right   
#define QuatF_Up                        Vec4f_Up      
#define QuatF_Forward                   Vec4f_Forward 
#define QuatF_Identity                  Vec4f_Identity
#define QuatF_Left                      Vec4f_Left    
#define QuatF_Down                      Vec4f_Down    
#define QuatF_Backward                  Vec4f_Backward
#define QuatF_Ndentity                  Vec4f_Ndentity
#define QuatF_Zero                      Vec4f_Zero 

#endif
