#ifndef TINY_MANDELBROT_HPP
#define TINY_MANDELBROT_HPP

#include <stdint.h>
#include "tiny_mandelbrot_config.hpp"
#include "array_queue.hpp"
#include "buffer2d_utils.hpp"

namespace tinymandelbrot {

// generate fixed point value
#if MANDEL_ENABLE_FIXED_POINT
static inline elem_t FIXED(int val) { return ((elem_t)val) << FIXED_POINT_POS; }
static inline elem_t FIXED(float val) { return (elem_t)(val * FIXED(1)); }
static inline elem_t FIXED(double val) { return (elem_t)(val * FIXED(1)); }
#else
template<typename T>
static inline elem_t FIXED(T val) { return (elem_t)val; }
#endif

template<typename T>
T limit(T min, T max, T value) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static count_t mandelbrot_func(elem_t a, elem_t b);

class TinyMandelbrot {
public:
    Buffer2D<count_t> buff;
    ArrayQueue<pos_t> queue;

private:
    elem_t _a, _b;
    int _zoom;
    rect_t _stable_rect;

public:
    TinyMandelbrot() : 
        buff(W, H), 
        queue(QUEUE_SIZE_BITS),
        _a(FIXED(-0.5)),
        _b(0),
        _zoom(0)
    {
        buff.fill();
    }

    elem_t a() const { return _a; }
    elem_t b() const { return _b; }
    void set_pos(elem_t a, elem_t b) {
        auto a_px = a_pixel();
        auto b_px = b_pixel();

        const elem_t range = FIXED(2);
        a = round_elem(a);
        b = round_elem(b);
        a = limit(-range, range, a);
        b = limit(-range, range, b);

        if (a == _a && b == _b) return;

        _a = a;
        _b = b;

#if MANDEL_ENABLE_FAST_SCROLL
        if (!_stable_rect.empty()) {
            auto dx = a_pixel() - a_px;
            auto dy = b_pixel() - b_px;
            if (-W < dx && dx < W && -H < dy && dy < H) {
                buff.scroll(-dx, -dy);
                if (dx > 0) {
                    buff.fill(rect_t(W - dx, 0, dx, H));
                }
                else if (dx < 0) {
                    buff.fill(rect_t(0, 0, -dx, H));
                }
                if (dy > 0) {
                    buff.fill(rect_t(0, H - dy, W, dy));
                }
                else if (dy < 0) {
                    buff.fill(rect_t(0, 0, W, -dy));
                }
                _stable_rect = _stable_rect.intersect(_stable_rect.offset(-dx, -dy));
            }
            else {
                invalidate_buffer();
            }
        }
#else
        invalidate_buffer();
#endif
    }
    void scroll(int da, int db) {
        auto px_size = pixel_size();
        auto a = _a + px_size * da;
        auto b = _b + px_size * db;
        set_pos(a, b);
    }

    void invalidate_buffer() { 
        buff.fill();
        _stable_rect = rect_t();
    }
    rect_t stable_rect() const { return _stable_rect; }
    bool no_change() const { return _stable_rect == buff.bounds(); }

    int zoom() const { return _zoom; }
    bool set_zoom(int z) {
        z = limit(0, MAX_ZOOM, z);
        if (z == _zoom) return false;
        _zoom = z;
        invalidate_buffer();
        return true;
    }
    bool zoom_in() { return set_zoom(_zoom + 1); }
    bool zoom_out() { return set_zoom(_zoom - 1); }

    // a, b のピクセル座標のLSB位置
    int pixel_lsb_pos() const { return FIXED_POINT_POS - PIXEL_SCALE_BITS - _zoom; }

    // a から 1 ピクセル未満の端数を除いた値
    elem_t a_round() const { return round_elem(_a); }

    // b から 1 ピクセル未満の端数を除いた値
    elem_t b_round() const { return round_elem(_b); }

#if MANDEL_ENABLE_FIXED_POINT
    // 1ピクセルの a, b の変量
    elem_t pixel_size() const { return FIXED(1) >> (PIXEL_SCALE_BITS + _zoom); }

    // a, b からピクセル座標を抽出するためのマスク
    elem_t round_elem(elem_t val) const {
        elem_t mask = 1;
        mask <<= pixel_lsb_pos();
        mask -= 1;
        mask = ~mask;
        return val & mask; 
    }

    // a のピクセル座標
    int32_t a_pixel() const { return a_round() >> pixel_lsb_pos(); }

    // b のピクセル座標
    int32_t b_pixel() const { return b_round() >> pixel_lsb_pos(); }
#else
    // 1ピクセルの a, b の変量
    elem_t pixel_size() const { return 1.0f / (1L << (PIXEL_SCALE_BITS + _zoom)); }

    // a, b からピクセル座標を抽出するためのマスク
    elem_t round_elem(elem_t val) const {
        int lsb_pos = pixel_lsb_pos();
        int32_t ival = val * (1L << -lsb_pos);
        return (double)ival / (1L << -lsb_pos); 
    }

    // a のピクセル座標
    int32_t a_pixel() const { return a_round() * (1L << -pixel_lsb_pos()); }

    // b のピクセル座標
    int32_t b_pixel() const { return b_round() * (1L << -pixel_lsb_pos()); }
#endif

    // redraw area
    void render() {
        auto step = pixel_size();
        auto a_offset = a_round() - step * (W / 2);
        auto b_offset = b_round() - step * (H / 2);

#if MANDEL_ENABLE_BORDER_SCAN
        // Border Scan Rendering
        push_task_rect(buff.bounds(), false);
        push_task_rect(_stable_rect, true);

        pos_t pos;
        while (queue.pop(&pos)) {
            elem_t a = a_offset + step * pos.x;
            elem_t b = b_offset + step * pos.y;
            auto *val_ptr = buff.ptr(pos);
            auto val = *val_ptr;
            if (val < 2) {
                val = 2 + mandelbrot_func(a, b);
                *val_ptr = val;
            }
            push_neighbor_tasks(pos, val, -1,  0);
            push_neighbor_tasks(pos, val,  1,  0);
            push_neighbor_tasks(pos, val,  0, -1);
            push_neighbor_tasks(pos, val,  0,  1);
        }

        count_t last_n = 0;
        for (int y = 0; y < H; y++) {
            auto *ptr = buff.ptr(0, y);
            for (int x = 0; x < W; x++) {
                auto n = *ptr;
                if (n == 0) {
                    *ptr = last_n;
                }
                else {
                    last_n = n;
                }
                ptr++;
            }
        }
#else
        // Raster Scan Rendering
        auto b = b_offset;
        int stable_rect_r = _stable_rect.r();
        int stable_rect_b = _stable_rect.b();
        for (int y = 0; y < H; y++) {
            auto a = a_offset;
            for (int x = 0; x < W; x++) {
                if (x < _stable_rect.x || stable_rect_r <= x || y < _stable_rect.y || stable_rect_b <= y) {
                    *buff.ptr(x, y) = 2 + mandelbrot_func(a, b);
                }
                a += step;
            }
            b += step;
        }
#endif

        _stable_rect = buff.bounds();
    }

private:
    void push_task_rect(rect_t rect, bool force) {
        int x0 = rect.x, x1 = rect.r();
        int y0 = rect.y, y1 = rect.b();

        for (int x = x0; x < x1; x++) {
            push_task(pos_t(x, y0), force);
            if (rect.w >= 2) {
                push_task(pos_t(x, y1 - 1), force);
            }
        }

        for (int y = y0 + 1; y < y1 - 1; y++) {
            push_task(pos_t(x0, y), force);
            if (rect.h >= 2) {
                push_task(pos_t(x1 - 1, y), force);
            }
        }
    }

    // enqueue calculation task
    void push_task(pos_t pos, bool force) {
        if (!buff.bounds().contains(pos)) return;
        auto &pixel = buff[pos];
        if (pixel != 0 && !force) return;
        pixel = 1;
        queue.push(pos);
    }

    // detect edge and push neighbors
    //  ,   ,   ,
    // -+---+---+-   ,   ,   ,   ,
    //  |new|new|   -+---+---+---+-
    // -+---+---+-   |new| P |new|
    //  | P | Q |   -+---+---+---+-
    // -+---+---+-   |new| Q |new|
    //  |new|new|   -+---+---+---+-
    // -+---+---+-   '   '   '   '
    //  '   '   '
    void push_neighbor_tasks(pos_t pos_p, int val_p, int dx, int dy) {
        auto pos_q = pos_p.offset(dx, dy);
        if (!buff.bounds().contains(pos_q)) return;
        auto val_q = buff[pos_q];
        if (val_q >= 2 && val_p != val_q) {
            if (dx != 0) {
                push_task(pos_p.offset(0, -1), false);
                push_task(pos_q.offset(0, -1), false);
                push_task(pos_p.offset(0,  1), false);
                push_task(pos_q.offset(0,  1), false);
            }
            else if (dy != 0) {
                push_task(pos_p.offset(-1, 0), false);
                push_task(pos_q.offset(-1, 0), false);
                push_task(pos_p.offset( 1, 0), false);
                push_task(pos_q.offset( 1, 0), false);
            }
        }
    }
};

// mandelbrot calculation loop
static count_t mandelbrot_func(elem_t a, elem_t b) {
    elem_t x = 0, y = 0;
    elem_t xx = 0;
    elem_t yy = 0;
    elem_t xy = 0;
    int n = MAX_LOOPS;
    do {
#if MANDEL_ENABLE_FIXED_POINT
#if MANDEL_ENABLE_MULT_SPLIT
        int xysign = 1;
        if (x < 0) { x = -x; xysign = -xysign; }
        if (y < 0) { y = -y; xysign = -xysign; }

        uint16_t xl = x & 0xffffu;
        uint16_t xh = (x >> 16) & 0xffffu;
        uint16_t yl = y & 0xffffu;
        uint16_t yh = (y >> 16) & 0xffffu;

        xy = (elem_t)xl * yl;
        xy = (xy >> 16) & 0xffffu;
        xy += (elem_t)xl * yh;
        xy += (elem_t)xh * yl;
        xy = (xy >> 8) & 0xffffffu;
        xy += ((elem_t)xh * yh) << 8;
        xy *= xysign;

        xx = (elem_t)xl * xl;
        xx = (xx >> 16) & 0xffffu;
        xx += 2 * ((elem_t)xl * xh);
        xx = (xx >> 8) & 0xffffffu;
        xx += ((elem_t)xh * xh) << 8;
        
        yy = (elem_t)yl * yl;
        yy = (yy >> 16) & 0xffffu;
        yy += 2 * ((elem_t)yl * yh);
        yy = (yy >> 8) & 0xffffffu;
        yy += ((elem_t)yh * yh) << 8;
#else
        xy = (((int64_t)x * y) >> FIXED_POINT_POS) & 0xffffffffl;
        xx = (((int64_t)x * x) >> FIXED_POINT_POS) & 0xffffffffl;
        yy = (((int64_t)y * y) >> FIXED_POINT_POS) & 0xffffffffl;
#endif
#else
        xy = x * y;
        xx = x * x;
        yy = y * y;
#endif
        x = xx - yy + a;
        y = xy + xy + b;
    } while (--n != 0 && xx + yy < FIXED(4));
    return MAX_LOOPS - n;
}

} // namespace

#endif
