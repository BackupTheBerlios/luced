/////////////////////////////////////////////////////////////////////////////////////
//
//   LucED - The Lucid Editor
//
//   Copyright (C) 2005-2008 Oliver Schmidt, oliver at luced dot de
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License Version 2 as published
//   by the Free Software Foundation in June 1991.
//
//   This program is distributed in the hope that it will be useful, but WITHOUT
//   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//   more details.
//
//   You should have received a copy of the GNU General Public License along with 
//   this program; if not, write to the Free Software Foundation, Inc., 
//   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
/////////////////////////////////////////////////////////////////////////////////////

#include "HeapObject.hpp"
#include "RawPtrGuardHolder.hpp"
#include "Null.hpp"

#ifndef OWNING_PTR_HPP
#define OWNING_PTR_HPP

namespace LucED
{

template
<
    class T
>
class OwningPtr
{
public:
    OwningPtr(const NullType& nullValue = Null) : ptr(NULL)
    {}
    
    explicit OwningPtr(T* ptr) : ptr(ptr) {
        if (ptr != NULL) {
            HeapObjectRefManipulator::obtainInitialOwnership(ptr);
        }
    }

    ~OwningPtr() {
        HeapObjectRefManipulator::decRefCounter(ptr);
    }
    
    OwningPtr(const OwningPtr& src) {
        ASSERT(this != &src);
        ptr = src.ptr;
        HeapObjectRefManipulator::incRefCounter(ptr);
    }
    
    template<class S> OwningPtr(const OwningPtr<S>& src) {
        if (src.isValid()) {
            ptr = src.getRawPtr();
            HeapObjectRefManipulator::incRefCounter(ptr);
        } else {
            ptr = NULL;
        }
    }
    
    OwningPtr& operator=(const OwningPtr& src) {
        if (src.isValid()) {
            T* ptr1 = ptr;
            ptr = src.getRawPtr();
            HeapObjectRefManipulator::incRefCounter(ptr);
            if (ptr1 != NULL) HeapObjectRefManipulator::decRefCounter(ptr1);
        } else {
            invalidate();
        }
        return *this;
    }
    
    template<class S> OwningPtr& operator=(const OwningPtr<S>& src) {
        if (src.isValid()) {
            T* ptr1 = ptr;
            ptr = src.getRawPtr();
            HeapObjectRefManipulator::incRefCounter(ptr);
            if (ptr1 != NULL) HeapObjectRefManipulator::decRefCounter(ptr1);
        } else {
            invalidate();
        }
        return *this;
    }
    
    void invalidate() {
        if (ptr != NULL) {
            HeapObjectRefManipulator::decRefCounter(ptr);
            ptr = NULL;
        }
    }
    
    bool isValid() const {
        return ptr != NULL;
    }
    
    bool isInvalid() const {
        return ptr == NULL;
    }
    
    T* operator->() const {
        ASSERT(ptr != NULL);
        return ptr;
    }
    
    T* getRawPtr() const {
        return ptr;
    }
    
    operator T*() const {
        return ptr;
    }
    
    bool operator==(const OwningPtr& rhs) const {
        return ptr == rhs.ptr;
    }
    
    template<class S> bool operator==(const OwningPtr<S>& rhs) const {
        return ptr == rhs.getRawPtr();
    }
    
    template<class S> bool operator==(const S* ptr) const {
        return this->ptr == ptr;
    }
    
    int getRefCounter() const {
        if (isInvalid()) {
            return 0;
        } else {
            return HeapObjectRefManipulator::getHeapObjectCounters(ptr)->getWeakCounter()
                 + HeapObjectRefManipulator::getHeapObjectCounters(ptr)->getOwningCounter();
        }
    }
    
    template<class T2
            >
    static OwningPtr<T> dynamicCast(OwningPtr<T2> rhs) {
        if (rhs.isValid()) {
            OwningPtr<T> rslt;
            rslt.ptr = dynamic_cast<T*>(rhs.getRawPtr());
            if (rslt.ptr != NULL) {
                HeapObjectRefManipulator::incRefCounter(rslt.ptr);
            }
            return rslt;
        } else {
            return Null;
        }
    }
    
private:
    T* ptr;
};

} // namespace LucED

#endif // OWNING_PTR_HPP
