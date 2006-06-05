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

#ifndef TOPWIN_H
#define TOPWIN_H

#include <iostream>

#include "GuiWidget.h"
#include "WeakPtr.h"
#include "OwningPtr.h"
#include "ObjectArray.h"
#include "TopWinOwner.h"

namespace LucED {


class TopWin : public GuiWidget, public TopWinOwner
{
public:
    typedef WeakPtr<TopWin> Ptr;
    
    virtual ~TopWin();

    virtual bool processEvent(const XEvent *event);

    virtual bool processKeyboardEvent(const XEvent *event) {return false;}
    
    virtual void requestCloseWindow();
    virtual void treatNewWindowPosition(Position newPosition) {}
    virtual void treatFocusIn() {}
    virtual void treatFocusOut() {}

    void setTitle(const char* title);
    
        
protected:
    TopWin(int x, int y, unsigned int width, unsigned int height, unsigned border_width);
    
    void setSizeHints(int x, int y, int minWidth, int minHeight, int dx, int dy);
    template<class T> static WeakPtr<T> transferOwnershipTo(T *topWin, TopWinOwner* owner);
    template<class T> static WeakPtr<T> transferOwnershipTo(T *topWin, TopWin::Ptr owner);
    
private:
    void setWindowIcon();
    
    Atom x11InternAtomForDeleteWindow;
    TopWinOwner* owner;
};



template<class T> WeakPtr<T> TopWin::transferOwnershipTo(T *topWin, TopWinOwner* owner)
{
    OwningPtr<T>  rslt(topWin);
    owner->ownedTopWins.append(rslt);
    topWin->owner = owner;
    return rslt;
}

template<class T> WeakPtr<T> TopWin::transferOwnershipTo(T *topWin, TopWin::Ptr owner)
{
    return transferOwnershipTo(topWin, owner.getRawPtr());
}


} // namespace LucED

#endif // TOPWIN_H

