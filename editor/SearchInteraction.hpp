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

#ifndef SEARCH_INTERACTION_HPP
#define SEARCH_INTERACTION_HPP

#include "HeapObject.hpp"
#include "OwningPtr.hpp"
#include "String.hpp"
#include "TextData.hpp"
#include "ReplaceUtil.hpp"
#include "SearchParameter.hpp"
#include "Callback.hpp"
#include "TextEditorWidget.hpp"
#include "MessageBoxParameter.hpp"
#include "MessageBoxQueue.hpp"
#include "ActionMethodBinding.hpp"

namespace LucED
{

class SearchInteraction : public HeapObject
{
public:
    typedef OwningPtr<SearchInteraction> Ptr;
    typedef SearchInteraction            ThisClass;
    
    class Callbacks
    {
    public:
        Callbacks(Callback<const MessageBoxParameter&>::Ptr        messageBoxInvoker,
                  MessageBoxQueue::Ptr                             messageBoxQueue,
                  Callback<ThisClass*>::Ptr                        requestCloseOfInvokingPanel,
                  Callback<>::Ptr                                  exceptionHandler,
                  DialogPanel*                                     hotKeyPredecessor)
            : messageBoxInvoker(messageBoxInvoker),
              messageBoxQueue(messageBoxQueue),
              requestCloseOfInvokingPanel(requestCloseOfInvokingPanel),
              exceptionHandler(exceptionHandler),
              hotKeyPredecessor(hotKeyPredecessor)
        {}
    private:
        friend class SearchInteraction;
        
        Callback<const MessageBoxParameter&>::Ptr       messageBoxInvoker;
        MessageBoxQueue::Ptr                            messageBoxQueue;
        Callback<ThisClass*>::Ptr                       requestCloseOfInvokingPanel;
        Callback<>::Ptr                                 exceptionHandler;
        WeakPtr<DialogPanel>                            hotKeyPredecessor;
    };
    
    static Ptr create(const SearchParameter& p, TextEditorWidget* e,
                                                const Callbacks& cb)
    {
        return Ptr(new SearchInteraction(p, e, cb));
    }
    
    ~SearchInteraction() {
        closeLastMessageBox();
    }
    void startFindSelection() {
        internalStartFind(true);
    }

    void startFind() {
        internalStartFind(false);
    }
    
    void replaceAndDontContinueWithFind() {
        internalReplaceAndFind(false, false);
    }
    
    void replaceAndContinueWithFind() {
        internalReplaceAndFind(true, false);
    }
    
    String getFindString() const {
        return replaceUtil.getFindString();
    }
    
    String getReplaceString() const {
        return replaceUtil.getReplaceString();
    }
    String getSubstitutedString() {
        return replaceUtil.getSubstitutedString();
    }

    bool replaceAllBetween(long spos, long epos) {
        return replaceUtil.replaceAllBetween(spos, epos);
    }
    
    SearchParameter getSearchParameter() const {
        return p;
    }
    
    bool isWaitingForContinue() const {
        return waitingForContinueFlag && waitingMessageBox.isValid();
    }
    
    void continueForwardAndKeepInvokingPanel();
    void continueBackwardAndKeepInvokingPanel();
    void replaceAndContinueForwardAndKeepInvokingPanel();
    void replaceAndContinueBackwardAndKeepInvokingPanel();

private:
    class MessageBoxActions : public ActionMethodBinding<MessageBoxActions>
    {
    public:
        enum Variant {
            VARIANT_NOT_FOUND,
            VARIANT_AUTO_CONTINUE_AT_END,
            VARIANT_AUTO_CONTINUE_AT_BEGIN
        };
        typedef OwningPtr<MessageBoxActions> Ptr;

        static Ptr create(RawPtr<SearchInteraction> thisInteraction, Variant variant) {
            return Ptr(new MessageBoxActions(thisInteraction, variant));
        }
        void findAgainForward() {
            closeInvokedMessageBox();
            switch (variant) {
                case VARIANT_NOT_FOUND:              thisInteraction->closeLastMessageBox(); return;
                case VARIANT_AUTO_CONTINUE_AT_END:   thisInteraction->findAgainForwardAndAutoContinue(); return;
                case VARIANT_AUTO_CONTINUE_AT_BEGIN: thisInteraction->findAgainForward(); return;
            }
        }
        void findAgainBackward() {
            closeInvokedMessageBox();
            switch (variant) {
                case VARIANT_NOT_FOUND:              thisInteraction->closeLastMessageBox(); return;
                case VARIANT_AUTO_CONTINUE_AT_END:   thisInteraction->findAgainBackward();   return;
                case VARIANT_AUTO_CONTINUE_AT_BEGIN: thisInteraction->findAgainBackwardAndAutoContinue(); return;
            }
        }
        void findSelectionForward() {
            closeInvokedMessageBox();
            switch (variant) {
                case VARIANT_NOT_FOUND:              thisInteraction->closeLastMessageBox(); return;
                case VARIANT_AUTO_CONTINUE_AT_END:   thisInteraction->findSelectionForwardAndAutoContinue(); return;
                case VARIANT_AUTO_CONTINUE_AT_BEGIN: thisInteraction->findSelectionForward(); return;
            }
        }
        void findSelectionBackward() {
            closeInvokedMessageBox();
            switch (variant) {
                case VARIANT_NOT_FOUND:              thisInteraction->closeLastMessageBox(); return;
                case VARIANT_AUTO_CONTINUE_AT_END:   thisInteraction->findSelectionBackward(); return;
                case VARIANT_AUTO_CONTINUE_AT_BEGIN: thisInteraction->findSelectionBackwardAndAutoContinue(); return;
            }
        }        
    private:
        MessageBoxActions(RawPtr<SearchInteraction> thisInteraction, Variant variant)
            : ActionMethodBinding<MessageBoxActions>(this),
              thisInteraction(thisInteraction),
              variant(variant)
        {}
        void closeInvokedMessageBox() {
            if (thisInteraction->invokedMessageBox.isValid()) {
                thisInteraction->invokedMessageBox->requestCloseWindow();
                thisInteraction->invokedMessageBox.invalidate();
            }
        }
        RawPtr<SearchInteraction> thisInteraction;
        Variant variant;
    };
    friend class ActionMethodBinding<MessageBoxActions>;

    SearchInteraction(const SearchParameter& p, TextEditorWidget* e,
                                                const Callbacks& cb)
        : p(p),
          e(e),
          cb(cb),
          textData(e->getTextData()),
          replaceUtil(textData),
          waitingForContinueFlag(false),
          continueForwardFlag(false)
    {}
    
    void setTextPositionForSearchStart();

    void continueWithFindSelection(String currentSelection, bool autoContinue);    
    void continueWithFindSelectionWithoutAutoContinue(String currentSelection);    
    void continueWithFindSelectionAndAutoContinue(String currentSelection);
    
    void findSelectionForward();
    void findSelectionForwardAndAutoContinue();
    void findSelectionBackward();
    void findSelectionBackwardAndAutoContinue();
    
    void internalStartFind(bool findSelection);
    void internalExecute(bool isWrapping, bool autoContinue);
    
    void internalReplaceAndFind(bool continueWithFind, bool autoContinue);
    
    void handleContinueAtBeginButton();
    void handleContinueAtEndButton();
    
    void findAgainForward();
    void findAgainForwardAndAutoContinue();
    void findAgainBackward();
    void findAgainBackwardAndAutoContinue();
    
    
    void notifyAboutInvokedMessageBox(TopWin* messageBox);
    void notifyAboutClosedMessageBox (TopWin* messageBox);
    
    void closeLastMessageBox();
    
    SearchParameter p;
    RawPtr<TextEditorWidget> e;
    
    Callbacks cb;

    TextData::Ptr textData;
    ReplaceUtil replaceUtil;
    
    bool waitingForContinueFlag;
    bool continueForwardFlag;

    WeakPtr<TopWin> waitingMessageBox;
    WeakPtr<TopWin> invokedMessageBox;
};

} // namespace LucED

#endif // SEARCH_INTERACTION_HPP
