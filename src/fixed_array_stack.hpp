#ifndef FIXED_ARRAY_STACK_HPP
#define FIXED_ARRAY_STACK_HPP

template<typename T, int CAPACITY>
class FixedArrayStack {
public:
    int size;
    T array[CAPACITY];

    FixedArrayStack() : size(0) { }

    void clear() { size = 0; }
    bool empty() const { return size <= 0; }
    bool full() const { return size >= CAPACITY; }

    bool push(T value) {
        if (full()) return false;
        array[size++] = value;
        return true;
    }

    bool pop(T *value) {
        if (empty()) return false;
        *value = array[--size];
        return true;
    }

    T pop() {
        T value;
        pop(&value);
        return value;
    }
};

#endif
