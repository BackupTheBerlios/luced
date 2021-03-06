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

#include "debug.hpp"
#include "BasicRegex.hpp"
#include "RegexException.hpp"

using namespace LucED;

const unsigned char* BasicRegex::pcreCharTable = NULL;

int BasicRegex::pcreCalloutCallback(pcre_callout_block* calloutBlock)
{
    CalloutData* d = static_cast<CalloutData*>(calloutBlock->callout_data);
    return d->calloutFunction(d->object, calloutBlock);
}


void BasicRegex::initialize(const char* expr, CreateOptions createOptions)
{
    pcre_callout = pcreCalloutCallback;

    if (pcreCharTable == NULL) {
        pcreCharTable = pcre_maketables();
    }
    const char* errortext;
    int errorpos;

    re = pcre_compile(expr, 
                      createOptions.getOptions()|PCRE_UTF8|PCRE_NO_UTF8_CHECK, 
                      &errortext,
                      &errorpos, 
                      BasicRegex::pcreCharTable);
    if (re == NULL) {
        throw RegexException(errortext, errorpos);
    }
    pcre_refcount(re, +1);
}

    
    
BasicRegex::BasicRegex(const String& expr, CreateOptions createOptions)
{
    initialize(expr.toCString(), createOptions);
}


BasicRegex::BasicRegex(const ByteArray& expr, CreateOptions createOptions)
{
    initialize(expr.toCString(), createOptions);
}


BasicRegex::BasicRegex(const BasicRegex& src)
{
    re = src.re;
    if (re != NULL) {
        pcre_refcount(re, +1);
    }
}


BasicRegex& BasicRegex::operator=(const BasicRegex& src)
{
    pcre* oldRe = re;
    re = src.re;
    if (re != NULL) {
        pcre_refcount(re, +1);
    }
    if (oldRe != NULL) {
        int refCount = pcre_refcount(oldRe, -1);
        if (refCount == 0) {
            pcre_free(oldRe);
        }
    }
    return *this;
}


BasicRegex::~BasicRegex()
{
    if (re != NULL) {
        int refCount = pcre_refcount(re, -1);
        if (refCount == 0) {
            pcre_free(re);
            re = NULL;
        }
    }
}


int BasicRegex::getCaptureNumberByName(const String& substringName) const
{
    ASSERT(re != NULL);
    int rslt = pcre_get_stringnumber(re, substringName.toCString());
    if (rslt == PCRE_ERROR_NOSUBSTRING) {
        throw RegexException(String() << "named substring '" << substringName << "' not found");
    }
    return rslt;
}

int BasicRegex::getCaptureNumberByName(const ByteArray& substringName) const
{
    ASSERT(re != NULL);
    int rslt = pcre_get_stringnumber(re, substringName.toCString());
    if (rslt == PCRE_ERROR_NOSUBSTRING) {
        throw RegexException(String() << "named substring '" << substringName.toString() << "' not found");
    }
    return rslt;
}

