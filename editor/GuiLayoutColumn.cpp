#include "util.h"
#include "GuiLayoutColumn.h"
#include "GuiLayoutSpacer.h"
#include "OwningPtr.h"
#include "GuiLayouter.h"

using namespace LucED;

void GuiLayoutColumn::addElement(GuiElement::Ptr element)
{
    elements.append(element);
}

void GuiLayoutColumn::addSpacer(int height)
{
    elements.append(GuiLayoutSpacer::create(0, 0, 0, height, 0, -1));
}

void GuiLayoutColumn::addSpacer()
{
    elements.append(GuiLayoutSpacer::create(0, 0, 0, 0, 0, -1));
}

static void maximize(int *a, int b)
{
    if (*a != -1) {
        if (b == -1) {
            *a = -1;
        } else {
            util::maximize(a, b);
        }
    }
}

static void addimize(int *a, int b)
{
    if (*a != -1) {
        if (b == -1) {
            *a = -1;
        } else {
            *a += b;
        }
    }
}


GuiElement::Measures GuiLayoutColumn::getDesiredMeasures()
{
    int bestWidth = 0;
    int bestHeight = 0;
    int minWidth = 0;
    int minHeight = 0;
    int maxWidth = 0;
    int maxHeight = 0;
    int incrWidth = 1;
    int incrHeight = 1;
    
    for (int i = 0; i < elements.getLength(); ++i)
    {
        Measures m = elements[i]->getDesiredMeasures();

        addimize(&minHeight,  m.minHeight);
        addimize(&bestHeight, m.bestHeight);
        addimize(&maxHeight,  m.maxHeight);
        
        maximize(&minWidth,  m.minWidth);
        maximize(&bestWidth, m.bestWidth);
        maximize(&maxWidth,  m.maxWidth);
        
        maximize(&incrWidth,  m.incrWidth);
        maximize(&incrHeight, m.incrHeight);
    }
    minWidth  = bestWidth  - ((bestWidth  - minWidth) / incrWidth)  * incrWidth;
    minHeight = bestHeight - ((bestHeight - minHeight)/ incrHeight) * incrHeight;
    return Measures(minWidth, minHeight, bestWidth, bestHeight, maxWidth, maxHeight,
                    incrWidth, incrHeight);
}

void GuiLayoutColumn::setPosition(Position p)
{
    rowMeasures.clear();
    
    for (int i = 0; i < elements.getLength(); ++i)
    {
        rowMeasures.append(elements[i]->getDesiredMeasures());
    }
    
    GuiLayouter<VerticalAdapter>::adjust(rowMeasures, p.h);
    
    int y = p.y;
    for (int i = 0; i < elements.getLength(); ++i)
    {
        Measures& m = rowMeasures[i];
        elements[i]->setPosition(Position(p.x, y, p.w, m.bestHeight));
        y += m.bestHeight;
    }
}
