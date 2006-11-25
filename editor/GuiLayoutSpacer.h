#ifndef GUILAYOUTSPACER_H
#define GUILAYOUTSPACER_H

#include "GuiElement.h"
#include "ObjectArray.h"
#include "debug.h"
#include "OwningPtr.h"

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

    virtual Measures getDesiredMeasures() {
        return measures;
    }
    virtual void setPosition(Position p) {
    }

protected:
    GuiLayoutSpacer(const Measures& m) : measures(m)
    {}
    
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
    typedef OwningPtr<GuiLayoutSpacerV> Ptr;

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
    
    virtual Measures getDesiredMeasures();

    virtual void setPosition(Position p);

protected:
    GuiLayoutSpacerFrame(GuiElement::Ptr member, int thickness);

    GuiElement::Ptr root;
};

} // namespace LucED

#endif // GUILAYOUTSPACER_H
