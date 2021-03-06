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

#ifndef SCROLL_BAR_HPP
#define SCROLL_BAR_HPP

#include "NonFocusableWidget.hpp"
#include "Callback.hpp"
#include "types.hpp"
#include "OwningPtr.hpp"

namespace LucED
{

class ScrollBar : public NonFocusableWidget
{
public:

    typedef OwningPtr<ScrollBar> Ptr;

    static ScrollBar::Ptr create(Orientation::Type orientation)
    {
        return ScrollBar::Ptr(new ScrollBar(orientation));
    }

    void setChangedValueCallback(Callback<long>::Ptr callback) {
        this->changedValueCallback = callback;
    }
    
    void setScrollStepCallback(Callback<ScrollStep::Type>::Ptr callback) {
        this->scrollStepCallback = callback;
    }

    void setValue(long value);
    void setValueRange(long totalValue, long heightValue, long value);

private:
    virtual Measures internalGetDesiredMeasures();
    virtual void processGuiWidgetCreatedEvent();

private: // GuiWidget::EventListener interface implementation
    virtual GuiWidget::ProcessingResult processGuiWidgetEvent(const XEvent* event);
    virtual void                        processGuiWidgetNewPositionEvent(const Position& newPosition);
    virtual void                        processGuiWidgetRedrawEvent(Region redrawRegion);

private:
    enum HilitedPart { TOP_ARROW, SCROLLER, BOTTOM_ARROW, NONE };

    ScrollBar(Orientation::Type orientation);

    void calculateValuesFromPosition();
    long calcScrollHeight();
    long calcScrollY();
    long calcHighestScrollY();
    long calcValue();
    
    void drawUpButton();
    void drawPressedUpButton();
    void drawDownButton();
    void drawPressedDownButton();
    void drawArea();
    void drawArrows();
    void hiliteScrollBarPartAtMousePosition(int mouseX, int mouseY);

    
    Callback<long>::Ptr changedValueCallback;
    Callback<ScrollStep::Type>::Ptr scrollStepCallback;
    
    const bool isV;
    
    int scrollHeight;
    int scrollY;
    
    int scrollAreaLength;
    int scrollAreaWidth;
    
    long totalValue;
    long heightValue;
    long originalTotalValue;
    long value;
    bool movingBar;
    int  movingYOffset;
    int  arrowLength;
    
    bool isButtonPressedForScrollStep;
    ScrollStep::Type scrollStep;
    void handleScrollStepRepeating();
    HilitedPart hilitedPart;
    GC scrollBar_gcid;
};


} // namespace LucED


#endif // SCROLL_BAR_HPP
