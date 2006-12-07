/////////////////////////////////////////////////////////////////////////////////////
//
//   LucED - The Lucid Editor
//
//   Copyright (C) 2005-2006 Oliver Schmidt, oliver at luced dot de
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

#ifndef LINEINFO_H
#define LINEINFO_H

#include "NonCopyable.h"
#include "ByteArray.h"

namespace LucED {

class LineInfo
{
public:
    LineInfo() : valid(false) {}

private:
    friend class TextWidget;
    friend class LineInfos;

    bool isDifferentOnScreenThan(const LineInfo& rhs) const {
        if (leftPixOffset  != rhs.leftPixOffset
                || isEndOfText != rhs.isEndOfText || backgroundToEnd != rhs.backgroundToEnd) {
            return true;
        }
        return !(outBuf == rhs.outBuf);
    }
    
    bool   valid;
    long   beginOfLinePos;
    long   endOfLinePos;
    long   startPos;
    long   endPos;
    bool   isEndOfText;
    long   totalPixWidth;
    int    pixWidth;
    int    leftPixOffset;
    int    rightPixOffset;
    int    backgroundToEnd;
    ByteArray outBuf;
    ByteArray styles;
};

class LineInfos
{
public:
    LineInfos() : first(0) {}
    
    int getLength() { return lineInfos.getLength(); }
    const LineInfo* getPtr(long nr) const {
        return &lineInfos[(first + nr) % lineInfos.getLength()];
    }
    LineInfo* getPtr(long nr) {
        return &lineInfos[(first + nr) % lineInfos.getLength()];
    }
    void setAllInvalid() {
        for (long i = 0; i < lineInfos.getLength(); ++i) {
            lineInfos[i].valid = false;
        }
    }
    void setLength(long newLength) {
        long oldLength = lineInfos.getLength();
        if (newLength > oldLength) {
            lineInfos.appendAmount(newLength - oldLength);
        } else {
            long removeAmount = oldLength - newLength;
            lineInfos.removeAmount(oldLength - removeAmount, removeAmount);
        }
    }
    void moveFirst(long offset) {
        long new_first = (first + offset) % lineInfos.getLength();
        if (new_first < 0)
            new_first = lineInfos.getLength() + new_first;
        if (offset > 0  && offset < lineInfos.getLength()) {
            for (long i = 0; i < offset; ++i) {
                getPtr(i)->valid = false;
            }
            first = new_first;
        } else if (offset < 0 && -offset < lineInfos.getLength()) {
            long length = lineInfos.getLength();
            for (long i = 0; i < -offset; ++i) {
                getPtr(length - i - 1)->valid = false;
            }
            first = new_first;
        } else if (offset != 0) {
            setAllInvalid();
            first = new_first;
        }
    }
private:
    ObjectArray<LineInfo> lineInfos;
    long first;
};

} // namespace LucED

#endif // LINEINFO_H
