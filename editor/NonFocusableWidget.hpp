/////////////////////////////////////////////////////////////////////////////////////
//
//   LucED - The Lucid Editor
//
//   Copyright (C) 2005-2009 Oliver Schmidt, oliver at luced dot de
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

#ifndef NON_FOCUSABLE_WIDGET_HPP
#define NON_FOCUSABLE_WIDGET_HPP

#include "GuiWidget.hpp"
#include "Position.hpp"
#include "RawPtr.hpp"

namespace LucED
{

class NonFocusableWidget : public GuiElement,
                           public GuiWidget
{
public:
    virtual void treatNewWindowPosition(Position newPosition);
    virtual void show();
    virtual void hide();

protected:
    NonFocusableWidget(RawPtr<GuiWidget> parent, int x, int y, unsigned int width, unsigned int height, unsigned border_width)
        : GuiWidget(parent, x, y, width, height, border_width)
    {}
};

} // namespace LucED

#endif // NON_FOCUSABLE_WIDGET_HPP
