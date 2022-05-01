#ifndef TINY_MANDELBROT_CONFIG_HPP
#define TINY_MANDELBROT_CONFIG_HPP

#include <stdint.h>

// 0: use float32
// 1: use Q8.24 fixed point
#define MANDEL_ENABLE_FIXED_POINT (1)

// 0: a * b = (int64_t)a * b >> 24
// 1: split multiplication into upper/lower words
#define MANDEL_ENABLE_MULT_SPLIT  (1)

// 0: raster scan
// 1: calculate only pixels near edges
#define MANDEL_ENABLE_BORDER_SCAN (1)

// 0: always redraw entire screen
// 1: aedraw only new areas
#define MANDEL_ENABLE_FAST_SCROLL (1)

namespace tinymandelbrot {
    
#ifdef PIXEL_DOUBLE
    static constexpr int W = 120;
    static constexpr int H = 120;
    static constexpr int MAX_ZOOM = 18;
    static constexpr int PIXEL_SCALE_BITS = 6; // clog2(W/2)
#else
    static constexpr int W = 240;
    static constexpr int H = 240;
    static constexpr int MAX_ZOOM = 17;
    static constexpr int PIXEL_SCALE_BITS = 7; // clog2(W/2)
#endif

    // queue size = (1 << QUEUE_SIZE_BITS)
    static constexpr int QUEUE_SIZE_BITS = 12;

#if MANDEL_ENABLE_FIXED_POINT
    // fixed point type
    using elem_t = int32_t;

    // fixed point position
    static constexpr int FIXED_POINT_POS = sizeof(elem_t) * 8 - 8;
#else
    // fixed point type
    using elem_t = float;

    // fixed point position
    static constexpr int FIXED_POINT_POS = 0;
#endif

    // mandelbrot calculation loop counter type
    using count_t = uint8_t;

    // max calculation loop count
    static constexpr count_t MAX_LOOPS = 128;

};

#endif
