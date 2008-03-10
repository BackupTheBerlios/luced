/////////////////////////////////////////////////////////////////////////////////////
//
//   LucED - The Lucid Editor
//
//   Copyright (C) 2005-2007 Oliver Schmidt, oliver at luced dot de
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

#ifndef GOTOLINEPANEL_H
#define GOTOLINEPANEL_H

#include "DialogPanel.hpp"
#include "Button.hpp"
#include "CheckBox.hpp"
#include "TextEditorWidget.hpp"
#include "SingleLineEditField.hpp"

namespace LucED {

class GotoLinePanel : public DialogPanel
{
public:
    typedef OwningPtr<GotoLinePanel> Ptr;

    static Ptr create(GuiWidget* parent, TextEditorWidget* editorWidget, Callback<GuiWidget*>::Ptr requestCloseCallback) {
        return Ptr(new GotoLinePanel(parent, editorWidget, requestCloseCallback));
    }
    
    virtual void treatFocusIn();
    
private:
    GotoLinePanel(GuiWidget* parent, TextEditorWidget* editorWidget, 
                                     Callback<GuiWidget*>::Ptr requestCloseCallback);

    void handleButtonPressed(Button* button);

    void filterInsert(const byte** buffer, long* length);

    Button::Ptr gotoButton;
    Button::Ptr cancelButton;
    SingleLineEditField::Ptr editField;
  
    WeakPtr<TextEditorWidget> editorWidget;
    ByteArray filterBuffer;
};

} // namespace LucED

#endif // GOTOLINEPANEL_H
