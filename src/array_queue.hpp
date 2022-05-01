#ifndef ARRAY_QUEUE_HPP
#define ARRAY_QUEUE_HPP

template<typename T>
class ArrayQueue {
private:
    int _wr_ptr;
    int _rd_ptr;
    int _size;

public:
    const int CAPACITY;
    const int INDEX_MASK;
    T *array;

    ArrayQueue(int capacity_bits) :
        CAPACITY(1 << capacity_bits), 
        INDEX_MASK(CAPACITY - 1),
        _wr_ptr(0), 
        _rd_ptr(0), 
        _size(0), 
        array(new T[CAPACITY]) { }

    ~ArrayQueue() {
        delete[] array;
    }

    void clear() {
        _wr_ptr = 0;
        _rd_ptr = 0;
        _size = 0;
    }
    int size() const { return _size; }
    bool empty() const { return _size <= 0; }
    bool full() const { return _size >= CAPACITY; }

    bool push(T value) {
        if (full()) return false;
        array[_wr_ptr++] = value;
        _wr_ptr &= INDEX_MASK;
        _size++;
        return true;
    }

    bool pop(T *value) {
        if (empty()) return false;
        *value = array[_rd_ptr++];
        _rd_ptr &= INDEX_MASK;
        _size--;
        return true;
    }

    T pop() {
        T value;
        pop(&value);
        return value;
    }
};

#endif
