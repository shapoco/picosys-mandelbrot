#include "picosystem.hpp"
#include "fixed_array_stack.hpp"
#include "tiny_mandelbrot.hpp"

#define BENCHMARK_A (0xffd8849c)
#define BENCHMARK_B (0xfef822ee)
#define BENCHMARK_ZOOM (18)
#define SKIP_STABLE_RECT (1)

using namespace picosystem;
//using namespace tinymandelbrot;

static constexpr int W = tinymandelbrot::W;
static constexpr int H = tinymandelbrot::H;

tinymandelbrot::TinyMandelbrot mandel;

enum state_t {
    SCROLL,
    ZOOM,
};

state_t state = SCROLL;

// zoom animation time (ms)
static constexpr int ZOOM_TIME = 100;

struct scroll_state_t {
    int dx = 0;
    int dy = 0;
    float update_step_accum = 0;
} scroll;

struct zoom_state_t {
    uint32_t t_zoom_end_ms;
    bool is_zoom_in;
    buffer_t *buff;
} zoom;

// color palette
static constexpr int MANDEL_PALETTE_SIZE = 256;
color_t mandel_palette[MANDEL_PALETTE_SIZE];

void scroll_start(uint32_t now);
void scroll_update(uint32_t now, int delta_time);
void scroll_draw();

void zoom_start(uint32_t now, bool zoom_in);
void zoom_update(uint32_t now, int delta_time);
void zoom_draw();

void init() {
    // zoom animation buffer
    zoom.buff = buffer(W/2, H/2);

    // generate color palette
    for (int i = 0; i < 256; i++) {
        int k = (i & 0x7) * 2;
        switch ((i >> 3) % 6) {
        case 0: mandel_palette[i] = rgb(   0,    0,    k); break;
        case 1: mandel_palette[i] = rgb(   0,    k,   15); break;
        case 2: mandel_palette[i] = rgb(   k,   15,   15); break;
        case 3: mandel_palette[i] = rgb(  15,   15, 15-k); break;
        case 4: mandel_palette[i] = rgb(  15, 15-k,    0); break;
        case 5: mandel_palette[i] = rgb(15-k,    0,    0); break;
        }
    }

    scroll_start(0);
}

void update(uint32_t tick) {
    static uint32_t last_time = 0;
    uint32_t now = time();
    int delta_time = now - last_time;
    last_time = now;

    switch (state) {
    case state_t::SCROLL:
        scroll_update(now, delta_time);
        break;
    case state_t::ZOOM:
        zoom_update(now, delta_time);
        break;
    }
}

void draw(uint32_t tick) {
    switch (state) {
    case state_t::SCROLL:
        scroll_draw();
        break;
    case state_t::ZOOM:
        zoom_draw();
        break;
    }
}

void scroll_start(uint32_t now) {
    scroll.dx = 0;
    scroll.dy = 0;
    scroll.update_step_accum = 0;
    state = state_t::SCROLL;
}

void scroll_update(uint32_t now, int delta_time) {
    scroll.update_step_accum += (float)delta_time * 4 / 25;
    int step = scroll.update_step_accum;
    scroll.update_step_accum -= step;

    if (step > W / 10) step = W / 10;

    if (button(LEFT )) scroll.dx -= step;
    if (button(RIGHT)) scroll.dx += step;
    if (button(UP   )) scroll.dy -= step;
    if (button(DOWN )) scroll.dy += step;
    if (pressed(A)) zoom_start(now, true);
    if (pressed(B)) zoom_start(now, false);

    if (pressed(Y)) {
        mandel.set_zoom(BENCHMARK_ZOOM);
        mandel.set_pos(BENCHMARK_A, BENCHMARK_B);
        mandel.invalidate_buffer();
    }
}

void scroll_draw() {
    Buffer2D<color_t> frame_buff(W, H, W, SCREEN->data);

    if (scroll.dx == 0 && scroll.dy == 0 && mandel.no_change()) {
        return ;
    }

    auto t_start = time_us();

    // scroll mandelbrot buffer
    mandel.scroll(scroll.dx, scroll.dy);

    // get stable area for fast scroll
    auto stable_rect = mandel.stable_rect();
    
    // update mandelbrot buffer
    mandel.render();

    // scroll frame buffer
    frame_buff.scroll(-scroll.dx, -scroll.dy);

    // update frame buffer
    int stable_x0 = stable_rect.x;
    int stable_y0 = stable_rect.y;
    int stable_y1 = stable_rect.b();
    for (int y = 0; y < H; y++) {
        auto *rd_ptr = mandel.buff.ptr(0, y);
        auto *wr_ptr = frame_buff.ptr(0, y);
        for (int x = 0; x < W; x++) {
            auto n = *(rd_ptr++);
            if (n >= tinymandelbrot::MAX_LOOPS + 2) {
                *(wr_ptr++) = rgb(0, 0, 0);
            }
            else {
                *(wr_ptr++) = mandel_palette[n % MANDEL_PALETTE_SIZE];
            }

#if SKIP_STABLE_RECT
            if (stable_y0 <= y && y < stable_y1 && x == stable_x0) {
                // skip the that already rendered
                int stride = stable_rect.w - 1;
                x += stride;
                rd_ptr += stride;
                wr_ptr += stride;
            }
#endif
        }
    }

    scroll.dx = 0;
    scroll.dy = 0;

    auto t_elapsed = time_us() - t_start;

#if 0
    pen(rgb(0, 0, 0));
    char buff[32];
    sprintf(buff, "%.3f ms", (float)t_elapsed / 1000);
    text(buff, 5, 5);
#endif

#if 0
    pen(rgb(0, 0, 0));
    frect(0, H - 10, W, 10);
    auto a_offset = view.a_round();
    auto b_offset = view.b_round();
    char buff[32];
    sprintf(buff, "%08x %08x %d", view.a_round(), view.b_round(), view.zoom);

    pen(rgb(15, 15, 15));
    text(buff, 0, H - 10);
#endif
}

// start zoom animation
void zoom_start(uint32_t now, bool zoom_in) {
    if (zoom_in && mandel.zoom_in()) {
        zoom.is_zoom_in = true;
        zoom.t_zoom_end_ms = now + ZOOM_TIME;
        state = state_t::ZOOM;
        // store frame buffer image for animation
        for (int y = 0; y < H / 2; y++) {
            auto *rd_ptr = SCREEN->p(W / 4, y + H / 4);
            auto *wr_ptr = zoom.buff->p(0, y);
            for (int x = 0; x < W / 2; x++) {
                *(wr_ptr++) = *(rd_ptr++);
            }
        }
    }
    else if (!zoom_in && mandel.zoom_out()) {
        zoom.is_zoom_in = false;
        zoom.t_zoom_end_ms = now + ZOOM_TIME;
        state = state_t::ZOOM;
        // store frame buffer image for animation
        for (int y = 0; y < H / 2; y++) {
            auto *rd_ptr = SCREEN->p(0, y * 2);
            auto *wr_ptr = zoom.buff->p(0, y);
            for (int x = 0; x < W / 2; x++) {
                *(wr_ptr++) = *rd_ptr;
                rd_ptr += 2;
            }
        }
    }
}

// update zoom animation
void zoom_update(uint32_t now, int delta_time) {
    if (now > zoom.t_zoom_end_ms + delta_time) {
        // zoom animation finished
        scroll_start(now);
    }
}

// render zoom animation
void zoom_draw() {
    int p = zoom.t_zoom_end_ms - time();
    if (p < 0) {
        p = 0;
    }
    else if (p > ZOOM_TIME) {
        p = ZOOM_TIME;
    }

    if (zoom.is_zoom_in) {
        p = ZOOM_TIME - p;
    }

    int dw = (W / 2) + (W / 2) * p / ZOOM_TIME;
    int dh = (H / 2) + (H / 2) * p / ZOOM_TIME;
    int dx = (W - dw) / 2;
    int dy = (H - dw) / 2;
    blit(zoom.buff, 0, 0, zoom.buff->w, zoom.buff->h, dx, dy, dw, dh);
}
