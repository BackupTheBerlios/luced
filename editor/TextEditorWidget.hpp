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

#ifndef TEXT_EDITOR_WIDGET_HPP
#define TEXT_EDITOR_WIDGET_HPP

#include "TextWidget.hpp"
#include "SelectionOwner.hpp"
#include "PasteDataReceiver.hpp"
#include "OwningPtr.hpp"
#include "WeakPtr.hpp"
#include "Callback.hpp"
#include "KeyModifier.hpp" 
#include "Flags.hpp"

namespace LucED
{

class ViewLuaInterface;

class TextEditorWidget : public TextWidget
{
public:
    typedef TextWidget                  BaseClass;
    typedef OwningPtr<TextEditorWidget> Ptr;
    
    static const int BORDER_WIDTH = 4;
    
    enum ActionCategory
    {
        ACTION_UNSPECIFIED,
        ACTION_TABULATOR,
        ACTION_NEWLINE,
        ACTION_KEYBOARD_INPUT
    };
    
    enum PasteParameter
    {
        CURSOR_TO_BEGIN_OF_PASTED_DATA,
        CURSOR_TO_END_OF_PASTED_DATA
    };

    static Ptr create(HilitedText::Ptr hilitedText,
                      CreateOptions    options = CreateOptions())
    {
        return Ptr(new TextEditorWidget(hilitedText, options));
    }

    void connectToVerticalScrollBar(ScrollBar::Ptr scrollBar)
    {
        scrollBar->setChangedValueCallback                   (newCallback(this, &TextEditorWidget::setTopLineNumber));
        scrollBar->setScrollStepCallback                     (newCallback(this, &TextEditorWidget::handleScrollStepV));
        this->setScrollBarVerticalValueRangeChangedCallback  (newCallback(scrollBar, &ScrollBar::setValueRange));
    }
    void connectToHorizontalScrollBar(ScrollBar::Ptr scrollBar)
    {
        scrollBar->setChangedValueCallback                   (newCallback(this, &TextEditorWidget::setLeftPix));
        scrollBar->setScrollStepCallback                     (newCallback(this, &TextEditorWidget::handleScrollStepH));
        this->setScrollBarHorizontalValueRangeChangedCallback(newCallback(scrollBar, &ScrollBar::setValueRange));
    }

    ActionCategory getLastActionCategory() const;
    void setCurrentActionCategory(ActionCategory action);

    bool isCursorBound() const {
        return boundCursorFlag;
    }    
    bool isCursorVisible();
    void assureCursorVisible();
    void assureSelectionVisible();
    void moveCursorToTextMarkAndAdjustVisibility(TextData::MarkHandle m);
    void moveCursorToTextPositionAndAdjustVisibility(int position);
    void adjustCursorVisibility();
    
    void displayCursorInSelectedLine(int lineNumber);
    
    void showCursor();
    void hideCursor();

//    virtual ProcessingResult processKeyboardEvent(const KeyPressEvent& keyPressEvent);
    
    virtual void treatFocusIn();
    virtual void treatFocusOut();

    void disableCursorChanges();
    void enableCursorChanges();
    bool areCursorChangesDisabled()  { return cursorChangesDisabled; }
    void rememberCursorPixX()        { rememberedCursorPixX = getCursorPixX(); }
    int  getRememberedCursorPixX()   { return rememberedCursorPixX; }
    bool isWordCharacter(int c);
    
    bool scrollUp();
    bool scrollDown();
    void scrollLeft();
    void scrollRight();
    void scrollPageUp();
    void scrollPageDown();
    void scrollPageLeft();
    void scrollPageRight();

    void handleScrollStepV(ScrollStep::Type scrollStep);
    void handleScrollStepH(ScrollStep::Type scrollStep);

    void showMousePointer() {
        BaseClass::showMousePointer();
    }
    void hideMousePointer() {
        BaseClass::hideMousePointer();
        getGuiWidget()->addToXEventMask(PointerMotionMask);
    }
    
    bool isReadOnly() const {
        return readOnlyFlag || textData->isReadOnly();
    }
    bool hasSelection() {
        return getBackliteBuffer()->hasActiveSelection();
    }
    bool hasPseudoSelection() {
        return getBackliteBuffer()->hasActiveSelection() && !getBackliteBuffer()->isSelectionPrimary();
    }
    bool hasPrimarySelection() {
        return getBackliteBuffer()->hasActiveSelection() && getBackliteBuffer()->isSelectionPrimary();
    }
    long getBeginSelectionPos() {
        return getBackliteBuffer()->getBeginSelectionPos();
    }
    long getEndSelectionPos() {
        return getBackliteBuffer()->getEndSelectionPos();
    }
    void setPrimarySelection(long anchorPos, long extendingPos) {
        selectionOwner->requestSelectionOwnership();
        getBackliteBuffer()->activateSelection(anchorPos);
        getBackliteBuffer()->extendSelectionTo(extendingPos);
    }
    void setPseudoSelection(long anchorPos, long extendingPos) {
        selectionOwner->releaseSelectionOwnership();
        getBackliteBuffer()->activateSelection(anchorPos);
        getBackliteBuffer()->extendSelectionTo(extendingPos);
        getBackliteBuffer()->makeSelectionToSecondarySelection();
    }
    void moveSelectionBeginTo(long beginPos) {
        bool wasAnchorAtBegin = getBackliteBuffer()->isAnchorAtBegin();
        getBackliteBuffer()->setAnchorToEndOfSelection();
        getBackliteBuffer()->extendSelectionTo(beginPos);
        if (wasAnchorAtBegin) {
            getBackliteBuffer()->setAnchorToBeginOfSelection();
        }
    }
    void moveSelectionEndTo(long endPos) {
        bool wasAnchorAtEnd = !getBackliteBuffer()->isAnchorAtBegin();
        getBackliteBuffer()->setAnchorToBeginOfSelection();
        getBackliteBuffer()->extendSelectionTo(endPos);
        if (wasAnchorAtEnd) {
            getBackliteBuffer()->setAnchorToEndOfSelection();
        }
    }
    void setAnchorToBeginOfSelection() {
        getBackliteBuffer()->setAnchorToBeginOfSelection();
    }
    void setAnchorToEndOfSelection() {
        getBackliteBuffer()->setAnchorToEndOfSelection();
    }
    bool isAnchorAtBeginOfSelection() {
        return getBackliteBuffer()->isAnchorAtBegin();
    }
    TextData::TextMark getNewMarkToBeginOfSelection() {
        return getBackliteBuffer()->createMarkToBeginOfSelection();
    }
    TextData::TextMark getNewMarkToEndOfSelection() {
        return getBackliteBuffer()->createMarkToEndOfSelection();
    }
    void makePseudoSelectionToPrimary() {
        selectionOwner->requestSelectionOwnership();
        getBackliteBuffer()->makeSecondarySelectionToPrimarySelection();
    }
    void extendSelectionTo(long pos) {
        getBackliteBuffer()->extendSelectionTo(pos);
    }
    long getSelectionByteLength() {
        return getBackliteBuffer()->getSelectionByteLength();
    }
    
    void releaseSelection();
    void releaseSelectionButKeepPseudoSelection();
    
    void requestClipboardPasting(const TextData::TextMark& m, 
                                 PasteParameter p = CURSOR_TO_END_OF_PASTED_DATA)
    {
        if (pasteDataReceiver.isValid() && !isReadOnly())
        {
            beginPastingTextMark = getTextData()->createNewMark(m);
            pasteParameter = p;
            pasteDataReceiver->requestClipboardPasting();
        }
    }

    void requestSelectionPasting(const TextData::TextMark& m,
                                 PasteParameter p = CURSOR_TO_END_OF_PASTED_DATA)
    {
        if (pasteDataReceiver.isValid() && !isReadOnly())
        {
            beginPastingTextMark = getTextData()->createNewMark(m);
            pasteParameter = p;
            pasteDataReceiver->requestPrimarySelectionPasting();
        }
    }

    void replaceTextWithPrimarySelection();
    void replaceSelection(const String& newContent);
    
    void registerListenerForNextSelectionChange(Callback<>::Ptr callback) {
        getBackliteBuffer()->registerListenerForNextChange(callback);
    }
    
    RawPtr<ViewLuaInterface> getViewLuaInterface();
    
private:
    virtual void processGuiWidgetCreatedEvent();

public: // GuiWidget::EventListener interface implementation
    virtual GuiWidget::ProcessingResult processGuiWidgetEvent(const XEvent* event);

protected:
    TextEditorWidget(HilitedText::Ptr hilitedText, 
                     CreateOptions    options = CreateOptions(),
                     int              borderWidth = BORDER_WIDTH);
    
    ~TextEditorWidget();
    
    void notifyAboutReceivedPasteData(const byte* data, long length);
    void notifyAboutEndOfPastingData();
    void notifyAboutBeginOfPastingData();

private:
    class MyKeyActionHandler; 

    void treatConfigUpdate();
    void treatLanguageModeChange(LanguageMode::Ptr newLanguageMode);
    
    bool handleLowPriorityKeyPress(const KeyPressEvent& keyPressEvent);
    
    void setNewMousePositionForMovingSelection(int x, int y);
    
    long rememberedCursorPixX;
    
    bool hasMovingSelection;
    int movingSelectionX;
    int movingSelectionY;
    bool isMovingSelectionScrolling;
    Callback<>::Ptr scrollRepeatCallback;
    void handleScrollRepeating();
    
    bool cursorChangesDisabled;
    
    Time lastButtonPressedTime;
    int buttonPressedCounter;
    bool wasDoubleClick;
    ActionCategory lastActionCategory;
    ActionCategory currentActionCategory;
    TextData::TextMark beginPastingTextMark;
    TextData::TextMark pastingTextMark;
    TextData::HistorySection::Ptr pastingDataHistorySectionHolder;
    PasteParameter pasteParameter;

    class SelectionContentHandler;
    SelectionOwner::Ptr selectionOwner;
    
    bool isSelectionPersistent;

    RawPtr<TextData> textData;

    class PasteDataContentHandler;
    PasteDataReceiver::Ptr pasteDataReceiver;
    
    bool readOnlyFlag;

    bool boundCursorFlag;

    OwningPtr<ViewLuaInterface> viewLuaInterface;
};

} // namespace LucED

#endif // TEXT_EDITOR_WIDGET_HPP
