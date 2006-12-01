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

#include "MessageBox.h"
#include "GlobalConfig.h"
#include "GuiLayoutRow.h"
#include "GuiLayoutSpacer.h"
#include "GlobalConfig.h"
#include "LabelWidget.h"
#include "Callback.h"

using namespace LucED;

MessageBox::MessageBox(TopWin* referingWindow, MessageBoxParameter p)
    : PanelDialogWin(referingWindow)
{
    button1 = Button::create(this, "O]K");
   
    LabelWidget::Ptr label0 = LabelWidget::create(this, p.message);
    GuiLayoutColumn::Ptr column0 = GuiLayoutColumn::create();
    GuiLayoutRow::Ptr row0 = GuiLayoutRow::create();
    GuiLayoutSpacerFrame::Ptr frame0 = GuiLayoutSpacerFrame::create(column0, 10);
    setRootElement(frame0);

    column0->addSpacer();
    column0->addElement(label0);
    column0->addElement(GuiLayoutSpacer::create(0, 0, 0, 10, 0, INT_MAX));
    column0->addElement(row0);
    column0->addSpacer();

    row0->addElement(GuiLayoutSpacer::create(0, 0, 0, 0, INT_MAX, 0));
    row0->addElement(button1);
    row0->addElement(GuiLayoutSpacer::create(3, 0, 10, 0, 10, 0));
    //row0->addElement(cancelButton);
    row0->addElement(GuiLayoutSpacer::create(0, 0, 0, 0, INT_MAX, 0));
    
    Callback1<Button*> buttonCallback(this, &MessageBox::handleButtonPressed);
    button1->setButtonPressedCallback(buttonCallback);

    label0->show();
    button1->show();
    
    button1->setAsDefaultButton();
    setTitle(p.title);

    label0->setNextFocusWidget(button1);
    button1->setNextFocusWidget(label0);
    setFocus(label0);
    label0->setFakeFocus(true);
}


void MessageBox::handleButtonPressed(Button* button)
{
    if (button == button1)
    {
        requestCloseWindow();
    }
}

