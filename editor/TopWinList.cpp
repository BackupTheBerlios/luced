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

#include "EventDispatcher.hpp"
#include "TopWinList.hpp"
#include "Clipboard.hpp"
#include "GlobalConfig.hpp"

using namespace LucED;

SingletonInstance<TopWinList> TopWinList::instance;


void TopWinList::requestCloseChildWindow(TopWin *topWin)
{
    TopWinOwner::requestCloseChildWindow(topWin);
    
    if (getNumberOfChildWindows() == 0
     && (!GlobalConfig::getInstance()->shouldKeepRunningIfOwningClipboard() 
      || !Clipboard::getInstance()->hasClipboardOwnership()))
    {
        EventDispatcher::getInstance()->requestProgramTermination();
    }
    
}
