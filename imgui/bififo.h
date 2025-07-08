#pragma once
#include <iostream>

template<typename T, const unsigned int stack_size>
struct BiFIFO {
    T data[stack_size];
    unsigned int start_fifo_1; //oldest element of fifo_1
    unsigned int start_fifo_2; //oldest element in fifo_2
    unsigned int end_fifo_1;

    //fifo_1 : [start_fifo_1, end_fifo_1[
    //fifo_2 : [start_fifo_2, end_fifo_1]

    BiFIFO() {
        clear();
    };

    unsigned int _add_ptr(int ptr) {
        if (ptr == stack_size - 1)
            return 0;
        return ptr + 1;
    }

    unsigned int _decr_ptr(int ptr) {
        if (ptr == 0)
            return stack_size - 1;
        return ptr - 1;
    }

    //clear fifo_2 and add  an element in fifo1
    void push(T&& element) {
        data[end_fifo_1] = std::move(element);
        start_fifo_2 = end_fifo_1;
        end_fifo_1 = _add_ptr(end_fifo_1);
        if(end_fifo_1 == start_fifo_1)
            start_fifo_1 = _add_ptr(start_fifo_1);
    }

    void push(const T& element) {
        data[end_fifo_1] = T(element);
        start_fifo_2 = end_fifo_1;
        end_fifo_1 = _add_ptr(end_fifo_1);
        if (end_fifo_1 == start_fifo_1)
            start_fifo_1 = _add_ptr(start_fifo_1);
    }

    //pop last element of fifo1 and push it into fifo2. return true if succesful
    bool pop(T** dest) {
        if (end_fifo_1 == start_fifo_1)
            return false;
        end_fifo_1 = _decr_ptr(end_fifo_1); //implicitely push into fifo_2
        *dest = &(data[end_fifo_1]);
        return true;
    }

    //pop last element of fifo2 and push it into fifo1. return true if succesful
    bool unpop(T** dest) {
        if (_decr_ptr(end_fifo_1) == start_fifo_2)
            return false;
        *dest = &(data[end_fifo_1]);
        end_fifo_1 = _add_ptr(end_fifo_1); //implicitely push into fifo_1

        return true;
    }

    void clear_secondFIFO() {
        start_fifo_2 = _decr_ptr(end_fifo_1);
    }
    void clear_firstFIFO() {
        start_fifo_1 = end_fifo_1;
    }
    void clear() {
        start_fifo_1 = 0;
        end_fifo_1 = 0;
        start_fifo_2 = stack_size - 1;
    }
};