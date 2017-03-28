#pragma once

#include "f4se/GameAPI.h"

template<class T>
class BSTArrayFunctor
{
public:
        BSTArrayFunctor(tArray<T> * arr, UInt32 growSize = 10, UInt32 shrinkSize = 10) : m_array(arr)
        {
                m_growSize = growSize;
                if(m_growSize == 0)
                        m_growSize = 1;
 
                m_shrinkSize = shrinkSize;
        }
 
        bool Allocate(UInt32 numEntries)
        {
                if(!m_array)
                        return false;
 
                if(m_array->entries)
                        Heap_Free(m_array->entries);
 
                m_array->entries = (T *)Heap_Allocate(sizeof(T) * numEntries);
                if(!m_array->entries) return false;
 
                for(UInt32 i = 0; i < numEntries; i++)
                        new (&m_array->entries[i]) T;
 
                m_array->capacity = numEntries;
                m_array->count = numEntries;
                return true;
        };
 
        void Set(T * entries, UInt32 count)
        {
                // Not enough space to hold the remaining items, free the old list and allocate
                if (count > m_array->capacity) {
                        // Free the existing array
                        if (m_array->entries)
                                Heap_Free(m_array->entries);
 
                        UInt32 newCapacity = count;
                        if (count % m_growSize > 0) // Amount we are allocating to is less the growth size
                                newCapacity = count + (count % m_growSize);
 
                        m_array->entries = (T *)Heap_Allocate(sizeof(T) * newCapacity);
                        memcpy_s(m_array->entries, sizeof(T) * count, entries, sizeof(T) * count);
 
                        if (newCapacity > count) {
                                for (UInt32 i = count; i < newCapacity; i++) // Allocate the remaining capacity
                                        new (&m_array->entries[i]) T;
                        }
 
                        m_array->capacity = newCapacity;
                        m_array->count = count;
                }
                else
                {
                        memcpy_s(m_array->entries, sizeof(T) * count, entries, sizeof(T) * count);
 
                        if (m_array->capacity > count) {
                                for (UInt32 i = count; i < m_array->capacity; i++) // Allocate the remaining empty capacity
                                        new (&m_array->entries[i]) T;
                        }
 
                        m_array->count = count;
                }
        }
 
        bool Push(const T & entry)
        {
                if(!m_array) return false;
 
                if(m_array->count + 1 > m_array->capacity) {
                        if(!Grow(m_growSize))
                                return false;
                }
 
                m_array->entries[m_array->count] = entry;
                m_array->count++;
                return true;
        };
 
        bool Insert(UInt32 index, const T & entry)
        {
                if(!m_array->entries || index < m_array->count)
                        return false;
 
                m_array->entries[index] = entry;
                return true;
        };
 
        bool Remove(UInt32 index)
        {
                if(!m_array->entries || index < m_array->count)
                        return false;
 
                m_array->entries[index] = NULL;
                if(index + 1 < m_array->count) {
                        UInt32 remaining = m_array->count - index;
                        memmove_s(&m_array->entries[index + 1], sizeof(T) * remaining, &m_array->entries[index], sizeof(T) * remaining); // Move the rest up
                }
                m_array->count--;
 
                if(m_array->capacity > m_array->count + m_shrinkSize)
                        Shrink();
 
                return true;
        };
 
        bool Shrink()
        {
                if(!m_array || m_array->count == m_array->capacity) return false;
 
                try {
                        UInt32 newSize = m_array->count;
                        T * oldArray = m_array->entries;
                        T * newArray = (T *)Heap_Allocate(sizeof(T) * newSize); // Allocate new block
                        memmove_s(newArray, sizeof(T) * newSize, m_array->entries, sizeof(T) * newSize); // Move the old block
                        m_array->entries = newArray;
                        m_array->capacity = m_array->count;
                        Heap_Free(oldArray); // Free the old block
                        return true;
                }
                catch(...) {
                        return false;
                }
 
                return false;
        }
 
        bool Grow(UInt32 entries)
        {
                if(!m_array) return false;
 
                try {
                        UInt32 oldSize = m_array->capacity;
                        UInt32 newSize = oldSize + entries;
                        T * oldArray = m_array->entries;
                        T * newArray = (T *)Heap_Allocate(sizeof(T) * newSize); // Allocate new block
						if(oldArray)
							memmove_s(newArray, sizeof(T) * newSize, m_array->entries, sizeof(T) * m_array->capacity); // Move the old block
                        m_array->entries = newArray;
                        m_array->capacity = newSize;

						if(oldArray)
							Heap_Free(oldArray); // Free the old block
 
                        for(UInt32 i = oldSize; i < newSize; i++) // Allocate the rest of the free blocks
                                new (&m_array->entries[i]) T;
                       
                        return true;
                }
                catch(...) {
                        return false;
                }
 
                return false;
        };
 
private:
        tArray<T> * m_array;
        UInt32  m_growSize;
        UInt32  m_shrinkSize;
};