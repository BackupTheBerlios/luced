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

#include <X11/keysym.h>

#include "FindPanel.hpp"
#include "GuiLayoutColumn.hpp"
#include "GuiLayoutRow.hpp"
#include "GlobalConfig.hpp"
#include "GuiLayoutSpacer.hpp"
#include "LabelWidget.hpp"
#include "RegexException.hpp"
#include "SearchHistory.hpp"

using namespace LucED;


FindPanel::FindPanel(RawPtr<TextEditorWidget> editorWidget, Callback<const MessageBoxParameter&>::Ptr messageBoxInvoker,
                                                            Callback<>::Ptr                           panelInvoker,
                                                            Callback<>::Ptr                           panelCloser)
    : BaseClass(panelCloser),

      e(editorWidget),
      messageBoxInvoker(messageBoxInvoker),
      panelInvoker(panelInvoker),
      defaultDirection(Direction::DOWN),
      historyIndex(-1),

      interactionCallbacks(messageBoxInvoker,
                           SearchHistory::getInstance()->getMessageBoxQueue(),
                           newCallback(this, &FindPanel::requestCloseFromInteraction),
                           newCallback(this, &FindPanel::handleException),
                           this)
      
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

    editField = SingleLineEditField::create(GlobalConfig::getInstance()->getDefaultLanguageMode(),
                                            editFieldTextData);
    editField->setDesiredWidthInChars(5, 10, INT_MAX);
    findNextButton          = Button::create     ("N]ext");
    findPrevButton          = Button::create     ("P]revious");
    //goBackButton            = Button::create     ("Go B]ack");
    cancelButton            = Button::create     ("Cl]ose");
    LabelWidget::Ptr label0 = LabelWidget::create("Find:");
    caseSensitiveCheckBox   = CheckBox::create   ("C]ase Sensitive");
    wholeWordCheckBox       = CheckBox::create   ("Wh]ole Word");
    regularExprCheckBox     = CheckBox::create   ("R]egular Expression");
    
    Callback<CheckBox*>::Ptr checkBoxCallback = newCallback(this, &FindPanel::handleCheckBoxPressed);
    
    caseSensitiveCheckBox->setButtonPressedCallback(checkBoxCallback);
    wholeWordCheckBox    ->setButtonPressedCallback(checkBoxCallback);
    regularExprCheckBox  ->setButtonPressedCallback(checkBoxCallback);
    
    label0   ->setLayoutHeight(findPrevButton->getStandardHeight(), VerticalAdjustment::CENTER);
    editField->setLayoutHeight(findPrevButton->getStandardHeight(), VerticalAdjustment::CENTER);
 
    Callback<Button*,Button::ActivationVariant>::Ptr buttonCallback = newCallback(this, &FindPanel::handleButtonPressed,
                                                                                        &FindPanel::handleException);
    findNextButton->setButtonPressedCallback(buttonCallback);
    findPrevButton->setButtonPressedCallback(buttonCallback);

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
    
    editField->getTextData()->registerModifiedFlagListener(newCallback(this, &FindPanel::handleModifiedEditField,
                                                                             &FindPanel::handleException));
    editField->getKeyActionHandler()->addActionMethods(EditFieldActions::create(this));
    
    label0->setMiddleMouseButtonCallback(newCallback(editField, &SingleLineEditField::replaceTextWithPrimarySelection));
}



void FindPanel::requestCloseFromInteraction(SearchInteraction* interaction)
{
    if (currentInteraction.getRawPtr() == interaction) {
        requestClose();
    }
}




void FindPanel::handleButtonPressed(Button* button, Button::ActivationVariant variant)
{
    if (variant == Button::WAS_DEFAULT_KEY || button == cancelButton)
    {
        requestClose();
    }
    
    if (button == findNextButton || button == findPrevButton)
    {
        if (button == findNextButton) 
        {
            findPrevButton->setAsDefaultButton(false);
            findNextButton->setAsDefaultButton(true);
            
        } else {
            findPrevButton->setAsDefaultButton(true);
            findNextButton->setAsDefaultButton(false);
        }

        SearchParameter p = getSearchParameterFromGuiControls()
                            .setSearchForwardFlag(button == findNextButton);
                            
        if (   currentInteraction.isValid()
            && currentInteraction->isWaitingForContinue() 
            && currentInteraction->getSearchParameter().findsSameThan(p))
        {
            if (button == findNextButton) {
                currentInteraction->continueForwardAndKeepInvokingPanel();
            } else {
                currentInteraction->continueBackwardAndKeepInvokingPanel();
            }
        }
        else
        {
            SearchHistory::getInstance()->getMessageBoxQueue()->closeQueued();

            historyIndex = -1;
    
            editField->getTextData()->setModifiedFlag(false);
    
            currentInteraction = SearchInteraction::create(p, e, interactionCallbacks);
            
            currentInteraction->startFind();
        }
    }
}

void FindPanel::invalidateOutdatedInteraction()
{
    SearchParameter p = getSearchParameterFromGuiControls();

    if (   currentInteraction.isValid()
        && currentInteraction->isWaitingForContinue() 
        && !currentInteraction->getSearchParameter().findsSameThan(p))
    {
        currentInteraction.invalidate();
    }
}

void FindPanel::handleCheckBoxPressed(CheckBox* checkBox)
{
    invalidateOutdatedInteraction();
}


void FindPanel::findAgainForward()
{
    internalFindAgain(true);
}

void FindPanel::findAgainBackward()
{
    internalFindAgain(false);
}

void FindPanel::internalFindAgain(bool forwardFlag)
{
    try
    {
        if (this->isMapped() && editField->getTextData()->getLength() == 0) {
            return;
        }
        
        SearchHistory::getInstance()->getMessageBoxQueue()->closeQueued();

        SearchParameter p;
        
        if (this->isMapped()) {
            p = getSearchParameterFromGuiControls();
            p.setSearchForwardFlag(forwardFlag);
            editField->getTextData()->setModifiedFlag(false);
            requestClose();
        }
        else
        {
            if (SearchHistory::getInstance()->hasEntries()) {
                p = SearchHistory::getInstance()->getSearchParameterFromLastEntry();
                p.setSearchForwardFlag(forwardFlag);
            } else {
                return;
            }
        }
        historyIndex = -1;
        
        currentInteraction = SearchInteraction::create(p, e, interactionCallbacks);
        
        currentInteraction->startFind();
    }
    catch (...)
    {
        handleException();
    }

}

void FindPanel::findSelectionForward()
{
    internalFindSelection(true);
}

void FindPanel::findSelectionBackward()
{
    internalFindSelection(false);
}


void FindPanel::internalFindSelection(bool forwardFlag)
{
    try
    {
        if (this->isMapped()) {
            requestClose();
        }

        SearchHistory::getInstance()->getMessageBoxQueue()->closeQueued();

        SearchParameter p;
                        p.setSearchForwardFlag(forwardFlag);
                        p.setIgnoreCaseFlag(true);
                        p.setRegexFlag(false);
                        p.setWholeWordFlag(false);

        historyIndex = -1;
        
        currentInteraction = SearchInteraction::create(p, e, interactionCallbacks);
        
        currentInteraction->startFindSelection();
    }
    catch (...)
    {
        handleException();
    }

}



void FindPanel::executeHistoryBackwardAction()
{
    SearchHistory* history = SearchHistory::getInstance();

    if (historyIndex < 0) {
        int    editFieldLength  = editField->getTextData()->getLength();
        String editFieldContent = editField->getTextData()->getHead(editFieldLength);

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
    RawPtr<TextData> textData = editField->getTextData();
    String editFieldContent = textData->getAsString();
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
}

void FindPanel::executeHistoryForwardAction()
{
    SearchHistory* history      = SearchHistory::getInstance();
    RawPtr<TextData> textData = editField->getTextData();

    if (historyIndex < 0) {
        String editFieldContent = editField->getTextData()->getAsString();

        SearchHistory::Entry newEntry;
                             newEntry.setFindString    (editFieldContent);
                             newEntry.setWholeWordFlag (wholeWordCheckBox->isChecked());
                             newEntry.setRegexFlag     (regularExprCheckBox->isChecked());
                             newEntry.setIgnoreCaseFlag(!caseSensitiveCheckBox->isChecked());

        history->append(newEntry);

        historyIndex = history->getEntryCount() - 1;
    }

    if (historyIndex >= 0) {
        String editFieldContent = textData->getAsString();
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
}



void FindPanel::show()
{
    setFocus(editField);
    BaseClass::show();
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
        invalidateOutdatedInteraction();
    }
}



void FindPanel::handleException()
{
    try
    {
        throw;
    }
    catch (RegexException& ex)
    {
        if (currentInteraction.isValid())
        {
            panelInvoker->call();
    
            editField->getTextData()->setToString(currentInteraction->getFindString());
            int position = ex.getPosition();
            if (position >= 0) {
                editField->setCursorPosition(position);
            }
        }
        messageBoxInvoker->call(MessageBoxParameter()
                                .setTitle("Regex Error")
                                .setMessage(String() << "Error within regular expression: " << ex.getMessage())
                                .setMessageBoxQueue(SearchHistory::getInstance()->getMessageBoxQueue()));
    }
    catch (LuaException& ex)
    {
        if (currentInteraction.isValid())
        {
            panelInvoker->call();
    
            editField->getTextData()->setToString(currentInteraction->getFindString());
        }
        messageBoxInvoker->call(MessageBoxParameter()
                                .setTitle("Lua Error")
                                .setMessage(ex.getMessage())
                                .setMessageBoxQueue(SearchHistory::getInstance()->getMessageBoxQueue()));
    }
}

