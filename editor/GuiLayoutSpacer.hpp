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

#ifndef GUILAYOUTSPACER_H
#define GUILAYOUTSPACER_H

#include <limits.h>

#include "GuiElement.hpp"
#include "ObjectArray.hpp"
#include "debug.hpp"
#include "WeakPtr.hpp"

namespace LucED {

class GuiLayoutSpacer : public GuiElement
{
public:
    typedef OwningPtr<GuiLayoutSpacer> Ptr;

    static Ptr create(int minWidth, int minHeight, int bestWidth, int bestHeight, int maxWidth, int maxHeight) {
        return Ptr(new GuiLayoutSpacer(Measures(minWidth, minHeight, bestWidth, bestHeight, maxWidth, maxHeight)));
    }

    static Ptr create() {
        return create(0, 0, 0, 0, 0, 0);
    }

protected:
    GuiLayoutSpacer(const Measures& m) : measures(m)
    {}

    virtual Measures internalGetDesiredMeasures();
    
    Measures measures;
};

class GuiLayoutSpacerH : public GuiLayoutSpacer
{
public:
    typedef OwningPtr<GuiLayoutSpacerH> Ptr;

    static Ptr create(int minWidth, int bestWidth, int maxWidth) {
        return Ptr(new GuiLayoutSpacerH(Measures(minWidth, 0, bestWidth, 0, maxWidth, 0)));
    }
    static Ptr create(int minWidth, int bestWidth) {
        return create(minWidth, bestWidth, bestWidth);
    }
    static Ptr create(int width) {
        return create(width, width, width);
    }
    static Ptr create() {
        return create(0, 0, INT_MAX);
    }
private:
    GuiLayoutSpacerH(const Measures& m) : GuiLayoutSpacer(m)
    {}
};

class GuiLayoutSpacerV : public GuiLayoutSpacer
{
public:
    typedef WeakPtr<GuiLayoutSpacerV> Ptr;

    static Ptr create(int minHeight, int bestHeight, int maxHeight) {
        return Ptr(new GuiLayoutSpacerV(Measures(0, minHeight, 0, bestHeight, 0, maxHeight)));
    }
    static Ptr create(int minHeight, int bestHeight) {
        return create(minHeight, bestHeight, bestHeight);
    }
    static Ptr create(int height) {
        return create(height, height, height);
    }
    static Ptr create() {
        return create(0, 0, INT_MAX);
    }
private:
    GuiLayoutSpacerV(const Measures& m) : GuiLayoutSpacer(m)
    {}
};

class GuiLayoutSpacerFrame : public GuiElement
{
public:
    typedef OwningPtr<GuiLayoutSpacerFrame> Ptr;

    static Ptr create(GuiElement::Ptr element, int thickness) {
        return Ptr(new GuiLayoutSpacerFrame(element, thickness));
    }
    
    virtual void setPosition(const Position& p);

protected:
    GuiLayoutSpacerFrame(GuiElement::Ptr member, int thickness);

    virtual Measures internalGetDesiredMeasures();
};

} // namespace LucED

#endif // GUILAYOUTSPACER_H
