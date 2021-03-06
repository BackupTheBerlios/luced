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

#ifndef SELECTION_OWNER_HPP
#define SELECTION_OWNER_HPP

#include "WidgetId.hpp"
#include "GuiWidget.hpp"
#include "RawPtr.hpp"
#include "Nullable.hpp"
#include "RawPointable.hpp"

namespace LucED
{

class SelectionOwnerMultiPartState : public RawPointable
{
    friend class SelectionOwner;
    
    WidgetId multiPartTargetWid;
    Atom     multiPartTargetProp;
    Atom     sendingTypeAtom;
    long     selectionDataLength;
    long     alreadySentPos;
};


class SelectionOwner : public HeapObject
{
public:
    typedef OwningPtr<SelectionOwner> Ptr;

    enum Type
    {
        TYPE_PRIMARY,
        TYPE_CLIPBOARD
    };

    class ContentHandler : public HeapObject
    {
    public:
        typedef OwningPtr<ContentHandler> Ptr;
        
        virtual long        initSelectionDataRequest()                   = 0;
        virtual const byte* getSelectionDataChunk(long pos, long length) = 0;
        virtual void        endSelectionDataRequest()                    = 0;
        virtual void        notifyAboutLostSelectionOwnership()          = 0;
        virtual void        notifyAboutObtainedSelectionOwnership()      = 0;
    protected:
        ContentHandler()
        {}
    };

    static Ptr create(RawPtr<GuiWidget> baseWidget, Type type, ContentHandler::Ptr contentHandler) {
        return Ptr(new SelectionOwner(baseWidget, type, contentHandler));
    }
    ~SelectionOwner();
    
    GuiWidget::ProcessingResult processSelectionOwnerEvent(const XEvent *event);
    
    void requestSelectionOwnership();
    void releaseSelectionOwnership();

    bool hasSelectionOwnership() {
        checkSelectionOwnership();
        return hasSelectionOwnershipFlag;
    }
    
    static SelectionOwner* getPrimarySelectionOwner() { 
        checkPrimarySelectionOwnership();
        return primarySelectionOwner;
    }
    static bool hasPrimarySelectionOwner() {
        checkPrimarySelectionOwnership();
        return primarySelectionOwner != NULL;
    }

    class ReceiverAccess
    {
        friend class PasteDataReceiver;
        
        static long initSelectionDataRequest(SelectionOwner* o) {
            return o->contentHandler->initSelectionDataRequest();
        }
        static const byte* getSelectionDataChunk(SelectionOwner* o, long pos, long length) {
            return o->contentHandler->getSelectionDataChunk(pos, length);
        }
        static void endSelectionDataRequest(SelectionOwner* o) {
            o->contentHandler->endSelectionDataRequest();
        }
    };

private:
    SelectionOwner(RawPtr<GuiWidget> baseWidget, Type type, ContentHandler::Ptr contentHandler);

    void        stopCurrentMultiPartSending();
    void        checkSelectionOwnership();
    static void checkPrimarySelectionOwnership();
    
    bool isPrimary() const {
        return x11AtomForSelection == XA_PRIMARY;
    }

    RawPtr<GuiWidget> baseWidget;
    OwningPtr<ContentHandler> contentHandler;
    
    const Atom x11AtomForSelection;

    bool hasSelectionOwnershipFlag;

    typedef SelectionOwnerMultiPartState MultiPartState;
    
    Nullable<MultiPartState> multiPartState;
    
    Display* const display;
    const Atom x11AtomForTargets;
    const Atom x11AtomForIncr;
    const Atom x11AtomForUtf8String;
    
    static WeakPtr<SelectionOwner> primarySelectionOwner;
    static WeakPtr<SelectionOwner> primarySelectionOwnerCandidate;
    
    Time lastX11Timestamp;
};

} // namespace LucED

#endif // SELECTION_OWNER_HPP
