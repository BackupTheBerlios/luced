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

#ifndef REPLACE_PANEL_HPP
#define REPLACE_PANEL_HPP

#include "String.hpp"

#include "DialogPanel.hpp"
#include "Button.hpp"
#include "CheckBox.hpp"
#include "TextEditorWidget.hpp"
#include "SingleLineEditField.hpp"
#include "MessageBox.hpp"
#include "Callback.hpp"
#include "BasicRegex.hpp"
#include "types.hpp"
#include "EditFieldGroup.hpp"
#include "FindPanel.hpp"
#include "PasteDataCollector.hpp"
#include "SearchInteraction.hpp"
#include "ActionMethodBinding.hpp"

namespace LucED
{

class ReplacePanel : public  DialogPanel,
                     private FindPanelAccess
{
public:
    typedef DialogPanel              BaseClass;
    typedef OwningPtr<ReplacePanel>  Ptr;

    static Ptr create(TextEditorWidget* editorWidget, FindPanel* findPanel,
                      Callback<const MessageBoxParameter&>::Ptr messageBoxInvoker,
                      Callback<>::Ptr                           panelInvoker,
                      Callback<>::Ptr                           panelClose)
    {
        return Ptr(new ReplacePanel(editorWidget, findPanel, messageBoxInvoker, 
                                                             panelInvoker,
                                                             panelClose));
    }
    
    void replaceAgainForward();
    void replaceAgainBackward();
    void findAgainForward();
    void findAgainBackward();
    
    void setDefaultDirection(Direction::Type direction) {
        ASSERT(direction == Direction::UP || direction == Direction::DOWN);
        defaultDirection = direction;
        findPrevButton->setAsDefaultButton(direction != Direction::DOWN);
        findNextButton->setAsDefaultButton(direction == Direction::DOWN);
        replacePrevButton->setAsDefaultButton(false);
        replaceNextButton->setAsDefaultButton(false);
    }
    
    virtual void show();
    virtual void hide();
    
private:
    class EditFieldActions : public ActionMethodBinding<EditFieldActions>
    {
    public:
        typedef OwningPtr<EditFieldActions> Ptr;

        static Ptr create(RawPtr<ReplacePanel> thisReplacePanel) {
            return Ptr(new EditFieldActions(thisReplacePanel));
        }
        void historyBackward() {
            thisReplacePanel->executeHistoryBackwardAction();
        }
        void historyForward() {
            thisReplacePanel->executeHistoryForwardAction();
        }
    private:
        EditFieldActions(RawPtr<ReplacePanel> thisReplacePanel)
            : ActionMethodBinding<EditFieldActions>(this),
              thisReplacePanel(thisReplacePanel)
        {}
        RawPtr<ReplacePanel> thisReplacePanel;
    };

    friend class ActionMethodBinding<EditFieldActions>;
    friend class PasteDataCollector<ReplacePanel>;
    
    ReplacePanel(TextEditorWidget* editorWidget, FindPanel* findPanel,
                 Callback<const MessageBoxParameter&>::Ptr messageBoxInvoker,
                 Callback<>::Ptr                           panelInvoker,
                 Callback<>::Ptr                           panelClose);

    void executeHistoryBackwardAction();
    void executeHistoryForwardAction();

    SearchParameter getSearchParameterFromGuiControls() const {
        return SearchParameter().setIgnoreCaseFlag (!caseSensitiveCheckBox->isChecked())
                                .setRegexFlag      (regularExprCheckBox->isChecked())
                                .setWholeWordFlag  (wholeWordCheckBox->isChecked())
                                .setFindString     (   findEditField->getTextData()->getAsString())
                                .setReplaceString  (replaceEditField->getTextData()->getAsString());
    }

    void invalidateOutdatedInteraction();
    
    void requestCloseFromInteraction(SearchInteraction* interaction);

    void handleException();
    
    void handleButtonPressed(Button* button,      Button::ActivationVariant variant);
    void handleButtonRightClicked(Button* button, Button::ActivationVariant variant);
    void handleCheckBoxPressed(CheckBox* checkBox);

    void handleModifiedEditField(bool modifiedFlag);
    
    void internalFindAgain(bool forwardFlag);
    void internalReplaceAgain(bool forwardFlag);

    void forgetRememberedSelection();

    WeakPtr<TextEditorWidget> e;

    SingleLineEditField::Ptr findEditField;
    SingleLineEditField::Ptr replaceEditField;
    
    Button::Ptr findNextButton;
    Button::Ptr findPrevButton;
    Button::Ptr replaceNextButton;
    Button::Ptr replacePrevButton;
    Button::Ptr cancelButton;
    Button::Ptr replaceSelectionButton;
    Button::Ptr replaceWindowButton;
    CheckBox::Ptr caseSensitiveCheckBox;
    CheckBox::Ptr wholeWordCheckBox;
    CheckBox::Ptr regularExprCheckBox;
    Callback<const MessageBoxParameter&>::Ptr messageBoxInvoker;
    Callback<>::Ptr                           panelInvoker;
    BasicRegex regex;
    Direction::Type defaultDirection;
    int historyIndex;
    String selectionSearchString;
    bool selectSearchRegexFlag;
    EditFieldGroup::Ptr editFieldGroup;
    String rememberedSelection;

    WeakPtr<FindPanel> findPanel;

    SearchInteraction::Ptr         currentInteraction;
    SearchInteraction::Callbacks   interactionCallbacks;
};

} // namespace LucED

#endif // REPLACE_PANEL_HPP
