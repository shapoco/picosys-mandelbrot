#ifndef BUFFER2D_UTILS
#define BUFFER2D_UTILS

struct pos_t {
    int16_t x, y;
    pos_t() : x(0), y(0) {}
    pos_t(int16_t x, int16_t y) : x(x), y(y) {}

    pos_t offset(int16_t dx, int16_t dy) { return pos_t(x + dx, y + dy); }

    pos_t operator +(pos_t other) const { return pos_t(x + other.x, y + other.y); }
    pos_t operator -(pos_t other) const { return pos_t(x - other.x, y - other.y); }
    //pos_t operator =(pos_t a) { x = a.x; y = a.y; }
    bool operator ==(pos_t other) const { return x == other.x && y == other.y; }
};

struct rect_t {
    int16_t x, y, w, h;
    
    rect_t() : x(0), y(0), w(0), h(0) {}
    rect_t(int16_t x, int16_t y, int16_t w, int16_t h) : x(x), y(y), w(w), h(h) {}

    static rect_t from_ltrb(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
        return rect_t(x0, y0, x1 - x0, y1 - y0);
    }

    int16_t r() const { return x + w; }
    int16_t b() const { return y + h; }
    
    bool empty() { return w == 0 || h == 0; }

    rect_t offset(int16_t dx, int16_t dy) const { 
        return rect_t(x + dx, y + dy, w, h); 
    }

    bool contains(pos_t pos) const {
        return
            x <= pos.x && pos.x < r() && 
            y <= pos.y && pos.y < b();
    }
    
    rect_t intersect(rect_t other) const {
        auto result = rect_t::from_ltrb(
            x >= other.x ? x : other.x,
            y >= other.y ? y : other.y,
            r() < other.r() ? r() : other.r(),
            b() < other.b() ? b() : other.b()
        );
        if (result.w >= 0 && result.h >= 0) {
            return result;
        }
        else {
            return rect_t(x, y, 0, 0);
        }
    }

    bool operator ==(rect_t other) const { 
        return x == other.x && y == other.y && w == other.w && h == other.h; 
    }
};

template<typename T>
class Buffer2D {
public:
    const int16_t W, H, STRIDE;
    T *data;
    bool destroy;

    Buffer2D(int16_t w, int16_t h) : Buffer2D(w, h, w, new T[w * h], true) { }

    Buffer2D(int16_t w, int16_t h, int16_t stride, T* data, bool destroy = false) 
        : W(w), H(h), STRIDE(stride), data(data), destroy(destroy) { }

    ~Buffer2D() {
        if (destroy) {
            delete[] data;
        }
    }
    
    rect_t bounds() const { return rect_t(0, 0, W, H); }

    T &operator[] (pos_t p) const { return data[p.y * STRIDE + p.x]; }
    T &operator[] (int i) const { return data[i]; }
    //T &operator[] (int x, int y) { return data[y * STRIDE + x]; }
    //T operator[] const (int x, int y) { return data[y * STRIDE + x]; }

    T *ptr(pos_t p) const { return data + p.y * STRIDE + p.x; }
    T *ptr(int16_t x, int16_t y) const { return data + y * STRIDE + x; }

    void fill(T value = 0) {
        fill(bounds(), value);
    }

    void fill(rect_t rect, T value = 0) {
        rect = rect.intersect(bounds());
        auto r = rect.r();
        auto b = rect.b();
        for (int16_t y = rect.y; y < b; y++) {
            auto *wr_ptr = ptr(rect.x, y);
            for (int16_t x = rect.x; x < r; x++) {
                *(wr_ptr++) = value;
            }
        }
    }

    void scroll(int16_t dx, int16_t dy) {
        int16_t y_src = 0, y_dst = 0;
        if (dy > 0) {
            y_src = 0;
            y_dst = dy;
        }
        else if (dy < 0) {
            y_src = -dy;
            y_dst = 0;
        }

        int16_t x_src = 0, x_dst = 0;
        if (dx > 0) {
            x_src = 0;
            x_dst = dx;
        }
        else if (dx < 0) {
            x_src = -dx;
            x_dst = 0;
        }

        auto *src = data + y_src * STRIDE + x_src;
        auto *dst = data + y_dst * STRIDE + x_dst;

        int16_t w_copy = W - abs(dx);
        int16_t h_copy = H - abs(dy);

        if (dy < 0) {
            for (int16_t i = 0; i < h_copy; i++) {
                line_copy(dst, src, w_copy);
                src += STRIDE;
                dst += STRIDE;
            }
        }
        else {
            src += STRIDE * h_copy;
            dst += STRIDE * h_copy;
            for (int16_t i = 0; i < h_copy; i++) {
                src -= STRIDE;
                dst -= STRIDE;
                line_copy(dst, src, w_copy);
            }
        }
    }

    void line_copy(T *dst, T *src, int16_t n) {
        if (dst < src) {
            for (int16_t i = 0; i < n; i++) {
                *(dst++) = *(src++);
            }
        }
        else {
            dst += n;
            src += n;
            for (int16_t i = 0; i < n; i++) {
                *(--dst) = *(--src);
            }
        }
    }

};


#endif
