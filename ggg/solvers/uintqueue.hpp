#ifndef GGG_Uintqueue_HPP
#define GGG_Uintqueue_HPP

#include <stdexcept>
#include <utility>

class Uintqueue {
  public:
    /******************************************************************************/
    /* Typedef                                                                    */
    /******************************************************************************/

    typedef unsigned int uint;

    /******************************************************************************/

    /******************************************************************************/
    /* Constructors and destructor                                                */
    /******************************************************************************/

    Uintqueue() {
        pointer = 0;
        queue = nullptr;
    }

    Uintqueue(uint capacity) {
        pointer = 0;
        queue = new uint[capacity];
    }

    ~Uintqueue() {
        delete[] queue;
    }

    /******************************************************************************/

    /******************************************************************************/
    /* Main methods                                                               */
    /******************************************************************************/

    __attribute__((always_inline)) bool nonempty() {
        return ((bool)pointer);
    }

    __attribute__((always_inline)) bool empty() {
        return pointer == 0;
    }

    __attribute__((always_inline)) uint pop() {
        return (queue[--pointer]);
    }

    __attribute__((always_inline)) void pop2() {
        pointer -= 2;
    }

    __attribute__((always_inline)) void push(uint element) {
#ifndef NDEBUG
        if (pointer >= capacity) {
            throw std::runtime_error("Uintqueue: push() would exceed capacity");
        }
#endif
        queue[pointer++] = element;
    }

    __attribute__((always_inline)) uint &back() {
        return (queue[pointer - 1]);
    }

    __attribute__((always_inline)) uint &back2() {
        return (queue[pointer - 2]);
    }

    __attribute__((always_inline)) void clear() {
        pointer = 0;
    }

    __attribute__((always_inline)) uint &operator[](uint idx) {
        return (queue[idx]);
    }

    __attribute__((always_inline)) uint size() {
        return (pointer);
    }

    __attribute__((always_inline)) void resize(uint capacity) {
        pointer = 0;
        if (queue != nullptr) {
            delete[] queue;
        }
        queue = new uint[capacity];
        this->capacity = capacity;
    }

    __attribute__((always_inline)) void swap(Uintqueue &other) {
        std::swap(queue, other.queue);
        std::swap(pointer, other.pointer);
        std::swap(capacity, other.capacity);
    }

    __attribute__((always_inline)) void swap_elements(uint idx1, uint idx2) {
        std::swap(queue[idx1], queue[idx2]);
    }

    /******************************************************************************/

  protected:
    /******************************************************************************/
    /* Queue and pointer                                                          */
    /******************************************************************************/

    uint *queue;
    int pointer;
    uint capacity = 0;

    /******************************************************************************/
};

#endif
