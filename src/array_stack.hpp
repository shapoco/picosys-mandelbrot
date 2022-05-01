#ifndef ARRAY_STACK_HPP
#define ARRAY_STACK_HPP

template<typename T>
class ArrayStack {
private:
    int _size;

public:
    const int CAPACITY;
    T *array;

    ArrayStack(int capacity) :
        CAPACITY(capacity),
        _size(0),
        array(new T[capacity]) { }

    ~ArrayStack() {
        delete[] array;
    }

    void clear() { _size = 0; }
    int size() const { return _size; }
    bool empty() const { return _size <= 0; }
    bool full() const { return _size >= CAPACITY; }

    bool push(T value) {
        if (full()) return false;
        array[_size++] = value;
        return true;
    }

    bool pop(T *value) {
        if (empty()) return false;
        *value = array[--_size];
        return true;
    }

    T pop() {
        T value;
        pop(&value);
        return value;
    }
};

#endif
