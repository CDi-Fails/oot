#ifndef AXIS_COLOR_H
#define AXIS_COLOR_H

/*typedef struct {
    u8 r, g, b;
} Color_RGB8;*/

/*typedef struct {
    u8 r, g, b, a;
} Color_RGBA8;*/

/*typedef union {
    struct {
        u8 r, g, b, a;
    };
    u32 rgba;
} Color_RGBA8_u32;*/

/*typedef struct {
    f32 r, g, b, a;
} Color_RGBAf;*/

/*typedef union {
    struct {
        u16 r : 5;
        u16 g : 5;
        u16 b : 5;
        u16 a : 1;
    };
    u16 rgba;
} Color_RGBA16, Color_RGBA_5551;*/

typedef union {
    struct {
        u8 r, g, b;
    };
    u32 rgb;
} Color_RGB24;

typedef union {
    struct {
        u8 r, g, b, a;
    };
    u32 rgba;
} Color_RGBA32;

typedef struct {
    f32 h, s, v;
} Color_HSVf;

#endif
