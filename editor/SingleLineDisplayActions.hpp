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

#ifndef SINGLE_LINE_DISPLAY_ACTIONS_HPP
#define SINGLE_LINE_DISPLAY_ACTIONS_HPP

#include "RawPtr.hpp"
#include "OwningPtr.hpp"
#include "TextEditorWidget.hpp"
#include "ActionMethodBinding.hpp"

namespace LucED
{

class SingleLineDisplayActions : public  ActionMethodBinding<SingleLineDisplayActions>
{
public:
    typedef OwningPtr<SingleLineDisplayActions> Ptr;
    
    static Ptr create(RawPtr<TextEditorWidget> editWidget) {
        return Ptr(new SingleLineDisplayActions(editWidget));
    }

    void scrollLeft();
    void scrollRight();
    void scrollPageLeft();
    void scrollPageRight();

    void cursorBeginOfText();
    void cursorEndOfText();

    void copyToClipboard();
    void selectAll();

private:
    SingleLineDisplayActions(RawPtr<TextEditorWidget> editWidget)
        : ActionMethodBinding<SingleLineDisplayActions>(this),
          e(editWidget)
    {}
    
    RawPtr<TextEditorWidget> e;
};

} // namespace LucED

#endif // SINGLE_LINE_DISPLAY_ACTIONS_HPP
