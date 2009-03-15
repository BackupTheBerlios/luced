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

#include <X11/keysym.h>

#include "util.hpp"
#include "Callback.hpp"
#include "DialogWin.hpp"
#include "Button.hpp"
#include "EventDispatcher.hpp"
#include "GlobalConfig.hpp"

using namespace LucED;

DialogWin::DialogWin(TopWin* referingWindow)
    : wasNeverShown(true),
      referingWindow(referingWindow),
      shouldBeMapped(false),
      actionMethodContainer(ActionMethodContainer::create()),
      actionKeySequenceHandler(actionMethodContainer)
{
    if (referingWindow != NULL) {
        XSetTransientForHint(getDisplay(), getWid(), referingWindow->getWid());
        referingWindow->registerMappingNotifyCallback(newCallback(this, &DialogWin::notifyAboutReferingWindowMapping));
    }
    setBackgroundColor(getGuiRoot()->getGuiColor03());
}

void DialogWin::requestCloseWindowByUser()
{
    requestCloseWindow(TopWin::CLOSED_BY_USER);
}

void DialogWin::requestCloseWindow(TopWin::CloseReason reason)
{

    if (   GuiRoot::getInstance()->getX11ServerVendorString().startsWith("Hummingbird")
        && GuiRoot::getInstance()->getX11ServerVendorRelease() == 6100
        && reason == CLOSED_SILENTLY
        && !this->hasFocus()
        && referingWindow.isValid() && !referingWindow->isClosing()
        &&  this->isMapped()
        && !this->isClosing())
    {
        // strange workaround for exceed (vendor = <Hummingbird Communications Ltd.> <6100>)
        XSetInputFocus(getDisplay(), getWid(), RevertToNone, EventDispatcher::getInstance()->getLastX11Timestamp());
    }

    TopWin::requestCloseWindow(reason);
}

void DialogWin::setRootElement(OwningPtr<FocusableWidget> rootElement)
{
    this->rootElement = rootElement;
}

void DialogWin::prepareSizeHints()
{
    if (rootElement.isValid()) {
        Measures m = rootElement->getDesiredMeasures();
        if (wasNeverShown) {
            Position pp;
            if (referingWindow != NULL)  {
                pp = referingWindow->getAbsolutePosition();
            } else {
                pp = GuiRoot::getInstance()->getRootPosition();
            }
            int x = pp.x + (pp.w - m.bestWidth)/2;
            int y = pp.y + (pp.h - m.bestHeight)/2;
            setPosition(Position(x, y, m.bestWidth, m.bestHeight));
            setSizeHints(x, y, m.minWidth, m.minHeight, m.incrWidth, m.incrHeight);
        } else {
            setSizeHints(getPosition().x, getPosition().y, m.minWidth, m.minHeight, m.incrWidth, m.incrHeight);
        }
        wasNeverShown = false;
    }
}

void DialogWin::show()
{
    if (referingWindow.isValid() && !referingWindow->isMapped())
    {
        shouldBeMapped = true;
    }
    else
    {
        prepareSizeHints();
        TopWin::show();
    }
}

void DialogWin::notifyAboutReferingWindowMapping(bool isReferingWindowMapped)
{
    if (isReferingWindowMapped && !this->isVisible() && shouldBeMapped)
    {
        prepareSizeHints();
        TopWin::show();
    }
}


void DialogWin::treatNewWindowPosition(Position newPosition)
{
    if (rootElement.isValid())
    {
        TopWin::treatNewWindowPosition(newPosition);
        rootElement->setPosition(Position(0, 0, newPosition.w, newPosition.h));
    }
}





GuiElement::ProcessingResult DialogWin::processKeyboardEvent(const KeyPressEvent& keyPressEvent)
{
    bool processed = false;

    if (!processed) {
        processed = actionKeySequenceHandler.handleKeyPress(keyPressEvent, rootElement->getKeyActionHandler());
    }

    if (!processed && actionKeySequenceHandler.isWithinSequence())
    {
        actionKeySequenceHandler.reset();
        processed = true;
    }

    return processed ? EVENT_PROCESSED : NOT_PROCESSED;
}



void DialogWin::setReferingWindowForPositionHintsOnly(TopWin* referingWindow)
{
    ASSERT(!this->referingWindow.isValid());
    
    if (referingWindow != NULL)
    {
        this->referingWindow = referingWindow;
        referingWindow->registerMappingNotifyCallback(newCallback(this, &DialogWin::notifyAboutReferingWindowMapping));
    }
}


void DialogWin::treatFocusOut()
{
    actionKeySequenceHandler.reset();
}

void DialogWin::treatFocusIn()
{
    actionKeySequenceHandler.reset();
}


