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

#include "String.h"

#include "EventDispatcher.h"
#include "SyntaxPatterns.h"
#include "LuaException.h"
#include "GlobalConfig.h"
#include "ConfigException.h"
#include "SingletonKeeper.h"
#include "EditorServer.h"
#include "HeapObjectArray.h"
#include "CommandlineException.h"
#include "FileException.h"

using namespace LucED;


int main(int argc, char **argv)
{
    int rc = 0;
    
    try
    {
        SingletonKeeper::Ptr singletonKeeper = SingletonKeeper::create();
        
        HeapObjectArray<String>::Ptr commandline = HeapObjectArray<String>::create();
        for (int argIndex = 1; argIndex < argc; ++argIndex)
        {
            commandline->append(String(argv[argIndex]));
        }

        GlobalConfig::getInstance()->readConfig("./config");

        EditorServer::getInstance()->startWithCommandline(commandline);

        EventDispatcher::getInstance()->doEventLoop();
        
    }
    catch (CommandlineException& ex)
    {
        fprintf(stderr, "CommandlineException: %s\n", ex.getMessage().toCString());
        rc = 1;
    }
    catch (LuaException& ex)
    {
        fprintf(stderr, "LuaException: %s\n", ex.getMessage().toCString());
        rc = 1;
    }
    catch (ConfigException& ex)
    {
        fprintf(stderr, "ConfigException: %s\n", ex.getMessage().toCString());
        rc = 1;
    }
    catch (FileException& ex)
    {
        fprintf(stderr, "FileException: %s\n", ex.getMessage().toCString());
        rc = 8;
    }
#ifdef DEBUG
    HeapObjectChecker::assertAllCleared();
#endif
    return rc;
}
