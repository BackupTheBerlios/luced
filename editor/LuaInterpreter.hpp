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

#ifndef LUAINTERPRETER_H
#define LUAINTERPRETER_H

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}


#include "HeapObject.hpp"
#include "ObjectArray.hpp"
#include "OwningPtr.hpp"
#include "SingletonInstance.hpp"
#include "LuaObject.hpp"
#include "LuaCFunctionArguments.hpp"
#include "LuaCFunctionResult.hpp"
#include "LuaCFunction.hpp"
#include "LuaStoredObject.hpp"
#include "LuaObjectList.hpp"
#include "String.hpp"

namespace LucED
{

class LuaInterpreter : public HeapObject
{
public:

    /**
     * Gets the global LuaInterpreter.
     */
    static LuaInterpreter* getInstance() {
        return instance.getPtr();
    }
    
    class Result
    {
    public:
        String        output;
        LuaObjectList objects;
    };    
    Result executeFile(String name);
    Result executeScript(const char* beginScript, long scriptLength, String name = String());
    Result executeExpression(const char* beginScript, long scriptLength, String name = String());

    Result executeExpression(const String& expr, String name = String()) {
        return executeExpression(expr.toCString(), expr.getLength(), name);
    }
    Result executeScript(String script, String name = String()) {
        return executeScript(script.toCString(), script.getLength(), name);
    }
    LuaObject getGlobal(const char* name);
    LuaObject getGlobal(const String& name) {
        return getGlobal(name.toCString());
    }
    void setGlobal(const char* name, LuaObject value);
    void clearGlobal(const char* name);
    
    template<class ImplFunction> 
    void setGlobal(const char* name, LuaCFunction<ImplFunction>)
    {
        lua_pushcfunction(L, &LuaCFunction<ImplFunction>::invokeFunction);
        lua_setglobal(L, name);
    }
    
private:
    friend class SingletonInstance<LuaInterpreter>;
    friend class LuaPrintFunction;
    friend class LuaStdoutWriteFunction;
    friend class LuaIoWriteFunction;
    friend class LuaIoOutputFunction;
    
    ~LuaInterpreter();

    static SingletonInstance<LuaInterpreter> instance;

    LuaInterpreter();
    
    lua_State *L;

    class StoredObjects : public HeapObject
    {
    public:
        typedef OwningPtr<StoredObjects> Ptr;
        
        static Ptr create() {
            return Ptr(new StoredObjects());
        }
      
        LuaStoredObject::Ptr originalToStringFunction;
        LuaStoredObject::Ptr originalIoOutputFunction;
        LuaStoredObject::Ptr originalIoWriteFunction;
        LuaStoredObject::Ptr lucedStdout;

    private:
        StoredObjects()
        {}
    };
    
    StoredObjects::Ptr storedObjects;
    bool isLucedStdoutActive;
    
    String printBuffer;
};



} // namespace LucED


#endif // LUAINTERPRETER_H