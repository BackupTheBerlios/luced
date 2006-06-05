/////////////////////////////////////////////////////////////////////////////////////
//
//   LucED - The Lucid Editor
//
//   Copyright (C) 2005-2006 Oliver Schmidt, osch@luced.de
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

#ifndef SINGLETONINSTANCE_H
#define SINGLETONINSTANCE_H

#include "debug.h"
#include "WeakPtr.h"
#include "SingletonKeeper.h"
#include "OwningPtr.h"

namespace LucED {

template<class T> class SingletonInstance : NonCopyable
{
public:

    SingletonInstance() : wasInstantiated(false) {}

    T* getPtr()
    {
        if (!wasInstantiated)
        {
            instancePtr = SingletonKeeper::getInstance()->add(OwningPtr<T>(new T()));
            wasInstantiated = true;
        } 
        else 
        {
            ASSERT(instancePtr.isValid());
        }
        return instancePtr.getRawPtr();
    }

private:
    bool wasInstantiated;
    WeakPtr<T> instancePtr;
};

} // namespace LucED

#endif // SINGLETONINSTANCE_H
