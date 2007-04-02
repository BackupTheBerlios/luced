/////////////////////////////////////////////////////////////////////////////////////
//
//   LucED - The Lucid Editor
//
//   Copyright (C) 2005-2006 Oliver Schmidt, oliver at luced dot de
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

#ifndef LUAOBJECT_H
#define LUAOBJECT_H

#include <stddef.h>
#include <string>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "HeapObject.h"
#include "HeapObjectArray.h"
#include "LuaStackChecker.h"
#include "LuaException.h"
#include "NonCopyable.h"

namespace LucED 
{

using std::string;

template<class ImplFunction> class LuaCFunction;
template<int N> class LuaFunctionArguments;

class LuaInterpreter;
class LuaObjectList;

class LuaObject
{
public:

    static LuaObject Table() {
        lua_newtable(L);
        return LuaObject(lua_gettop(L));
    }

    static LuaObject Nil() {
        lua_pushnil(L);
        return LuaObject(lua_gettop(L));
    }

    LuaObject() {
        lua_pushnil(L);
        stackIndex = lua_gettop(L);
    #ifdef DEBUG
        stackGeneration = LuaStackChecker::getInstance()->registerAndGetGeneration(stackIndex);
    #endif
        lua_checkstack(L, 10);
    }
        
    LuaObject(const LuaObject& rhs) {
        lua_pushvalue(L, rhs.stackIndex);
        stackIndex = lua_gettop(L);
    #ifdef DEBUG
        stackGeneration = LuaStackChecker::getInstance()->registerAndGetGeneration(stackIndex);
    #endif
        lua_checkstack(L, 10);
    }

    template<class ImplFunction> 
    LuaObject(LuaCFunction<ImplFunction> f);

    LuaObject& operator=(const LuaObject& rhs) {
        lua_pushvalue(L, rhs.stackIndex);
        lua_replace(L, stackIndex);
        return *this;
    }
    
    ~LuaObject() {
    #ifdef DEBUG
        LuaStackChecker::getInstance()->truncateGenerationAtStackIndex(stackGeneration, stackIndex);
    #endif
        lua_remove(L, stackIndex);
    }
    bool isValid() const {
        ASSERT(stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(stackGeneration));
        return stackIndex != -1 && !isNil();
    }
    bool isNil() const {
        ASSERT(stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(stackGeneration));
        return lua_isnil(L, stackIndex);
    }
    
    bool isBoolean() const {
        ASSERT(stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(stackGeneration));
        return lua_isboolean(L, stackIndex);
    }
    bool isNumber() const {
        ASSERT(stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(stackGeneration));
        return lua_isnumber(L, stackIndex);
    }
    bool isString() const {
        ASSERT(stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(stackGeneration));
        return lua_isstring(L, stackIndex);
    }
    bool isTable() const {
        ASSERT(stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(stackGeneration));
        return lua_istable(L, stackIndex);
    }
    bool isFunction() const {
        ASSERT(stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(stackGeneration));
        return lua_isfunction(L, stackIndex);
    }
    void setToNil() const {
        ASSERT(stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(stackGeneration));
        lua_pushnil(L);
        lua_replace(L, stackIndex);
    }
    void setToEmptyTable() const {
        ASSERT(stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(stackGeneration));
        lua_newtable(L);
        lua_replace(L, stackIndex);
    }
    
    const char* getTypeName() const {
        ASSERT(stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(stackGeneration));
        return lua_typename(L, lua_type(L, stackIndex));
    }
    
    bool toBoolean() const {
        return lua_toboolean(L, stackIndex);
    }
    double toNumber() const {
        return lua_tonumber(L, stackIndex);
    }
    string toString() const {
        size_t len;
        const char* ptr = lua_tolstring(L, stackIndex, &len);
        if (ptr != NULL) {
            return string(ptr, len);
        } else {
            return string();
        }
    }
    
    bool operator==(const LuaObject& rhs) const {
        ASSERT(    stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(    stackGeneration));
        ASSERT(rhs.stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(rhs.stackGeneration));
        return lua_equal(L, stackIndex, rhs.stackIndex);
    }
    bool operator!=(const LuaObject& rhs) const {
        return !(*this == rhs);
    }
    bool operator==(const char* rhs) const {
        int rhsLength = strlen(rhs);
        return isString()
            && rhsLength == lua_strlen(L, stackIndex)
            && memcmp(lua_tostring(L, stackIndex), rhs, rhsLength) == 0;
    }
    bool operator!=(const char* rhs) const {
        return !(*this == rhs);
    }
    
    bool operator==(const string& rhs) const {
        return isString() 
            && rhs.length() == lua_strlen(L, stackIndex)
            && memcmp(lua_tostring(L, stackIndex), rhs.c_str(), rhs.length()) == 0;
    }
    bool operator!=(const string& rhs) const {
        return !(*this == rhs);
    }
    
    template<class KeyType>
    class LuaObjectTableElementRef
    {
    public:

        void operator=(const LuaObject& newValue) {
            push(key);
            lua_pushvalue(LuaObject::L, newValue.stackIndex);
            lua_settable( LuaObject::L, tableStackIndex);
        }
        operator LuaObject() {
            push(key);
            lua_gettable( LuaObject::L, tableStackIndex);
            return LuaObject(lua_gettop(L));
        }
        bool isValid() const {
            push(key);
            lua_gettable(LuaObject::L, tableStackIndex);
            bool rslt = !lua_isnil(L, -1);
            lua_pop(LuaObject::L, 1);
            return rslt;
        }
        bool isNil() const {
            push(key);
            lua_gettable(LuaObject::L, tableStackIndex);
            bool rslt = lua_isnil(L, -1);
            lua_pop(LuaObject::L, 1);
            return rslt;
        }

        bool isBoolean() const {
            push(key);
            lua_gettable(LuaObject::L, tableStackIndex);
            bool rslt = lua_isboolean(L, -1);
            lua_pop(LuaObject::L, 1);
            return rslt;
        }
        bool isNumber() const {
            push(key);
            lua_gettable(LuaObject::L, tableStackIndex);
            bool rslt = lua_isnumber(L, -1);
            lua_pop(LuaObject::L, 1);
            return rslt;
        }
        bool isString() const {
            push(key);
            lua_gettable(LuaObject::L, tableStackIndex);
            bool rslt = lua_isstring(L, -1);
            lua_pop(LuaObject::L, 1);
            return rslt;
        }
        bool isTable() const {
            push(key);
            lua_gettable(LuaObject::L, tableStackIndex);
            bool rslt = lua_istable(L, -1);
            lua_pop(LuaObject::L, 1);
            return rslt;
        }
        bool isFunction() const {
            push(key);
            lua_gettable(LuaObject::L, tableStackIndex);
            bool rslt = lua_isfunction(L, -1);
            lua_pop(LuaObject::L, 1);
            return rslt;
        }

        bool toBoolean() const {
            push(key);
            lua_gettable(LuaObject::L, tableStackIndex);
            bool rslt = lua_toboolean(L, -1);
            lua_pop(LuaObject::L, 1);
            return rslt;
        }
        double toNumber() const {
            push(key);
            lua_gettable(LuaObject::L, tableStackIndex);
            double rslt = lua_tonumber(L, -1);
            lua_pop(LuaObject::L, 1);
            return rslt;
        }
        string toString() const {
            push(key);
            lua_gettable(LuaObject::L, tableStackIndex);
            string rslt = string(lua_tostring(L, -1),
                                 lua_strlen(  L, -1));
            lua_pop(LuaObject::L, 1);
            return rslt;
        }
        bool operator==(const LuaObject& rhs) const {
            push(key);
            lua_gettable(LuaObject::L, tableStackIndex);
            bool rslt = lua_equal(L, -1, rhs.stackIndex);
            lua_pop(LuaObject::L, 1);
            return rslt;
        }
        bool operator!=(const LuaObject& rhs) const {
            return !(*this == rhs);
        }
        bool operator==(const char* rhs) const {
            push(key);
            lua_gettable(LuaObject::L, tableStackIndex);
            int rhsLength = strlen(rhs);
            bool rslt = isString()
                     && rhsLength == lua_strlen(L, stackIndex)
                     && memcmp(lua_tostring(L, stackIndex), rhs, rhsLength) == 0;
            lua_pop(LuaObject::L, 1);
            return rslt;
        }
        bool operator!=(const char* rhs) const {
            return !(*this == rhs);
        }

        bool operator==(const string& rhs) const {
            push(key);
            lua_gettable(LuaObject::L, tableStackIndex);
            bool rslt = isString() 
                && rhs.length() == lua_strlen(L, stackIndex)
                && memcmp(lua_tostring(L, stackIndex), rhs.c_str(), rhs.length()) == 0;
            lua_pop(LuaObject::L, 1);
            return rslt;
        }
        bool operator!=(const string& rhs) const {
            return !(*this == rhs);
        }

    private:
        friend class LuaObject;

        LuaObjectTableElementRef(int tableStackIndex, KeyType key)
            : tableStackIndex(tableStackIndex),
              key(key)
        {}
        LuaObjectTableElementRef(const LuaObjectTableElementRef&);
        
        int     tableStackIndex;
        KeyType key;
    };
    
    LuaObjectTableElementRef<const char*> operator[](const char* fieldName) {
        ASSERT(stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(stackGeneration));
        return LuaObjectTableElementRef<const char*>(stackIndex, fieldName);
    }

    LuaObjectTableElementRef<const string&> operator[](const string& fieldName) {
        ASSERT(stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(stackGeneration));
        return LuaObjectTableElementRef<const string&>(stackIndex, fieldName);
    }

    LuaObjectTableElementRef<int> operator[](int index) {
        ASSERT(stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(stackGeneration));
        return LuaObjectTableElementRef<int>(stackIndex, index);
    }

    LuaObjectTableElementRef<const LuaObject&> operator[](const LuaObject& key) {
        ASSERT(stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(stackGeneration));
        return LuaObjectTableElementRef<const LuaObject&>(stackIndex, key);
    }
    
    class LuaFunctionArguments
    {
    public:
    
        LuaFunctionArguments()
        {
            ++refCounter;
            lua_checkstack(LuaObject::L, 20);
            if (refCounter == 1) {
                ASSERT(numberArguments == 0);
                lua_pushnil(LuaObject::L); // placeholder for function
            }
        #ifdef DEBUG
            if (refCounter == 1) {
                newestStackGeneration = LuaStackChecker::getInstance()->getNewestGeneration();
                highestStackIndex     = LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(newestStackGeneration);
                startStackIndex       = lua_gettop(LuaObject::L) - 1;
            } else {
                checkStack();
            }
        #endif
        }
        
        LuaFunctionArguments(const LuaFunctionArguments& rhs)
        {
        #ifdef DEBUG
            checkStack();
        #endif
            ++refCounter;
        }
        
        ~LuaFunctionArguments() {
            --refCounter;
            if (refCounter == 0 && numberArguments > 0) {
                #ifdef DEBUG
                    ASSERT(newestStackGeneration == LuaStackChecker::getInstance()->getNewestGeneration());
                    ASSERT(highestStackIndex     == LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(newestStackGeneration));
                #endif
                lua_pop(LuaObject::L, numberArguments + 1);
                numberArguments = 0;
            }
        }
        LuaFunctionArguments& operator<<(const LuaObject& arg)
        {
        #ifdef DEBUG
            checkStack();
        #endif
            lua_pushvalue(LuaObject::L, arg.stackIndex);
            ++numberArguments;
            if (numberArguments % 10 == 0) {
                lua_checkstack(LuaObject::L, 20);
            }
            return *this;
        }
        
        int getLength() const {
            return numberArguments;
        }
    
    private:
        friend class LuaObject;

    #ifdef DEBUG
        void checkStack() const {
            ASSERT(startStackIndex + 1 + numberArguments == lua_gettop(LuaObject::L));
            ASSERT(newestStackGeneration == LuaStackChecker::getInstance()->getNewestGeneration());
            ASSERT(highestStackIndex     == LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(newestStackGeneration));
        }
        static int  newestStackGeneration;
        static int  highestStackIndex;
        static int  startStackIndex;
    #endif
        static int numberArguments;
        static int refCounter;
    };
    

    LuaObject call(const LuaFunctionArguments& args = LuaFunctionArguments())
    {
    #ifdef DEBGUG
        args.checkStack();
        ASSERT(stackIndex <= LuaStackChecker::getInstance()->getHighestStackIndexForGeneration(stackGeneration));
    #endif
        lua_checkstack(L, 10);
        lua_pushvalue(L, stackIndex);

        lua_replace(L, -args.getLength() - 2);

        int error = lua_pcall(L, args.getLength(), 1, 0);
        args.numberArguments = 0;
        
        if (error != 0)
        {
            LuaException ex(lua_tostring(L, -1));
            lua_pop(L, 1);
            throw ex;
        } else {    
            return LuaObject(lua_gettop(L));
        }
    }
    
    operator LuaFunctionArguments() const {
        return LuaFunctionArguments() << *this;
    }
    
    LuaFunctionArguments operator<<(const LuaObject& rhs) const {
        return LuaFunctionArguments() << *this << rhs;
    }

private:
    friend class LuaInterpreter;
    friend class LuaIterator;
    friend class LuaStoredObject;
    friend class LuaCFunctionArguments;
    friend class LuaCFunctionResult;
    
    explicit LuaObject(int stackIndex) : stackIndex(stackIndex)
    {
    #ifdef DEBUG
        stackGeneration = LuaStackChecker::getInstance()->registerAndGetGeneration(stackIndex);
    #endif
        lua_checkstack(L, 10);
    }
    
    static void push(const char* arg) {
        lua_pushstring(L, arg);
    }
    static void push(const string& arg) {
        lua_pushlstring(L, arg.c_str(), arg.length());
    }
    static void push(int arg) {
        lua_pushnumber(L, arg);
    }
    static void push(const LuaObject& arg) {
        lua_pushvalue(L, arg.stackIndex);
    }


    static lua_State* L;
    
    int stackIndex;
#ifdef DEBUG
    int stackGeneration;
#endif
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool operator==(const char* lhs, const LuaObject& rhs)
{
    return rhs == lhs;
}

inline bool operator!=(const char* lhs, const LuaObject& rhs)
{
    return rhs != lhs;
}

inline bool operator==(const string& lhs, const LuaObject& rhs)
{
   return rhs == lhs;
}

inline bool operator!=(const string& lhs, const LuaObject& rhs)
{
   return rhs != lhs;
}

} // namespace LucED

#include "LuaCFunction.h"

namespace LucED
{

/**
 * Constructs LuaObject for CFunction.
 */
template<class ImplFunction>
LuaObject::LuaObject(LuaCFunction<ImplFunction>)
{
    lua_pushcfunction(L, &LuaCFunction<ImplFunction>::invokeFunction);
    stackIndex = lua_gettop(L);
#ifdef DEBUG
    stackGeneration = LuaStackChecker::getInstance()->registerAndGetGeneration(stackIndex);
#endif
    lua_checkstack(L, 10);
}

} // namespace LucED

#endif // LUAOBJECT_H
