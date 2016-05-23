//
//  Resource.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 10/8/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Resource_h
#define hifi_gpu_Resource_h

#include <assert.h>

#include "Format.h"

#include <vector>
#include <atomic>

#include <memory>
#ifdef _DEBUG
#include <QDebug>
#endif

namespace gpu {

class Resource {
public:
    typedef size_t Size;

    static const Size NOT_ALLOCATED = (Size)-1;

    // The size in bytes of data stored in the resource
    virtual Size getSize() const = 0;

    enum Type {
        BUFFER = 0,
        TEXTURE_1D,
        TEXTURE_2D,
        TEXTURE_3D,
        TEXTURE_CUBE,
        TEXTURE_1D_ARRAY,
        TEXTURE_2D_ARRAY,
        TEXTURE_3D_ARRAY,
        TEXTURE_CUBE_ARRAY,
    };

protected:

    Resource() {}
    virtual ~Resource() {}

    // Sysmem is the underneath cache for the data in ram of a resource.
    class Sysmem {
    public:

        Sysmem();
        Sysmem(Size size, const Byte* bytes);
        Sysmem(const Sysmem& sysmem); // deep copy of the sysmem buffer
        Sysmem& operator=(const Sysmem& sysmem); // deep copy of the sysmem buffer
        ~Sysmem();

        Size getSize() const { return _size; }

        // Allocate the byte array
        // \param pSize The nb of bytes to allocate, if already exist, content is lost.
        // \return The nb of bytes allocated, nothing if allready the appropriate size.
        Size allocate(Size pSize);

        // Resize the byte array
        // Keep previous data [0 to min(pSize, mSize)]
        Size resize(Size pSize);

        // Assign data bytes and size (allocate for size, then copy bytes if exists)
        Size setData(Size size, const Byte* bytes );

        // Update Sub data, 
        // doesn't allocate and only copy size * bytes at the offset location
        // only if all fits in the existing allocated buffer
        Size setSubData(Size offset, Size size, const Byte* bytes);

        // Append new data at the end of the current buffer
        // do a resize( size + getSIze) and copy the new data
        // \return the number of bytes copied
        Size append(Size size, const Byte* data);

        // Access the byte array.
        // The edit version allow to map data.
        const Byte* readData() const { return _data; } 
        Byte* editData() { return _data; }

        template< typename T > const T* read() const { return reinterpret_cast< T* > ( _data ); } 
        template< typename T > T* edit() { return reinterpret_cast< T* > ( _data ); } 

        // Access the current version of the sysmem, used to compare if copies are in sync
        Stamp getStamp() const { return _stamp; }

        static Size allocateMemory(Byte** memAllocated, Size size);
        static void deallocateMemory(Byte* memDeallocated, Size size);

        bool isAvailable() const { return (_data != 0); }

    private:
        Stamp _stamp { 0 };
        Size  _size { 0 };
        Byte* _data { nullptr };
    };

};

class Buffer : public Resource {
    static std::atomic<uint32_t> _bufferCPUCount;
    static std::atomic<Size> _bufferCPUMemoryUsage;
    static void updateBufferCPUMemoryUsage(Size prevObjectSize, Size newObjectSize);

public:
    enum Flag {
        DIRTY = 0x01,
    };

    // Currently only one flag... 'dirty'
    using PageFlags = std::vector<uint8_t>;
    static const Size DEFAULT_PAGE_SIZE = 4096;
    static uint32_t getBufferCPUCount();
    static Size getBufferCPUMemoryUsage();
    static uint32_t getBufferGPUCount();
    static Size getBufferGPUMemoryUsage();

    Buffer(Size pageSize = DEFAULT_PAGE_SIZE);
    Buffer(Size size, const Byte* bytes, Size pageSize = DEFAULT_PAGE_SIZE);
    Buffer(const Buffer& buf); // deep copy of the sysmem buffer
    Buffer& operator=(const Buffer& buf); // deep copy of the sysmem buffer
    ~Buffer();

    // The size in bytes of data stored in the buffer
    Size getSize() const;
    const Byte* getData() const { return getSysmem().readData(); }
    
    // Resize the buffer
    // Keep previous data [0 to min(pSize, mSize)]
    Size resize(Size pSize);

    // Assign data bytes and size (allocate for size, then copy bytes if exists)
    // \return the size of the buffer
    Size setData(Size size, const Byte* data);

    // Assign data bytes and size (allocate for size, then copy bytes if exists)
    // \return the number of bytes copied
    Size setSubData(Size offset, Size size, const Byte* data);

    template <typename T>
    Size setSubData(Size index, const T& t) {
        Size offset = index * sizeof(T);
        Size size = sizeof(T);
        return setSubData(offset, size, reinterpret_cast<const Byte*>(&t));
    }

    template <typename T>
    Size setSubData(Size index, const std::vector<T>& t) {
        if (t.empty()) {
            return 0;
        }
        Size offset = index * sizeof(T);
        Size size = t.size() * sizeof(T);
        return setSubData(offset, size, reinterpret_cast<const Byte*>(&t[0]));
    }

    // Append new data at the end of the current buffer
    // do a resize( size + getSize) and copy the new data
    // \return the number of bytes copied
    Size append(Size size, const Byte* data);

    template <typename T> 
    Size append(const T& t) {
        return append(sizeof(t), reinterpret_cast<const Byte*>(&t));
    }

    template <typename T>
    Size append(const std::vector<T>& t) {
        if (t.empty()) {
            return _end;
        }
        return append(sizeof(T) * t.size(), reinterpret_cast<const Byte*>(&t[0]));
    }

    bool getNextTransferBlock(Size& outOffset, Size& outSize, Size& currentPage) const {
        Size pageCount = _pages.size();
        // Advance to the first dirty page
        while (currentPage < pageCount && (0 == (Buffer::DIRTY & _pages[currentPage]))) {
            ++currentPage;
        }

        // If we got to the end, we're done
        if (currentPage >= pageCount) {
            return false;
        }

        // Advance to the next clean page
        outOffset = static_cast<Size>(currentPage * _pageSize);
        while (currentPage < pageCount && (0 != (Buffer::DIRTY & _pages[currentPage]))) {
            _pages[currentPage] &= ~Buffer::DIRTY;
            ++currentPage;
        }
        outSize = static_cast<Size>((currentPage * _pageSize) - outOffset);
        return true;
    }

    const GPUObjectPointer gpuObject {};
    
    // Access the sysmem object, limited to ourselves and GPUObject derived classes
    const Sysmem& getSysmem() const { return _sysmem; }
    // FIXME find a better access mechanism for clearing this
    mutable uint8_t _flags;
protected:
    void markDirty(Size offset, Size bytes);

    template <typename T>
    void markDirty(Size index, Size count = 1) {
        markDirty(sizeof(T) * index, sizeof(T) * count);
    }

    Sysmem& editSysmem() { return _sysmem; }
    Byte* editData() { return editSysmem().editData(); }

    Size getRequiredPageCount() const;

    Size _end { 0 };
    mutable PageFlags _pages;
    const Size _pageSize;
    Sysmem _sysmem;

    // FIXME find a more generic way to do this.
    friend class gl::GLBuffer;
    friend class BufferView;
};

typedef std::shared_ptr<Buffer> BufferPointer;
typedef std::vector< BufferPointer > Buffers;


class BufferView {
protected:
    static const Resource::Size DEFAULT_OFFSET{ 0 };
    static const Element DEFAULT_ELEMENT;

public:
    using Size = Resource::Size;
    using Index = int;

    BufferPointer _buffer;
    Size _offset;
    Size _size;
    Element _element;
    uint16 _stride;

    BufferView(const BufferView& view) = default;
    BufferView& operator=(const BufferView& view) = default;

    BufferView();
    BufferView(const Element& element);
    BufferView(Buffer* newBuffer, const Element& element = DEFAULT_ELEMENT);
    BufferView(const BufferPointer& buffer, const Element& element = DEFAULT_ELEMENT);
    BufferView(const BufferPointer& buffer, Size offset, Size size, const Element& element = DEFAULT_ELEMENT);
    BufferView(const BufferPointer& buffer, Size offset, Size size, uint16 stride, const Element& element = DEFAULT_ELEMENT);

    Size getNumElements() const { return _size / _element.getSize(); }

    //Template iterator with random access on the buffer sysmem
    template<typename T>
    class Iterator : public std::iterator<std::random_access_iterator_tag, T, Index, T*, T&>
    {
    public:

        Iterator(T* ptr = NULL, int stride = sizeof(T)): _ptr(ptr), _stride(stride) { }
        Iterator(const Iterator<T>& iterator) = default;
        ~Iterator() {}

        Iterator<T>& operator=(const Iterator<T>& iterator) = default;
        Iterator<T>& operator=(T* ptr) {
            _ptr = ptr;
            // stride is left unchanged
            return (*this);
        }

        operator bool() const
        {
            if(_ptr)
                return true;
            else
                return false;
        }

        bool operator==(const Iterator<T>& iterator) const { return (_ptr == iterator.getConstPtr()); }
        bool operator!=(const Iterator<T>& iterator) const { return (_ptr != iterator.getConstPtr()); }

        void movePtr(const Index& movement) {
            auto byteptr = ((Byte*)_ptr);
            byteptr += _stride * movement;
            _ptr = (T*)byteptr;
        }

        Iterator<T>& operator+=(const Index& movement) {
            movePtr(movement);
            return (*this);
        }
        Iterator<T>& operator-=(const Index& movement) {
            movePtr(-movement);
            return (*this);
        }
        Iterator<T>& operator++() {
            movePtr(1);
            return (*this);
        }
        Iterator<T>& operator--() {
            movePtr(-1);
            return (*this);
        }
        Iterator<T> operator++(Index) {
            auto temp(*this);
            movePtr(1);
            return temp;
        }
        Iterator<T> operator--(Index) {
            auto temp(*this);
            movePtr(-1);
            return temp;
        }
        Iterator<T> operator+(const Index& movement) {
            auto oldPtr = _ptr;
            movePtr(movement);
            auto temp(*this);
            _ptr = oldPtr;
            return temp;
        }
        Iterator<T> operator-(const Index& movement) {
            auto oldPtr = _ptr;
            movePtr(-movement);
            auto temp(*this);
            _ptr = oldPtr;
            return temp;
        }

        Index operator-(const Iterator<T>& iterator) { return (iterator.getPtr() - this->getPtr())/sizeof(T); }

        T& operator*(){return *_ptr;}
        const T& operator*()const{return *_ptr;}
        T* operator->(){return _ptr;}

        T* getPtr()const{return _ptr;}
        const T* getConstPtr()const{return _ptr;}

    protected:

        T* _ptr;
        int _stride;
    };

#if 0
    // Direct memory access to the buffer contents is incompatible with the paging memory scheme
    template <typename T> Iterator<T> begin() { return Iterator<T>(&edit<T>(0), _stride); }
    template <typename T> Iterator<T> end() { return Iterator<T>(&edit<T>(getNum<T>()), _stride); }
#else 
    template <typename T> Iterator<const T> begin() const { return Iterator<const T>(&get<T>(), _stride); }
    template <typename T> Iterator<const T> end() const { return Iterator<const T>(&get<T>(getNum<T>()), _stride); }
#endif
    template <typename T> Iterator<const T> cbegin() const { return Iterator<const T>(&get<T>(), _stride); }
    template <typename T> Iterator<const T> cend() const { return Iterator<const T>(&get<T>(getNum<T>()), _stride); }

    // the number of elements of the specified type fitting in the view size
    template <typename T> Index getNum() const {
        return Index(_size / _stride);
    }

    template <typename T> const T& get() const {
 #if _DEBUG
        if (!_buffer) {
            qDebug() << "Accessing null gpu::buffer!";
        }
        if (sizeof(T) > (_buffer->getSize() - _offset)) {
            qDebug() << "Accessing buffer in non allocated memory, element size = " << sizeof(T) << " available space in buffer at offset is = " << (_buffer->getSize() - _offset);
        }
        if (sizeof(T) > _size) {
            qDebug() << "Accessing buffer outside the BufferView range, element size = " << sizeof(T) << " when bufferView size = " << _size;
        }
 #endif
        const T* t = (reinterpret_cast<const T*> (_buffer->getData() + _offset));
        return *(t);
    }

    template <typename T> T& edit() {
 #if _DEBUG
        if (!_buffer) {
            qDebug() << "Accessing null gpu::buffer!";
        }
        if (sizeof(T) > (_buffer->getSize() - _offset)) {
            qDebug() << "Accessing buffer in non allocated memory, element size = " << sizeof(T) << " available space in buffer at offset is = " << (_buffer->getSize() - _offset);
        }
        if (sizeof(T) > _size) {
            qDebug() << "Accessing buffer outside the BufferView range, element size = " << sizeof(T) << " when bufferView size = " << _size;
        }
 #endif
        _buffer->markDirty(_offset, sizeof(T));
        T* t = (reinterpret_cast<T*> (_buffer->editData() + _offset));
        return *(t);
    }

    template <typename T> const T& get(const Index index) const {
        Resource::Size elementOffset = index * _stride + _offset;
 #if _DEBUG
        if (!_buffer) {
            qDebug() << "Accessing null gpu::buffer!";
        }
        if (sizeof(T) > (_buffer->getSize() - elementOffset)) {
            qDebug() << "Accessing buffer in non allocated memory, index = " << index << ", element size = " << sizeof(T) << " available space in buffer at offset is = " << (_buffer->getSize() - elementOffset);
        }
        if (index > getNum<T>()) {
            qDebug() << "Accessing buffer outside the BufferView range, index = " << index << " number elements = " << getNum<T>();
        }
 #endif
        return *(reinterpret_cast<const T*> (_buffer->getData() + elementOffset));
    }

    template <typename T> T& edit(const Index index) const {
        Resource::Size elementOffset = index * _stride + _offset;
 #if _DEBUG
        if (!_buffer) {
            qDebug() << "Accessing null gpu::buffer!";
        }
        if (sizeof(T) > (_buffer->getSize() - elementOffset)) {
            qDebug() << "Accessing buffer in non allocated memory, index = " << index << ", element size = " << sizeof(T) << " available space in buffer at offset is = " << (_buffer->getSize() - elementOffset);
        }
        if (index > getNum<T>()) {
            qDebug() << "Accessing buffer outside the BufferView range, index = " << index << " number elements = " << getNum<T>();
        }
 #endif
        _buffer->markDirty(elementOffset, sizeof(T));
        return *(reinterpret_cast<T*> (_buffer->editData() + elementOffset));
    }
};
 
};

#endif
