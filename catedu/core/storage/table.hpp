///
/// BRIEF: Table for allocation and management of rows
///

#pragma once
#include <assert.h>
#include <catedu/core/alloc/allocator.hpp>
#include <iterator>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct TableId
{
    size_t id;
    size_t generation;

    bool operator==(TableId other)
    {
        return (id ? generation == other.generation : true) && id == other.id;
    }

    bool operator!=(TableId other)
    {
        return !(*this == other);
    }

    bool valid()
    {
        return id != 0;
    }
};

#define NULL_ID (TableId{0})

template <class T> struct Table
{
    T *values;
    struct Slot
    {
        bool taken;
        size_t generation;
    } *slots;
    size_t capacity;
    size_t count;

    void deinit()
    {
        ALLOCATOR_MALLOC.free(values);
        ALLOCATOR_MALLOC.free(slots);
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
            fprintf(stderr, "Table::scale: values realloc failed\n");
            assert(false && "Table::scale: values realloc failed");
            return false;
        }
        values = new_values;

        Slot *new_slots = (Slot *)ALLOCATOR_MALLOC.realloc(
            slots, sizeof(Slot) * new_capacity);
        if (new_slots == NULL)
        {
            // values was already expanded but capacity is unchanged,
            // so accesses remain within the old bounds — safe but wasteful.
            fprintf(stderr, "Table::scale: slots realloc failed\n");
            assert(false && "Table::scale: slots realloc failed");
            return false;
        }
        slots = new_slots;

        memset(slots + capacity, 0, (new_capacity - capacity) * sizeof(Slot));

        capacity = new_capacity;
        return true;
    }

    TableId allocate_at(TableId id, T value)
    {
        while (capacity < id.id)
        {
            if (!scale())
                return NULL_ID;
        }

        count++;
        slots[id.id - 1].generation = id.generation;
        slots[id.id - 1].taken = true;
        values[id.id - 1] = value;
        return id;
    }

    TableId find_next_id()
    {
        TableId id = {0};
        for (size_t i = 0; i < capacity; i++)
        {
            if (!slots[i].taken)
            {
                id = {i + 1};
                id.generation = slots[i].generation;
                break;
            }
        }

        if (id.id == 0)
        {
            id = {capacity + 1};
        }

        return id;
    }

    void mark_generation_at(TableId id)
    {
        while (capacity < id.id)
        {
            bool ok = scale();
            assert(ok);
        }

        slots[id.id - 1].generation = id.generation;
    }

    TableId allocate(T value)
    {
        TableId id = {0};
        for (size_t i = 0; i < capacity; i++)
        {
            if (!slots[i].taken)
            {
                id = {i + 1};
                break;
            }
        }

        if (id.id == 0)
        {
            size_t old_capacity = capacity;
            if (!scale())
            {
                return NULL_ID;
            }
            id = {old_capacity + 1};
        }

        count++;
        slots[id.id - 1].taken = true;
        id.generation = slots[id.id - 1].generation;
        values[id.id - 1] = value;
        return id;
    }

    /// RETURNS: Status value indicating whether the element was freed correctly
    bool remove(TableId id)
    {
        if (id.id - 1 < capacity && slots[id.id - 1].taken &&
            slots[id.id - 1].generation == id.generation)
        {
            slots[id.id - 1].taken = false;
            slots[id.id - 1].generation++;
            count--;
            return true;
        }
        return false;
    }

    // @note Use this only when it's a programming error to have an invalid ID.
    //       This in general applies to all asserts.
    T &get_assert(TableId id) const
    {
        assert(id.id - 1 < capacity && slots[id.id - 1].taken &&
               slots[id.id - 1].generation == id.generation && "Invalid ID.");
        return values[id.id - 1];
    }

    T *get(TableId id)
    {
        if (id.id - 1 < capacity && slots[id.id - 1].taken &&
            slots[id.id - 1].generation == id.generation)
        {
            return &values[id.id - 1];
        }
        return nullptr;
    }
};

template <class T> struct TableIterator
{
    Table<T> *table;
    TableId id;
    bool is_going;

    bool going()
    {
        return is_going;
    }

    void next()
    {
        is_going = false;

        id.id += 1;
        while ((id.id - 1) < table->capacity)
        {
            if (table->slots[id.id - 1].taken)
            {
                id.generation = table->slots[id.id - 1].generation;
                is_going = true;
                break;
            }
            id.id += 1;
        }
    }

    static TableIterator init(Table<T> *table)
    {
        TableIterator iterator = {table, {0}, false};
        iterator.next();
        return iterator;
    }
};

template <class T> struct CxxTableIterator
{
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<TableId, T &>;
    using difference_type = std::ptrdiff_t;
    using pointer = std::pair<TableId, T *> *;
    using reference = std::pair<TableId, T &>;

    TableIterator<T> iterator;

    CxxTableIterator(TableIterator<T> iterator) : iterator(iterator)
    {
    }

    CxxTableIterator begin()
    {
        return *this;
    }

    CxxTableIterator end()
    {
        // NOTE: This is a hack to make the iterator work with range-based for
        return CxxTableIterator(TableIterator<T>{iterator.table, {0}, false});
    }

    bool operator==(const CxxTableIterator<T> &other) const
    {

        return iterator.is_going == other.iterator.is_going &&
               (iterator.is_going ? iterator.table == other.iterator.table &&
                                        iterator.id.id == other.iterator.id.id
                                  : true);
    }

    bool operator!=(const CxxTableIterator<T> &other) const
    {
        return !(*this == other);
    }

    std::pair<TableId, T &> operator*()
    {
        return {iterator.id, iterator.table->get_assert(iterator.id)};
    }

    std::pair<TableId, T *> operator->()
    {
        return {iterator.id, iterator.table->get(iterator.id)};
    }

    CxxTableIterator &operator++()
    {
        iterator.next();
        return *this;
    }

    CxxTableIterator operator++(int)
    {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }
};

template <class T> CxxTableIterator<T> iter(Table<T> &table)
{
    return CxxTableIterator<T>(TableIterator<T>::init(&table));
}
