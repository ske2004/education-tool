#pragma once

#include "catedu/core/alloc/allocator.hpp"
#include <assert.h>
#include <iterator>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// MARK: Stack
template <class T> struct Stack
{
    T *values;
    size_t capacity;
    size_t count;

    void deinit()
    {
        ALLOCATOR_MALLOC.free(values);
    }

    bool scale()
    {
        size_t new_capacity = capacity;
        if (new_capacity == 0)
            new_capacity = 32;
        else
            new_capacity *= 2;

        T *new_values =
            (T *)ALLOCATOR_MALLOC.realloc(values, sizeof(T) * new_capacity);

        if (new_values == NULL)
        {
            return false;
        }

        values = new_values;
        capacity = new_capacity;

        return true;
    }

    void push(T value)
    {
        if (count == capacity)
        {
            if (!scale())
            {
                fprintf(stderr, "Stack::push: allocation failed, dropping value\n");
                assert(false && "Stack::push: allocation failed");
                return;
            }
        }

        values[count++] = value;
    }

    T pop()
    {
        assert(count > 0);

        return values[--count];
    }

    void del(size_t index)
    {
        assert(index < count);

        for (size_t i = index; i < count - 1; i++)
        {
            values[i] = values[i + 1];
        }

        count--;
    }

    void ins(size_t index, T value)
    {
        assert(index <= count);

        if (count == capacity)
        {
            if (!scale())
            {
                fprintf(stderr, "Stack::ins: allocation failed, dropping value\n");
                assert(false && "Stack::ins: allocation failed");
                return;
            }
        }

        for (size_t i = count; i > index; i--)
        {
            values[i] = values[i - 1];
        }

        values[index] = value;
        count++;
    }

    T &top()
    {
        assert(count > 0);

        return values[count - 1];
    }

    T &operator[](size_t index)
    {
        assert(index < count);

        return values[index];
    }

    bool empty()
    {
        return count == 0;
    }
};

// MARK: Iterator

template <class T> struct StackIterator
{
    Stack<T> *stack;
    size_t i;

    bool going()
    {
        return i < stack->count;
    }

    void next()
    {
        i++;
    }

    static StackIterator init(Stack<T> *stack)
    {
        StackIterator iterator = {stack, 0};
        return iterator;
    }
};

template <class T> struct CxxStackIterator
{
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using reference = T;

    StackIterator<T> iterator;

    CxxStackIterator(StackIterator<T> iterator) : iterator(iterator)
    {
    }

    CxxStackIterator begin()
    {
        return *this;
    }

    CxxStackIterator end()
    {
        // NOTE: This is a hack to make the iterator work with range-based for
        return CxxStackIterator(
            StackIterator<T>{iterator.stack, iterator.stack->count});
    }

    bool operator==(const CxxStackIterator<T> &other) const
    {
        return iterator.i == other.iterator.i &&
               iterator.stack == other.iterator.stack;
    }

    bool operator!=(const CxxStackIterator<T> &other) const
    {
        return !(*this == other);
    }

    T &operator*()
    {
        return iterator.stack->values[iterator.i];
    }

    T *operator->()
    {
        return iterator.stack->values + iterator.i;
    }

    CxxStackIterator &operator++()
    {
        iterator.next();
        return *this;
    }

    CxxStackIterator operator++(int)
    {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }
};

template <class T> CxxStackIterator<T> iter(Stack<T> &stack)
{
    return CxxStackIterator<T>(StackIterator<T>::init(&stack));
}
