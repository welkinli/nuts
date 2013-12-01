// buffer.h

#ifndef __WNS_BUFFER_H__
#define __WNS_BUFFER_H__
#include <stddef.h>

namespace WNS
{

    template<class T> inline T MyMin(T a, T b)
    {
        return a < b ? a : b;
    }
    template<class T> inline T MyMax(T a, T b)
    {
        return a > b ? a : b;
    }

    template<class T> inline int MyCompare(T a, T b)
    {
        return a < b ? -1 : (a == b ? 0 : 1);
    }

    inline int BoolToInt(bool value)
    {
        return (value ? 1 : 0);
    }

    inline bool IntToBool(int value)
    {
        return (value != 0);
    }

    template<class T> class CBuffer
    {
        protected:
            size_t _capacity;
            T *_items;
        public:
            void Free()
            {
                delete[] _items;
                _items = 0;
                _capacity = 0;
            }
            CBuffer() : _capacity(0), _items(0)
            {
            }
            ;
            CBuffer(const CBuffer &buffer) : _capacity(0), _items(0)
            {
                *this = buffer;
            }
            CBuffer(size_t size) : _items(0), _capacity(0)
            {
                SetCapacity(size);
            }
            virtual ~CBuffer()
            {
                delete[] _items;
            }
            operator T *()
            {
                return _items;
            }
            ;
            operator const T *() const
            {
                return _items;
            }
            ;
            size_t GetCapacity() const
            {
                return _capacity;
            }
            void SetCapacity(size_t newCapacity)
            {
                if (newCapacity == _capacity)
                    return;
                T *newBuffer;
                if (newCapacity > 0)
                {
                    newBuffer = new T[newCapacity];
                    if (_capacity > 0)
                        memmove(newBuffer, _items, MyMin(_capacity, newCapacity) * sizeof(T));
                }
                else
                    newBuffer = 0;
                delete[] _items;
                _items = newBuffer;
                _capacity = newCapacity;
            }
            void ExpandIfNeed(size_t needCapaticy)
            {
                if (needCapaticy > _capacity)
                    SetCapacity(needCapaticy);
            }

            CBuffer& operator=(const CBuffer &buffer)
            {
                Free();
                if (buffer._capacity > 0)
                {
                    SetCapacity(buffer._capacity);
                    memmove(_items, buffer._items, buffer._capacity * sizeof(T));
                }
                return *this;
            }
    };

    template<class T>
    bool operator==(const CBuffer<T>& b1, const CBuffer<T>& b2)
    {
        if (b1.GetCapacity() != b2.GetCapacity())
            return false;
        for (size_t i = 0; i < b1.GetCapacity(); i++)
            if (b1[i] != b2[i])
                return false;
        return true;
    }

    template<class T>
    bool operator!=(const CBuffer<T>& b1, const CBuffer<T>& b2)
    {
        return !(b1 == b2);
    }

    typedef CBuffer<char> CCharBuffer;
    typedef CBuffer<wchar_t> CWCharBuffer;
    typedef CBuffer<unsigned char> CByteBuffer;
}
#endif//__WNS_BUFFER_H__
