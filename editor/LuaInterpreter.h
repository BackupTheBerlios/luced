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

#include <string>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}


#include "HeapObject.h"
#include "ObjectArray.h"
#include "OwningPtr.h"
#include "SingletonInstance.h"
#include "LuaObject.h"

namespace LucED
{

using std::string;

class LuaInterpreter : public HeapObject
{
public:

    /**
     * Gets the global LuaInterpreter.
     */
    static LuaInterpreter* getInstance() {
        return instance.getPtr();
    }
    
    
    string executeFile(string name);
    string executeScript(const char* beginScript, long scriptLength, string name = string());
    string executeScript(string script, string name = string()) {
        return executeScript(script.c_str(), script.length(), name);
    }
    LuaObject getGlobal(const char* name);
    void setGlobal(const char* name, LuaObject value);
    void clearGlobal(const char* name);
    
private:
    friend class SingletonInstance<LuaInterpreter>;
   
    ~LuaInterpreter();

    static SingletonInstance<LuaInterpreter> instance;

    LuaInterpreter();
    
    lua_State *L;
};



} // namespace LucED


#endif // LUAINTERPRETER_H
