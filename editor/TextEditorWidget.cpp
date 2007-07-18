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

#include <ctype.h>
#include <X11/keysym.h>

#include "TextEditorWidget.hpp"
#include "util.hpp"
#include "EventDispatcher.hpp"
#include "GlobalConfig.hpp"
#include "Clipboard.hpp"

using namespace LucED;


TextEditorWidget::TextEditorWidget(GuiWidget*       parent, 
                                   TextStyles::Ptr  textStyles, 
                                   HilitedText::Ptr hilitedText, 
                                   int              borderWidth)
                                   
      : TextWidget(parent, textStyles, hilitedText, borderWidth),
        SelectionOwner(this),
        PasteDataReceiver(this),
        rememberedCursorPixX(0),
        scrollRepeatCallback(this, &TextEditorWidget::handleScrollRepeating),
        hasMovingSelection(false),
        cursorChangesDisabled(false),
        buttonPressedCounter(0),
        lastActionId(ACTION_UNSPECIFIED),
        currentActionId(ACTION_UNSPECIFIED)
{
    hasFocusFlag = false;
    addToXEventMask(ButtonPressMask|ButtonReleaseMask|ButtonMotionMask);
    getTextData()->activateHistory();
}


bool TextEditorWidget::isWordCharacter(unsigned char c)
{
    return c == '_' || isalnum(c);
}

bool TextEditorWidget::isCursorVisible()
{
    if (  (getCursorLineNumber() < getTopLineNumber())
       || (getCursorLineNumber() >= getTopLineNumber() + getNumberOfVisibleLines()))
    {
        return false;
    }
    long pixX = getCursorPixX();
    int spaceWidth = getTextStyles()->get(0)->getSpaceWidth();
    
    if (  pixX > 0
       && (   (pixX < getLeftPix() + spaceWidth)
           || (pixX > getRightPix() - spaceWidth)))
    {
        return false;
    }
    return true;
}

void TextEditorWidget::assureCursorVisible()
{
    if (getCursorLineNumber() < getTopLineNumber()) {
        setTopLineNumber(getCursorLineNumber());
    } else if (getCursorLineNumber() >= getTopLineNumber() + getNumberOfVisibleLines()) {
        setTopLineNumber(getCursorLineNumber() - getNumberOfVisibleLines() + 1);
    }
    long pixX = getCursorPixX();
    int spaceWidth = getTextStyles()->get(0)->getSpaceWidth();
    
    if (pixX < getLeftPix() + spaceWidth) {
        setLeftPix(pixX - spaceWidth);
    } else if (pixX > getRightPix() - spaceWidth) {
        long newLeftPix = spaceWidth 
                * util::roundedUpDiv(pixX - (getRightPix() - getLeftPix()) + spaceWidth, spaceWidth);
        setLeftPix(newLeftPix);
    }
}

void TextEditorWidget::assureSelectionVisible()
{
    if (getBackliteBuffer()->getEndSelectionLine() <= getTopLineNumber() + 1)
    {
        int newTopLineNumber = getBackliteBuffer()->getEndSelectionLine() - getNumberOfVisibleLines() / 3;
        if (newTopLineNumber < 0) {
            newTopLineNumber = 0;
        }
        setTopLineNumber(newTopLineNumber);
    }
    else if (getBackliteBuffer()->getBeginSelectionLine() >= getTopLineNumber() + getNumberOfVisibleLines()  - 1)
    {
        int newTopLineNumber = getBackliteBuffer()->getBeginSelectionLine() + getNumberOfVisibleLines() / 3 - getNumberOfVisibleLines();
        if (newTopLineNumber > getTextData()->getNumberOfLines() - getNumberOfVisibleLines()) {
            newTopLineNumber = getTextData()->getNumberOfLines() - getNumberOfVisibleLines();
        }
        setTopLineNumber(newTopLineNumber);
    }
}

void TextEditorWidget::adjustCursorVisibility()
{
    if (getCursorLineNumber() < getTopLineNumber()) {
        int newTopLineNumber = getCursorLineNumber() - getNumberOfVisibleLines() / 4;
        if (newTopLineNumber < 0) {
            newTopLineNumber = 0;
        }
        setTopLineNumber(newTopLineNumber);
    } else if (getCursorLineNumber() >= getTopLineNumber() + getNumberOfVisibleLines()) {
        int newTopLineNumber = getCursorLineNumber() - getNumberOfVisibleLines() + getNumberOfVisibleLines() / 4;
        if (newTopLineNumber + getNumberOfVisibleLines() > getTextData()->getNumberOfLines()) {
            newTopLineNumber = getTextData()->getNumberOfLines() - getNumberOfVisibleLines();
        }
        setTopLineNumber(newTopLineNumber);
    }
    long pixX = getCursorPixX();
    int spaceWidth = getTextStyles()->get(0)->getSpaceWidth();
    if (pixX < getLeftPix() + spaceWidth) {
        setLeftPix(pixX - spaceWidth);
    } else {
        long rightPix = getRightPix();
        if (rightPix - spaceWidth > 0 && pixX > rightPix - spaceWidth) {
            long newLeftPix = spaceWidth 
                    * util::roundedUpDiv(pixX - (rightPix - getLeftPix()) + spaceWidth, spaceWidth);
            setLeftPix(newLeftPix);
        }
    }
}

void TextEditorWidget::moveCursorToTextMarkAndAdjustVisibility(TextData::MarkHandle m)
{
    moveCursorToTextMark(m);
    adjustCursorVisibility();
}

void TextEditorWidget::moveCursorToTextPositionAndAdjustVisibility(int position)
{
    moveCursorToTextPosition(position);
    adjustCursorVisibility();
}

void TextEditorWidget::notifyAboutLostSelectionOwnership()
{
    if (getBackliteBuffer()->isSelectionPrimary()) {
        getBackliteBuffer()->deactivateSelection();
    }
}

void TextEditorWidget::notifyAboutBeginOfPastingData()
{
    disableCursorChanges();
    
    if (!beginPastingTextMark.isValid()) {
        beginPastingTextMark = createNewMarkFromCursor();
        pastingTextMark      = createNewMarkFromCursor();
    } else {
        pastingTextMark      = getTextData()->createNewMark(beginPastingTextMark);
    }
//    getBackliteBuffer()->activateSelection(pastingTextMark.getPos());
//    getBackliteBuffer()->makeSelectionToSecondarySelection();
}

void TextEditorWidget::notifyAboutReceivedPasteData(const byte* data, long length)
{
    if (length > 0) {
        bool first =  (pastingTextMark.getPos() == beginPastingTextMark.getPos());
        
        getTextData()->insertAtMark(pastingTextMark, data, length);
        pastingTextMark.moveToPos(pastingTextMark.getPos() + length);
        
        if (!first) {
            if (!getBackliteBuffer()->hasActiveSelection()
              || getBackliteBuffer()->getBeginSelectionPos() != beginPastingTextMark.getPos())
            {
                getBackliteBuffer()->activateSelection(beginPastingTextMark.getPos());
            }
            getBackliteBuffer()->makeSelectionToSecondarySelection();
            getBackliteBuffer()->extendSelectionTo(pastingTextMark.getPos());
        }
    }
}

void TextEditorWidget::notifyAboutEndOfPastingData()
{
    if (cursorChangesDisabled) {
        enableCursorChanges();
    }

    releaseSelectionOwnershipButKeepPseudoSelection();

    if (!getBackliteBuffer()->hasActiveSelection()
      || getBackliteBuffer()->getBeginSelectionPos() != beginPastingTextMark.getPos())
    {
        getBackliteBuffer()->activateSelection(beginPastingTextMark.getPos());
    }
    getBackliteBuffer()->makeSelectionToSecondarySelection();
    getBackliteBuffer()->extendSelectionTo(pastingTextMark.getPos());

    moveCursorToTextMark(pastingTextMark);
    assureCursorVisible();

    rememberedCursorPixX = getCursorPixX();
    getTextData()->setHistorySeparator();

    pastingTextMark = TextData::TextMark();
}


static inline MicroSeconds calculateScrollTime(int diffPix, int lineHeight)
{
    long microSecs = GlobalConfig::getInstance()->getScrollBarRepeatNextMicroSecs();
    long rslt = microSecs - (microSecs * diffPix)/(10*lineHeight);
    if (rslt < microSecs/10) 
        rslt = microSecs/10;
    return MicroSeconds(rslt);
}

GuiElement::ProcessingResult TextEditorWidget::processEvent(const XEvent *event)
{
    if (processSelectionOwnerEvent(event) == EVENT_PROCESSED || processPasteDataReceiverEvent(event) == EVENT_PROCESSED
            || TextWidget::processEvent(event) == EVENT_PROCESSED) {
        return EVENT_PROCESSED;
    } else {
        switch (event->type)
        {
            case ButtonPress:
            {
                showMousePointer();
                if (!hasFocusFlag) {
                    requestFocusFor(this);
                }
                if (event->xbutton.button == Button1)
                {
                    if (!cursorChangesDisabled)
                    {
                        if (buttonPressedCounter == 1 && event->xbutton.time - lastButtonPressedTime
                                < GlobalConfig::getInstance()->getDoubleClickMilliSecs()) {
                            wasDoubleClick = true;
                        } else {
                            buttonPressedCounter = 0;
                            wasDoubleClick = false;
                        }
                        ++buttonPressedCounter;
                        
                        int x = event->xbutton.x;
                        int y = event->xbutton.y;

                        long newCursorPos = getTextPosFromPixXY(x, y);

                        bool extendingSelection = (event->xbutton.state & ShiftMask != 0);
                        if (extendingSelection && !hasSelectionOwnership()) {
                            requestSelectionOwnership();
                            getBackliteBuffer()->activateSelection(getCursorTextPosition());
                        }

                        if (extendingSelection) {
                            bool toStart = 
                                abs(newCursorPos - getBackliteBuffer()->getBeginSelectionPos())
                              < abs(newCursorPos - getBackliteBuffer()->getEndSelectionPos());
                            if (toStart) {
                                getBackliteBuffer()->setAnchorToEndOfSelection();
                            } else {
                                getBackliteBuffer()->setAnchorToBeginOfSelection();
                            }
                        } else if (hasSelectionOwnership()) {
                            releaseSelectionOwnership();
                        } else if (getBackliteBuffer()->hasActiveSelection()) {
                            getBackliteBuffer()->deactivateSelection();
                        }

                        if (wasDoubleClick)
                        {
                            long doubleClickPos = getTextPosFromPixXY(x, y, false);
                            long p1 = doubleClickPos;
                            long p2 = p1;
                            TextData *textData = getTextData();
                            if (doubleClickPos < textData->getLength())
                            {
                                if (isWordCharacter(textData->getChar(p1))) {
                                    while (p1 > 0 && isWordCharacter(textData->getChar(p1 - 1))) {
                                        --p1;
                                    }
                                    while (p2 < textData->getLength() && isWordCharacter(textData->getChar(p2))) {
                                        ++p2;
                                    }
                                } else if (ispunct(textData->getChar(p1))) {
                                    while (p1 > 0 && ispunct(textData->getChar(p1 - 1))) {
                                        --p1;
                                    }
                                    while (p2 < textData->getLength() && ispunct(textData->getChar(p2))) {
                                        ++p2;
                                    }
                                }
                                if (extendingSelection) {
                                    if (getBackliteBuffer()->isAnchorAtBegin()) {
                                        getBackliteBuffer()->extendSelectionTo(p2);
                                        moveCursorToTextPosition(p2);
                                    } else {
                                        getBackliteBuffer()->extendSelectionTo(p1);
                                        moveCursorToTextPosition(p1);
                                    }
                                } else {
                                    if (p1 != p2) {
                                        requestSelectionOwnership();
                                        getBackliteBuffer()->activateSelection(p1);
                                        getBackliteBuffer()->extendSelectionTo(p2);
                                    }
                                    moveCursorToTextPosition(p2);
                                }
                            }
                        } else {
                            if (extendingSelection) {
                                getBackliteBuffer()->extendSelectionTo(newCursorPos);
                            } else {
                                // not here, activate Selection if mouse cursor move
                                // getBackliteBuffer()->activateSelection(newCursorPos);
                            }
                            moveCursorToTextPosition(newCursorPos);
                        }
                        assureCursorVisible();
                        rememberedCursorPixX = getCursorPixX();

                        this->movingSelectionY = y;
                        this->movingSelectionX = x;
                        this->hasMovingSelection = true;
                        this->isMovingSelectionScrolling = false;
                        lastButtonPressedTime = event->xbutton.time;
                        currentActionId = ACTION_UNSPECIFIED;
                    }
                    return EVENT_PROCESSED;
                }
                else if (event->xbutton.button == Button2)
                {
                    if (!cursorChangesDisabled)
                    {
                        int x = event->xbutton.x;
                        int y = event->xbutton.y;

                        long newCursorPos = getTextPosFromPixXY(x, y);
                        moveCursorToTextPosition(newCursorPos);
                        EditingHistory::SectionHolder::Ptr historySectionHolder = getTextData()->createHistorySection();
                        
                        requestSelectionPasting(createNewMarkFromCursor());
                        currentActionId = ACTION_UNSPECIFIED;
                        
                        /*if (hasSelectionOwnership()) {
                            releaseSelectionOwnership();
                        }*/
                    }
                    assureCursorVisible();
                    rememberedCursorPixX = getCursorPixX();
                }
                else if (event->xbutton.button == Button4)
                {
                    setTopLineNumber(getTopLineNumber() - 5);
                    if (hasMovingSelection) {
                        setNewMousePositionForMovingSelection(event->xbutton.x, event->xbutton.y);
                    }
                    return EVENT_PROCESSED;
                }
                else if (event->xbutton.button == Button5)
                {
                    if (getTopLineNumber() + 5 >= getTextData()->getNumberOfLines() - getNumberOfVisibleLines()) {
                        setTopLineNumber(getTextData()->getNumberOfLines() - getNumberOfVisibleLines());
                    } else {
                        setTopLineNumber(getTopLineNumber() + 5);
                    }
                    if (hasMovingSelection) {
                        setNewMousePositionForMovingSelection(event->xbutton.x, event->xbutton.y);
                    }
                    return EVENT_PROCESSED;
                }
                break;
            }
            case ButtonRelease:
            {
                showMousePointer();
                if (event->xbutton.button == Button1) {
                    if (hasMovingSelection) {
                        hasMovingSelection = false;
                        isMovingSelectionScrolling = false;
                        currentActionId = ACTION_UNSPECIFIED;
                        return EVENT_PROCESSED;
                    }
                }
                break;
            }
            case MotionNotify:
            {
                showMousePointer();
                if (hasMovingSelection) {
                    XEvent newEvent;
                    XSync(getDisplay(), False);
                    if (XCheckWindowEvent(getDisplay(), getWid(), ButtonMotionMask, &newEvent) == True) {
                        event = &newEvent;
                        while (XCheckWindowEvent(getDisplay(), getWid(), ButtonMotionMask, &newEvent) == True);
                    }

                    int x = event->xmotion.x;
                    int y = event->xmotion.y;
                    setNewMousePositionForMovingSelection(x, y);
                    currentActionId = ACTION_UNSPECIFIED;
                    return EVENT_PROCESSED;
                } else {
                    removeFromXEventMask(PointerMotionMask);
                }
                break;
            }
        }
        return propagateEventToParentWidget(event);
    }
}

void TextEditorWidget::setNewMousePositionForMovingSelection(int x, int y)
{
    if (hasMovingSelection)
    {
        this->movingSelectionY = y;
        this->movingSelectionX = x;
        long newCursorPos = getTextPosFromPixXY(x, y);

        if (wasDoubleClick) {
            long doubleClickPos = getTextPosFromPixXY(x, y, false);
            long p1 = doubleClickPos;
            long p2 = p1;
            TextData *textData = getTextData();

            if (doubleClickPos < textData->getLength())
            {
                if (isWordCharacter(textData->getChar(p1))) {
                    while (p1 > 0 && isWordCharacter(textData->getChar(p1 - 1))) {
                        --p1;
                    }
                    while (p2 < textData->getLength() && isWordCharacter(textData->getChar(p2))) {
                        ++p2;
                    }
                } else if (ispunct(textData->getChar(p1))) {
                    while (p1 > 0 && ispunct(textData->getChar(p1 - 1))) {
                        --p1;
                    }
                    while (p2 < textData->getLength() && ispunct(textData->getChar(p2))) {
                        ++p2;
                    }
                }
                if (p1 != getCursorTextPosition() || p2 != getCursorTextPosition())
                {
                    if (!hasSelectionOwnership()) {
                        requestSelectionOwnership();
                        getBackliteBuffer()->activateSelection(getCursorTextPosition());
                    }
                    if (p1 < getBackliteBuffer()->getSelectionAnchorPos()) {
                        if (getBackliteBuffer()->isAnchorAtBegin()) {
                            long p = getBackliteBuffer()->getSelectionAnchorPos();
                            if (isWordCharacter(textData->getChar(p))) {
                                while (p < textData->getLength() && isWordCharacter(textData->getChar(p))) {
                                    ++p;
                                }
                            } else if (ispunct(textData->getChar(p))) {
                                while (p < textData->getLength() && ispunct(textData->getChar(p))) {
                                    ++p;
                                }
                            }
                            getBackliteBuffer()->extendSelectionTo(p);
                            getBackliteBuffer()->setAnchorToEndOfSelection();
                        }
                        getBackliteBuffer()->extendSelectionTo(p1);
                        newCursorPos = p1;
                    } else {
                        if (!getBackliteBuffer()->isAnchorAtBegin()) {
                            long p = getBackliteBuffer()->getSelectionAnchorPos();
                            if (p > 0 && isWordCharacter(textData->getChar(p - 1))) {
                                while (p > 0 && isWordCharacter(textData->getChar(p - 1))) {
                                    --p;
                                }
                            } else if (p > 0 && ispunct(textData->getChar(p - 1))) {
                                while (p > 0 && ispunct(textData->getChar(p - 1))) {
                                   --p;
                                }
                            }
                            getBackliteBuffer()->extendSelectionTo(p);
                            getBackliteBuffer()->setAnchorToBeginOfSelection();
                        }
                        getBackliteBuffer()->extendSelectionTo(p2);
                        newCursorPos = p2;
                    }
                }
            }
        } else {
            if (newCursorPos != getCursorTextPosition())
            {
                if (!hasSelectionOwnership()) {
                    requestSelectionOwnership();
                    getBackliteBuffer()->activateSelection(getCursorTextPosition());
                }
                getBackliteBuffer()->extendSelectionTo(newCursorPos);
            }
        }
        moveCursorToTextPosition(newCursorPos);
        assureCursorVisible();
        rememberedCursorPixX = getCursorPixX();

        if (y < 0 ) {
            if (!isMovingSelectionScrolling) {
                scrollUp();
                isMovingSelectionScrolling = true;
                EventDispatcher::getInstance()->registerTimerCallback(Seconds(0), 
                        calculateScrollTime(-y, getLineHeight()),
                        scrollRepeatCallback);
            }
        } else if (y > getHeightPix()) {
            if (!isMovingSelectionScrolling) {
                scrollDown();
                isMovingSelectionScrolling = true;
                EventDispatcher::getInstance()->registerTimerCallback(Seconds(0), 
                        calculateScrollTime(y - getHeightPix(), getLineHeight()),
                        scrollRepeatCallback);
            }
        } else if (x < 0 ) {
            if (!isMovingSelectionScrolling) {
                scrollLeft();
                isMovingSelectionScrolling = true;
                EventDispatcher::getInstance()->registerTimerCallback(Seconds(0), 
                        calculateScrollTime(-x, getLineHeight()),
                        scrollRepeatCallback);
            }
        } else if (x > getPixWidth() && !getTextData()->isEndOfLine(newCursorPos)) {
            if (!isMovingSelectionScrolling) {
                scrollRight();
                isMovingSelectionScrolling = true;
                EventDispatcher::getInstance()->registerTimerCallback(Seconds(0), 
                        calculateScrollTime(x - getPixWidth(), getLineHeight()),
                        scrollRepeatCallback);
            }
        } else {
            isMovingSelectionScrolling = false;
        }
    }
}

void TextEditorWidget::releaseSelectionOwnership()
{
    if (getBackliteBuffer()->hasActiveSelection()) {
        getBackliteBuffer()->deactivateSelection();
    }
    SelectionOwner::releaseSelectionOwnership();
}

void TextEditorWidget::releaseSelectionOwnershipButKeepPseudoSelection()
{
    if (getBackliteBuffer()->hasActiveSelection()) {
        getBackliteBuffer()->makeSelectionToSecondarySelection();
    }
    if (hasSelectionOwnership()) {
        SelectionOwner::releaseSelectionOwnership();
    }
}

long  TextEditorWidget::initSelectionDataRequest()
{
    ASSERT(getBackliteBuffer()->hasActiveSelection());
    
    long selBegin = getBackliteBuffer()->getBeginSelectionPos();
    long selLength = getBackliteBuffer()->getEndSelectionPos() - selBegin;
    disableCursorChanges();
    return selLength;
}

const byte* TextEditorWidget::getSelectionDataChunk(long pos, long length)
{
    ASSERT(getBackliteBuffer()->hasActiveSelection());

    long selBegin = getBackliteBuffer()->getBeginSelectionPos();
    return getTextData()->getAmount(selBegin + pos, length);
}

void  TextEditorWidget::endSelectionDataRequest()
{
    enableCursorChanges();
}


void TextEditorWidget::handleScrollRepeating()
{
    if (isMovingSelectionScrolling) {
        long newCursorPos = getTextPosFromPixXY(movingSelectionX, movingSelectionY);
        if (newCursorPos != getCursorTextPosition())
        {
            if (!hasSelectionOwnership()) {
                requestSelectionOwnership();
                getBackliteBuffer()->activateSelection(getCursorTextPosition());
            }
            moveCursorToTextPosition(newCursorPos);
            assureCursorVisible();
            rememberedCursorPixX = getCursorPixX();
            getBackliteBuffer()->extendSelectionTo(getCursorTextPosition());
        }
        if (movingSelectionY < 0 ) {
            scrollUp();
            EventDispatcher::getInstance()->registerTimerCallback(Seconds(0), 
                    calculateScrollTime(-movingSelectionY, getLineHeight()),
                    scrollRepeatCallback);
        } else if (movingSelectionY > getHeightPix()) {
            scrollDown();
            EventDispatcher::getInstance()->registerTimerCallback(Seconds(0), 
                    calculateScrollTime(movingSelectionY - getHeightPix(), getLineHeight()),
                    scrollRepeatCallback);
        } else if (movingSelectionX < 0 ) {
            scrollLeft();
            EventDispatcher::getInstance()->registerTimerCallback(Seconds(0), 
                    calculateScrollTime(-movingSelectionX, getLineHeight()),
                    scrollRepeatCallback);
        } else if (movingSelectionX > getPixWidth() && !getTextData()->isEndOfLine(newCursorPos)) {
            scrollRight();
            EventDispatcher::getInstance()->registerTimerCallback(Seconds(0), 
                    calculateScrollTime(movingSelectionX - getPixWidth(), getLineHeight()),
                    scrollRepeatCallback);
        } else {
            isMovingSelectionScrolling = false;
        }
    }    
}


GuiElement::ProcessingResult TextEditorWidget::processKeyboardEvent(const XEvent *event)
{
    if (hasFocusFlag && event->type == KeyPress && !IsModifierKey(XLookupKeysym((XKeyEvent*)&event->xkey, 0)))
    {
        hideMousePointer();
    }
    
    unsigned int buttonState = event->xkey.state & (ControlMask|Mod1Mask|ShiftMask);
    Callback0 m = keyMapping.find(buttonState, XLookupKeysym((XKeyEvent*)&event->xkey, 0));

    if (m.isValid())
    {
        lastActionId = currentActionId;
        currentActionId = ACTION_UNSPECIFIED;
    
        m.call();
        return EVENT_PROCESSED;
    }
    else
    {
        
        char buffer[100];
        int len = XLookupString(&((XEvent*)event)->xkey, buffer, 100, NULL, NULL);
        if (len > 0) {
            if (!cursorChangesDisabled)
            {
                lastActionId = currentActionId;
                currentActionId = ACTION_KEYBOARD_INPUT;
                
                getTextData()->setMergableHistorySeparator();
                EditingHistory::SectionHolder::Ptr historySectionHolder = getTextData()->getHistorySectionHolder();
                
                hideCursor();
                if (hasSelectionOwnership())
                {
                    long selBegin = getBackliteBuffer()->getBeginSelectionPos();
                    long selLength = getBackliteBuffer()->getEndSelectionPos() - selBegin;
                    moveCursorToTextPosition(selBegin);
                    removeAtCursor(selLength);
                    releaseSelectionOwnership();
                }
                else if (getBackliteBuffer()->hasActiveSelection())
                {
                    if (   (lastActionId != ACTION_KEYBOARD_INPUT && lastActionId != ACTION_TABULATOR)
                        || getCursorTextPosition() != getBackliteBuffer()->getEndSelectionPos())
                    {
                        getBackliteBuffer()->deactivateSelection();
                    }
                }
                if (!getBackliteBuffer()->hasActiveSelection())
                {
                    getBackliteBuffer()->activateSelection(getCursorTextPosition());
                    getBackliteBuffer()->makeSelectionToSecondarySelection();
                }
                long insertedLength = insertAtCursor(buffer[0]);
                moveCursorToTextPosition(getCursorTextPosition() + insertedLength);

                getBackliteBuffer()->extendSelectionTo(getCursorTextPosition());

                if (getCursorLineNumber() < getTopLineNumber()) {
                    setTopLineNumber(getCursorLineNumber());
                } else if ((getCursorLineNumber() - getTopLineNumber() + 1) * getLineHeight() > getHeightPix()) {
                    setTopLineNumber(getCursorLineNumber() - getNumberOfVisibleLines() + 1);
                }
                long cursorPixX = getCursorPixX();
                int spaceWidth = getTextStyles()->get(0)->getSpaceWidth();
                if (cursorPixX < getLeftPix()) {
                    setLeftPix(cursorPixX - spaceWidth);
                } else if (cursorPixX >= getRightPix() - spaceWidth) {
                    setLeftPix(cursorPixX - (getRightPix() - getLeftPix()) + 2 * spaceWidth);
                }
                showCursor();
            }
            assureCursorVisible();
            rememberedCursorPixX = getCursorPixX();
            return EVENT_PROCESSED;
        } else {
            return NOT_PROCESSED;
        }
    }
}

void TextEditorWidget::disableCursorChanges()
{
    setCursorInactive();
    stopCursorBlinking();
    cursorChangesDisabled = true;
}

void TextEditorWidget::enableCursorChanges()
{
    if (hasFocusFlag) {
        setCursorActive();
        startCursorBlinking();
    }
    cursorChangesDisabled = false;
}

void TextEditorWidget::treatFocusIn()
{
    if (!cursorChangesDisabled) {
        setCursorActive();
        startCursorBlinking();
    }
    showMousePointer();
    hasFocusFlag = true;

    if (getBackliteBuffer()->hasActiveSelection()) {
        getBackliteBuffer()->turnOffSelectionPersistence();
#if 0
        if (requestSelectionOwnership()) {
            getBackliteBuffer()->makeSecondarySelectionToPrimarySelection();
        } else {
            getBackliteBuffer()->deactivateSelection();
        }
#endif
    }
}


void TextEditorWidget::treatFocusOut()
{
    setCursorInactive();
    stopCursorBlinking();
    showMousePointer();
    hasFocusFlag = false;

    if (getBackliteBuffer()->hasActiveSelection()) {
        getBackliteBuffer()->turnOnSelectionPersistence();
    }

}

void TextEditorWidget::showCursor()
{
    if (hasFocusFlag) {
        TextWidget::startCursorBlinking();
    } else {
        TextWidget::showCursor();
    }
}


void TextEditorWidget::hideCursor()
{
    TextWidget::hideCursor();
}

void TextEditorWidget::handleScrollStepV(ScrollStep::Type scrollStep)
{
    if (scrollStep == ScrollStep::LINE_UP) {
        scrollUp();
    }
    else if (scrollStep == ScrollStep::LINE_DOWN) {
        scrollDown();
    }
    else if (scrollStep == ScrollStep::PAGE_UP) {
        scrollPageUp();
    }
    else if (scrollStep == ScrollStep::PAGE_DOWN) {
        scrollPageDown();
    }
}

void TextEditorWidget::handleScrollStepH(ScrollStep::Type scrollStep)
{
    if (scrollStep == ScrollStep::LINE_UP) {
        scrollLeft();
    }
    else if (scrollStep == ScrollStep::LINE_DOWN) {
        scrollRight();
    }
    else if (scrollStep == ScrollStep::PAGE_UP) {
        scrollPageLeft();
    }
    else if (scrollStep == ScrollStep::PAGE_DOWN) {
        scrollPageRight();
    }
}

bool TextEditorWidget::scrollDown()
{
    if (this->getTopLineNumber() < this->getTextData()->getNumberOfLines() - this->getNumberOfVisibleLines()) {
        this->setTopLineNumber(this->getTopLineNumber() + 1);
        return true;
    } else {
        return false;
    }
}


bool TextEditorWidget::scrollUp()
{
    long topLine = this->getTopLineNumber();
    if (topLine > 0) {
        this->setTopLineNumber(this->getTopLineNumber() - 1);
        return true;
    } else {
        return false;
    }
}


void TextEditorWidget::scrollLeft()
{
    long newLeft = this->getLeftPix() - this->getTextStyles()->get(0)->getSpaceWidth();
    this->setLeftPix(newLeft);
}


void TextEditorWidget::scrollRight()
{
    long newLeft = this->getLeftPix() + this->getTextStyles()->get(0)->getSpaceWidth();
    this->setLeftPix(newLeft);
}


void TextEditorWidget::scrollPageUp()
{
    long targetTopLine = this->getTopLineNumber() - (this->getNumberOfVisibleLines() - 1);
    
    if (targetTopLine < 0) {
        targetTopLine = 0;
    }
    this->setTopLineNumber(targetTopLine);
}


void TextEditorWidget::scrollPageDown()
{
    long targetTopLine = this->getTopLineNumber() + this->getNumberOfVisibleLines() - 1;
    
    if (targetTopLine > this->getTextData()->getNumberOfLines() - this->getNumberOfVisibleLines()) {
        targetTopLine = this->getTextData()->getNumberOfLines() - this->getNumberOfVisibleLines();
    }
    this->setTopLineNumber(targetTopLine);
}


void TextEditorWidget::scrollPageLeft()
{
    int columns = this->getPixWidth() / this->getTextStyles()->get(0)->getSpaceWidth();
    long newLeft = this->getLeftPix() - this->getTextStyles()->get(0)->getSpaceWidth() * (columns/2);
    this->setLeftPix(newLeft);
}


void TextEditorWidget::scrollPageRight()
{
    int columns = this->getPixWidth() / this->getTextStyles()->get(0)->getSpaceWidth();
    long newLeft = this->getLeftPix() + this->getTextStyles()->get(0)->getSpaceWidth() * (columns/2);
    this->setLeftPix(newLeft);
}

TextEditorWidget::ActionId TextEditorWidget::getLastAction() const
{
    return lastActionId;
}

void TextEditorWidget::setCurrentAction(TextEditorWidget::ActionId actionId)
{
    this->currentActionId = actionId;
}

