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

#include "FindPanel.hpp"
#include "GuiLayoutColumn.hpp"
#include "GuiLayoutRow.hpp"
#include "GlobalConfig.hpp"
#include "GuiLayoutSpacer.hpp"
#include "LabelWidget.hpp"
#include "RegexException.hpp"
#include "SearchHistory.hpp"
#include "FindUtil.hpp"

using namespace LucED;

class FindPanel::PasteDataContentHandler : public PasteDataReceiver::ContentHandler
{
public:
    typedef OwningPtr<ContentHandler> Ptr;

    static Ptr create(ValidPtr<FindPanel> findPanel) {
        return Ptr(new PasteDataContentHandler(findPanel));
    }
    
    virtual void notifyAboutBeginOfPastingData() {
        findPanel->notifyAboutBeginOfPastingData();
    }
    virtual void notifyAboutReceivedPasteData(const byte* data, long length) {
        findPanel->notifyAboutReceivedPasteData(data, length);
    }
    virtual void notifyAboutEndOfPastingData() {
        findPanel->notifyAboutEndOfPastingData();
    }
private:
    PasteDataContentHandler(ValidPtr<FindPanel> findPanel)
        : findPanel(findPanel)
    {}
    ValidPtr<FindPanel> findPanel;
};



FindPanel::FindPanel(GuiWidget* parent, ValidPtr<TextEditorWidget> editorWidget, Callback<MessageBoxParameter>::Ptr messageBoxInvoker,
                                                                                 Callback<DialogPanel*>::Ptr        panelInvoker,
                                                                                 Callback<GuiWidget*>::Ptr          requestCloseCallback)
    : DialogPanel(parent, requestCloseCallback),

      pasteDataReceiver(PasteDataReceiver::create(this,
                                                  PasteDataContentHandler::create(this))),

      e(editorWidget),
      messageBoxInvoker(messageBoxInvoker),
      panelInvoker(panelInvoker),
      defaultDirection(Direction::DOWN),
      historyIndex(-1),
      selectSearchRegexFlag(false),
      selectionSearchForwardFlag(true),
      findUtil(e->getTextData())
{
    GuiLayoutColumn::Ptr  c0 = GuiLayoutColumn::create();
    GuiLayoutColumn::Ptr  c1 = GuiLayoutColumn::create();
    GuiLayoutColumn::Ptr  c2 = GuiLayoutColumn::create();
    GuiLayoutRow::Ptr     r0 = GuiLayoutRow::create();
    GuiLayoutRow::Ptr     r1 = GuiLayoutRow::create();
    GuiLayoutRow::Ptr     r2 = GuiLayoutRow::create();
    GuiLayoutRow::Ptr     r3 = GuiLayoutRow::create();
    GuiLayoutRow::Ptr     r4 = GuiLayoutRow::create();
    GuiLayoutRow::Ptr     r5 = GuiLayoutRow::create();
    GuiLayoutSpacerFrame::Ptr frame0 = GuiLayoutSpacerFrame::create(c0, 0);
    setRootElement(frame0);

    editFieldTextData = TextData::create();

    editField = SingleLineEditField::create(this, 
                                            GlobalConfig::getInstance()->getDefaultLanguageMode(),
                                            editFieldTextData);
    editField->setDesiredWidthInChars(5, 10, INT_MAX);
    findNextButton          = Button::create     (this, "N]ext");
    findPrevButton          = Button::create     (this, "P]revious");
    //goBackButton            = Button::create     (this, "Go B]ack");
    cancelButton            = Button::create     (this, "Cl]ose");
    LabelWidget::Ptr label0 = LabelWidget::create(this, "Find:");
    caseSensitiveCheckBox   = CheckBox::create   (this, "C]ase Sensitive");
    wholeWordCheckBox       = CheckBox::create   (this, "Wh]ole Word");
    regularExprCheckBox     = CheckBox::create   (this, "R]egular Expression");
    
    label0   ->setLayoutHeight(findPrevButton->getStandardHeight(), VerticalAdjustment::CENTER);
    editField->setLayoutHeight(findPrevButton->getStandardHeight(), VerticalAdjustment::CENTER);
 
    Callback<Button*>::Ptr buttonCallback           = newCallback(this, &FindPanel::handleButtonPressed);
    Callback<Button*>::Ptr buttonDefaultKeyCallback = newCallback(this, &FindPanel::handleButtonDefaultKey);
    
    findNextButton->setButtonPressedCallback(buttonCallback);
    findPrevButton->setButtonPressedCallback(buttonCallback);

    findNextButton->setButtonDefaultKeyCallback(buttonDefaultKeyCallback);
    findPrevButton->setButtonDefaultKeyCallback(buttonDefaultKeyCallback);

    //goBackButton->setButtonPressedCallback(buttonCallback);
    cancelButton->setButtonPressedCallback(buttonCallback);
    
    editField->setNextFocusWidget(findPrevButton);
    findPrevButton->setNextFocusWidget(findNextButton);
    findNextButton->setNextFocusWidget(caseSensitiveCheckBox);

    caseSensitiveCheckBox->setNextFocusWidget(wholeWordCheckBox);
    wholeWordCheckBox->setNextFocusWidget(regularExprCheckBox);
    regularExprCheckBox->setNextFocusWidget(cancelButton);

    //goBackButton->setNextFocusWidget(cancelButton);
    cancelButton->setNextFocusWidget(editField);

    setFocus(editField);

    Measures buttonMeasures;
             buttonMeasures.maximize(findPrevButton->getDesiredMeasures());
             buttonMeasures.maximize(findNextButton->getDesiredMeasures());
             //buttonMeasures.maximize(goBackButton  ->getDesiredMeasures());
             buttonMeasures.maximize(cancelButton  ->getDesiredMeasures());

    c0->addElement(r0);
    r0->addElement(c1);

    c1->addElement(r4);
    r4->addElement(label0);
    r4->addElement(editField);
    r4->addElement(findPrevButton);
    r4->addElement(findNextButton);

    c1->addElement(r3);
    r3->addElement(caseSensitiveCheckBox);
    r3->addElement(wholeWordCheckBox);
    r3->addElement(regularExprCheckBox);
    r3->addSpacer();
    r3->addElement(cancelButton);
    //r0->addElement(c2);
    //c2->addElement(r5);
    //c2->addElement(r1);
    //r1->addElement(goBackButton);
    
    caseSensitiveCheckBox->setChecked(false);
    
    findNextButton->setAsDefaultButton();

    findPrevButton->setDesiredMeasures(buttonMeasures);
    findNextButton->setDesiredMeasures(buttonMeasures);
    //goBackButton->setDesiredMeasures(buttonMeasures);
    cancelButton->setDesiredMeasures(buttonMeasures);
    
    label0->show();
    editField->show();
    findPrevButton->show();
    findNextButton->show();
    //goBackButton->show();
    cancelButton->show();
    caseSensitiveCheckBox->show();
    regularExprCheckBox->show();
    wholeWordCheckBox->show();
    
    editField->getTextData()->registerModifiedFlagListener(newCallback(this, &FindPanel::handleModifiedEditField));
    
    label0->setMiddleMouseButtonCallback(newCallback(editField, &SingleLineEditField::replaceTextWithPrimarySelection));
}

void FindPanel::treatFocusIn()
{
    setFocus(editField);
    DialogPanel::treatFocusIn();
}


void FindPanel::executeFind(bool isWrapping, Callback<>::Ptr handleContinueSearchButton)
{
    try
    {
/*        {
            if (e->hasSelectionOwnership()) {
                e->getBackliteBuffer()->deactivateSelection();
            }
            TextData::TextMark m = e->createNewMarkFromCursor();
            m.moveToPos(f.getTextPosition());
            e->moveCursorToTextMarkAndAdjustVisibility(m);
        }
*/
        GuiRoot::getInstance()->flushDisplay();

        findUtil.setAllowMatchAtStartOfSearchFlag(isWrapping);
        findUtil.findNext();

        if (findUtil.wasFound())
        {
            TextData::TextMark m = e->createNewMarkFromCursor();
            if (findUtil.isSearchingForward()) {
                m.moveToPos(findUtil.getMatchBeginPos());
            } else {
                m.moveToPos(findUtil.getMatchEndPos());
            }
            e->moveCursorToTextMarkAndAdjustVisibility(m);
            if (findUtil.isSearchingForward()) {
                m.moveToPos(findUtil.getMatchEndPos());
            } else {
                m.moveToPos(findUtil.getMatchBeginPos());
            }
            e->moveCursorToTextMarkAndAdjustVisibility(m);
            e->rememberCursorPixX();
            if (findUtil.getMatchBeginPos() < findUtil.getMatchEndPos())
            {
                e->setPrimarySelection(findUtil.getMatchBeginPos(),
                                       findUtil.getMatchEndPos());
            } else {
                e->releaseSelection();
            }
        } else if (!isWrapping) {
            if (findUtil.isSearchingForward()) {
                messageBoxInvoker->call(MessageBoxParameter()
                                        .setTitle("Not found")
                                        .setMessage("Continue search from beginning of file?")
                                        .setDefaultButton("C]ontinue", handleContinueSearchButton)
                                        .setCancelButton("Ca]ncel"));
            } else {
                messageBoxInvoker->call(MessageBoxParameter()
                                        .setTitle("Not found")
                                        .setMessage("Continue search from end of file?")
                                        .setDefaultButton("C]ontinue", handleContinueSearchButton)
                                        .setCancelButton("Ca]ncel"));
            }
        } else {
                messageBoxInvoker->call(MessageBoxParameter()
                                        .setTitle("Not found")
                                        .setMessage("String was not found"));
        }
    }
    catch (RegexException& ex)
    {
        panelInvoker->call(this);

        editField->getTextData()->setToString(findUtil.getSearchString());
        int position = ex.getPosition();
        if (position >= 0) {
            editField->setCursorPosition(position);
        }
        messageBoxInvoker->call(MessageBoxParameter()
                                .setTitle("Regex Error")
                                .setMessage(String() << "Error within regular expression: " << ex.getMessage()));
    }
    catch (LuaException& ex)
    {
        panelInvoker->call(this);

        editField->getTextData()->setToString(findUtil.getSearchString());

        messageBoxInvoker->call(MessageBoxParameter()
                                .setTitle("Lua Error")
                                .setMessage(ex.getMessage()));
    }
}

void FindPanel::findSelectionForward()
{
    requestClose();
    if (!e->areCursorChangesDisabled())
    {
        selectionSearchString.clear();
        selectionSearchForwardFlag = true;
        pasteDataReceiver->requestSelectionPasting();
    }
    e->assureCursorVisible();
}


void FindPanel::findSelectionBackward()
{
    requestClose();
    if (!e->areCursorChangesDisabled())
    {
        selectionSearchString.clear();
        selectionSearchForwardFlag = false;
        pasteDataReceiver->requestSelectionPasting();
    }
    e->assureCursorVisible();
}

void FindPanel::internalFindNext(bool isWrapping)
{
    if (findUtil.getSearchString().getLength() <= 0) {
        return;
    }

    SearchHistory::Entry newEntry;
                         newEntry.setFindString    (findUtil.getSearchString());
                         newEntry.setWholeWordFlag (findUtil.getWholeWordFlag());
                         newEntry.setRegexFlag     (findUtil.getRegexFlag());
                         newEntry.setIgnoreCaseFlag(findUtil.getIgnoreCaseFlag());
    SearchHistory::getInstance()->append(newEntry);

    bool forward = findUtil.getSearchForwardFlag();

    executeFind(isWrapping, forward ? newCallback(this, &FindPanel::handleContinueAtBeginningButton)
                                    : newCallback(this, &FindPanel::handleContinueAtEndButton));
}

void FindPanel::handleButtonPressed(Button* button)
{
    if (button == cancelButton)
    {
        requestClose();
    }
//    else if (button == goBackButton)
//    {
//        messageBoxInvoker->call(MessageBoxParameter().setTitle("xxx title xxx").setMessage("xxx message xxx"));
//    }
    else if (button == findNextButton || button == findPrevButton)
    {
        int textPosition = e->getCursorTextPosition();
        if (button == findNextButton) 
        {
            findPrevButton->setAsDefaultButton(false);
            findNextButton->setAsDefaultButton(true);
            
            if (e->hasPrimarySelection()) {
                findUtil.setTextPosition(e->getBeginSelectionPos());
                if (findUtil.doesMatch() && findUtil.getMatchEndPos() == e->getEndSelectionPos()) {
                    textPosition = e->getEndSelectionPos();
                }
            }
        } else {
            findPrevButton->setAsDefaultButton(true);
            findNextButton->setAsDefaultButton(false);

            if (e->hasPrimarySelection()) {
                findUtil.setTextPosition(e->getBeginSelectionPos());
                if (findUtil.doesMatch() && findUtil.getMatchEndPos() == e->getEndSelectionPos()) {
                    textPosition = e->getBeginSelectionPos();
                }
            }
        }
        findUtil.setTextPosition(textPosition);

        historyIndex = -1;

        findUtil.setSearchForwardFlag(button == findNextButton);
        findUtil.setIgnoreCaseFlag   (!caseSensitiveCheckBox->isChecked());
        findUtil.setRegexFlag        (regularExprCheckBox->isChecked());
        findUtil.setWholeWordFlag    (wholeWordCheckBox->isChecked());
        findUtil.setSearchString     (editField->getTextData()->getAsString());
        editField->getTextData()->setModifiedFlag(false);


        internalFindNext(false);
    }
}

void FindPanel::handleButtonDefaultKey(Button* button)
{
    if (button == findNextButton || button == findPrevButton) {
        requestClose();
    }
    handleButtonPressed(button);
}


void FindPanel::handleContinueSelectionFindAtBeginningButton()
{
    findUtil.setSearchForwardFlag(true);
    findUtil.setIgnoreCaseFlag   (true);
    findUtil.setRegexFlag        (selectSearchRegexFlag);
    findUtil.setWholeWordFlag    (false);
    findUtil.setSearchString     (selectionSearchString);
    findUtil.setTextPosition     (0);
    executeFind(true, newCallback(this, &FindPanel::handleContinueSelectionFindAtBeginningButton));
}

void FindPanel::handleContinueAtBeginningButton()
{
    ASSERT(findUtil.getSearchForwardFlag() == true);
    findUtil.setTextPosition(0);
    internalFindNext(true);
}


void FindPanel::handleContinueSelectionFindAtEndButton()
{
    findUtil.setSearchForwardFlag(false);
    findUtil.setIgnoreCaseFlag   (true);
    findUtil.setRegexFlag        (selectSearchRegexFlag);
    findUtil.setWholeWordFlag    (false);
    findUtil.setSearchString     (selectionSearchString);
    findUtil.setTextPosition     (e->getTextData()->getLength());
    executeFind(true, newCallback(this, &FindPanel::handleContinueSelectionFindAtEndButton));
}

void FindPanel::handleContinueAtEndButton()
{
    ASSERT(findUtil.getSearchForwardFlag() == false);
    findUtil.setTextPosition(e->getTextData()->getLength());
    internalFindNext(true);
}


void FindPanel::findAgainForward()
{
    if (this->isVisible() && editField->getTextData()->getLength() == 0) {
        return;
    }
    if (this->isVisible()) {
        findUtil.setIgnoreCaseFlag   (!caseSensitiveCheckBox->isChecked());
        findUtil.setRegexFlag        (regularExprCheckBox->isChecked());
        findUtil.setWholeWordFlag    (wholeWordCheckBox->isChecked());
        findUtil.setSearchString     (editField->getTextData()->getAsString());
        editField->getTextData()->setModifiedFlag(false);
        requestClose();
    }
    else
    {
        SearchHistory* history = SearchHistory::getInstance();
        if (history->getEntryCount() >= 1) {
            SearchHistory::Entry entry = history->getEntry(history->getEntryCount() - 1);

            findUtil.setIgnoreCaseFlag   (entry.getIgnoreCaseFlag());
            findUtil.setRegexFlag        (entry.getRegexFlag());
            findUtil.setWholeWordFlag    (entry.getWholeWordFlag());
            findUtil.setSearchString     (entry.getFindString());
        }
    }
    historyIndex = -1;

    int textPosition = e->getCursorTextPosition();
    if (e->hasPrimarySelection()) {
        findUtil.setTextPosition(e->getBeginSelectionPos());
        if (findUtil.doesMatch() && findUtil.getMatchEndPos() == e->getEndSelectionPos()) {
            textPosition = e->getEndSelectionPos();
        }
    }
    findUtil.setTextPosition(textPosition);
    
    findUtil.setSearchForwardFlag(true);
    
    internalFindNext(false);
}


void FindPanel::findAgainBackward()
{
    if (this->isVisible() && editField->getTextData()->getLength() == 0) {
        return;
    }

    if (this->isVisible()) {
        findUtil.setIgnoreCaseFlag   (!caseSensitiveCheckBox->isChecked());
        findUtil.setRegexFlag        (regularExprCheckBox->isChecked());
        findUtil.setWholeWordFlag    (wholeWordCheckBox->isChecked());
        findUtil.setSearchString     (editField->getTextData()->getAsString());
        editField->getTextData()->setModifiedFlag(false);
        requestClose();
    }
    else
    {
        SearchHistory* history = SearchHistory::getInstance();
        if (history->getEntryCount() >= 1) {
            SearchHistory::Entry entry = history->getEntry(history->getEntryCount() - 1);

            findUtil.setIgnoreCaseFlag   (entry.getIgnoreCaseFlag());
            findUtil.setRegexFlag        (entry.getRegexFlag());
            findUtil.setWholeWordFlag    (entry.getWholeWordFlag());
            findUtil.setSearchString     (entry.getFindString());
        }
    }
    historyIndex = -1;

    int textPosition = e->getCursorTextPosition();
    if (e->hasPrimarySelection()) {
        findUtil.setTextPosition(e->getBeginSelectionPos());
        if (findUtil.doesMatch() && findUtil.getMatchEndPos() == e->getEndSelectionPos()) {
            textPosition = e->getBeginSelectionPos();
        }
    }
    findUtil.setTextPosition(textPosition);

    findUtil.setSearchForwardFlag(false);
    
    internalFindNext(false);
}

    
GuiElement::ProcessingResult FindPanel::processEvent(const XEvent *event)
{
    if (pasteDataReceiver->processPasteDataReceiverEvent(event) == EVENT_PROCESSED) {
        return EVENT_PROCESSED;
    } else {
        return DialogPanel::processEvent(event);
    }
}


GuiElement::ProcessingResult FindPanel::processKeyboardEvent(const XEvent *event)
{
    KeyId pressedKey = KeyId(XLookupKeysym((XKeyEvent*)&event->xkey, 0));
    bool processed = false;

    KeyMapping::Id keyMappingId(event->xkey.state, pressedKey);

    if (KeyMapping::Id(0, KeyId("Up"))    == keyMappingId
     || KeyMapping::Id(0, KeyId("KP_Up")) == keyMappingId)
    {
        SearchHistory* history = SearchHistory::getInstance();

        if (historyIndex < 0) {
            int    editFieldLength  = editField->getTextData()->getLength();
            String editFieldContent = editField->getTextData()->getSubstring(0, editFieldLength);

            SearchHistory::Entry newEntry;
                                 newEntry.setFindString    (editFieldContent);
                                 newEntry.setWholeWordFlag (wholeWordCheckBox->isChecked());
                                 newEntry.setRegexFlag     (regularExprCheckBox->isChecked());
                                 newEntry.setIgnoreCaseFlag(!caseSensitiveCheckBox->isChecked());

            history->append(newEntry);
        }

        int h = historyIndex;
        if (h < 0) {
            h = history->getEntryCount();
        }
        --h;
        ValidPtr<TextData> textData = editField->getTextData();
        String editFieldContent = textData->getSubstring(0, textData->getLength());
        while (h >= 0) {
            SearchHistory::Entry entry = history->getEntry(h);
            String lastFindString = entry.getFindString();
            if (lastFindString != editFieldContent) {
                textData->setToString(lastFindString);
                textData->clearHistory();
                textData->setModifiedFlag(false);
                historyIndex = h;
                wholeWordCheckBox->setChecked(entry.getWholeWordFlag());
                regularExprCheckBox->setChecked(entry.getRegexFlag());
                caseSensitiveCheckBox->setChecked(!entry.getIgnoreCaseFlag());
                break;
            }
            --h;
        }
        processed = true;
    }
    else if (KeyMapping::Id(0, KeyId("Down"))    == keyMappingId
          || KeyMapping::Id(0, KeyId("KP_Down")) == keyMappingId)
    {
        SearchHistory* history      = SearchHistory::getInstance();
        ValidPtr<TextData> textData = editField->getTextData();

        if (historyIndex < 0) {
            int editFieldLength = editField->getTextData()->getLength();
            String editFieldContent = editField->getTextData()->getSubstring(0, editFieldLength);

            SearchHistory::Entry newEntry;
                                 newEntry.setFindString    (editFieldContent);
                                 newEntry.setWholeWordFlag (wholeWordCheckBox->isChecked());
                                 newEntry.setRegexFlag     (regularExprCheckBox->isChecked());
                                 newEntry.setIgnoreCaseFlag(!caseSensitiveCheckBox->isChecked());

            history->append(newEntry);

            historyIndex = history->getEntryCount() - 1;
        }

        if (historyIndex >= 0) {
            String editFieldContent = textData->getSubstring(0, textData->getLength());
            int h = historyIndex;
            bool found = false;
            while (h + 1 < history->getEntryCount()) {
                ++h;
                SearchHistory::Entry entry = history->getEntry(h);
                String nextFindString = entry.getFindString();
                if (nextFindString != editFieldContent) {
                    textData->setToString(nextFindString);
                    textData->clearHistory();
                    textData->setModifiedFlag(false);
                    historyIndex = h;
                    found = true;
                    wholeWordCheckBox->setChecked(entry.getWholeWordFlag());
                    regularExprCheckBox->setChecked(entry.getRegexFlag());
                    caseSensitiveCheckBox->setChecked(!entry.getIgnoreCaseFlag());
                    break;
                }
            }
            if (!found) {
                historyIndex = -1;
                textData->clear();
                caseSensitiveCheckBox->setChecked(false);
                regularExprCheckBox->setChecked(false);
                wholeWordCheckBox->setChecked(false);
            }
        }
        processed = true;
    }
    else if (KeyMapping::Id(ControlMask, KeyId("g")) == keyMappingId)
    {
        findAgainForward();
        processed = true;
    }
    else if (KeyMapping::Id(ShiftMask|ControlMask, KeyId("g")) == keyMappingId)
    {
        findAgainBackward();
        processed = true;
    }
    
    if (!processed) {
        return DialogPanel::processKeyboardEvent(event);
    } else {
        editField->showCursor();
        return EVENT_PROCESSED;
    }
}


void FindPanel::show()
{
    DialogPanel::show();
    if (editField->getTextData()->getModifiedFlag() == false) {
        editField->getTextData()->clear();
        editField->getTextData()->setModifiedFlag(false);
        historyIndex = -1;
        caseSensitiveCheckBox->setChecked(false);
        regularExprCheckBox->setChecked(false);
        wholeWordCheckBox->setChecked(false);
    }
}

void FindPanel::handleModifiedEditField(bool modifiedFlag)
{
    if (modifiedFlag == true)
    {
        historyIndex = -1;
    }
}


void FindPanel::notifyAboutBeginOfPastingData()
{
    selectionSearchString.clear();
}

void FindPanel::notifyAboutReceivedPasteData(const byte* data, long length)
{
    selectionSearchString.append(data, length);
}

void FindPanel::notifyAboutEndOfPastingData()
{
    if (selectionSearchString.getLength() > 0)
    {
        if (!e->areCursorChangesDisabled())
        {
            selectSearchRegexFlag = false;
            if (selectionSearchString.contains('\n')) {
                selectSearchRegexFlag = true;
                selectionSearchString = FindUtil::quoteRegexCharacters(selectionSearchString);
            }

            SearchHistory::Entry newEntry;
                                 newEntry.setFindString    (selectionSearchString);
                                 newEntry.setWholeWordFlag (false);
                                 newEntry.setRegexFlag     (selectSearchRegexFlag);
                                 newEntry.setIgnoreCaseFlag(true);
            SearchHistory::getInstance()->append(newEntry);

            findUtil.setSearchForwardFlag(selectionSearchForwardFlag);
            findUtil.setIgnoreCaseFlag   (true);
            findUtil.setRegexFlag        (selectSearchRegexFlag);
            findUtil.setWholeWordFlag    (false);
            findUtil.setSearchString     (selectionSearchString);
            if (e->hasPrimarySelection()) {
                if (selectionSearchForwardFlag) {
                    findUtil.setTextPosition(e->getEndSelectionPos());
                } else {
                    findUtil.setTextPosition(e->getBeginSelectionPos());
                }
            } else {
                findUtil.setTextPosition(e->getCursorTextPosition());
            }
            if (selectionSearchForwardFlag) {
                executeFind(false, newCallback(this, &FindPanel::handleContinueSelectionFindAtBeginningButton));
            } else {
                executeFind(false, newCallback(this, &FindPanel::handleContinueSelectionFindAtEndButton));
            }
        }
        e->assureCursorVisible();
    }
}


