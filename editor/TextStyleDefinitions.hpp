/////////////////////////////////////////////////////////////////////////////////////
//
//   LucED - The Lucid Editor
//
//   Copyright (C) 2005-2010 Oliver Schmidt, oliver at luced dot de
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

#ifndef TEXT_STYLE_DEFINITIONS_HPP
#define TEXT_STYLE_DEFINITIONS_HPP

#include "ObjectArray.hpp"
#include "HeapObject.hpp"
#include "OwningPtr.hpp"
#include "TextStyleDefinition.hpp"
#include "Nullable.hpp"
#include "Null.hpp"
#include "ConfigData.hpp"

namespace LucED
{

class TextStyleDefinitions : public HeapObject
{
public:
    typedef OwningPtr<      TextStyleDefinitions> Ptr;
    typedef OwningPtr<const TextStyleDefinitions> ConstPtr;
    
    typedef ConfigData::Fonts     ::Element::Font      ConfigDataFont;
    typedef ConfigData::TextStyles::Element::TextStyle ConfigDataTextStyle;
    
    typedef HeapObjectArray<ConfigDataFont::Ptr>       FontList;
    typedef HeapObjectArray<ConfigDataTextStyle::Ptr>  TestStyleList;
    
    static Ptr create(FontList::Ptr        fonts, 
                      TestStyleList::Ptr   textStyles)
    {
        return Ptr(new TextStyleDefinitions(fonts, textStyles));
    }
    
    static Ptr create()
    {
        return Ptr(new TextStyleDefinitions(Null, Null));
    }
    
    void append(const TextStyleDefinition& newDefinition) {
        definitions.append(newDefinition);
    }
    
    bool isEmpty() const {
        return definitions.getLength() == 0;
    }
    int getLength() const {
        return definitions.getLength();
    }
    const TextStyleDefinition& get(int i) const {
        return definitions[i];
    }
    Nullable<TextStyleDefinition> getFirstWithName(const String& name) const {
        for (int i = 0, n = getLength(); i < n; ++i) {
            if (definitions[i].getName() == name) {
                return definitions[i];
            }
        }
        return Null;
    }
     
    
private:
    TextStyleDefinitions(FontList::Ptr       fonts, 
                         TestStyleList::Ptr  textStyles);
                         
    ObjectArray<TextStyleDefinition> definitions;
};

} // namespace LucED
                    
#endif // TEXT_STYLE_DEFINITIONS_HPP
