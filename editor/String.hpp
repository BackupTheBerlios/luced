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

#ifndef STRING_HPP
#define STRING_HPP

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <sstream>

#include "debug.hpp"
#include "types.hpp"

namespace LucED
{

class String
{
public:
    String()
    {}

    String(const char* rhs)
#ifdef DEBUG
    {
        ASSERT(rhs != NULL);
        s = std::string(rhs);
    }
#else
        : s(rhs)
    {}
#endif
    

    String(const char* rhs, int length)
#ifdef DEBUG
    {
        ASSERT(length == 0 || rhs != NULL);
        s = std::string(rhs, length);
    }
#else
        : s(rhs, length)
    {}
#endif

    String(const std::string& rhs)
        : s(rhs)
    {}

    explicit String(int i) 
        : s(stringify(i))
    {}

    int getLength() const {
        return s.length();
    }
    const char* toCString() const {
        return s.c_str();
    }
    const std::string& toStdString() const {
        return s;
    }
    std::string toStdString() {
        return s;
    }
    char& operator[](int i) {
        ASSERT(0 <= i && i < getLength());
        return s[i];
    }
    const char& operator[](int i) const {
        ASSERT(0 <= i && i < getLength());
        return s[i];
    }
    String getSubstring(int pos, int length) const {
        if (length == 0) {
            return String();
        }
        ASSERT(0 <= length);
        ASSERT(0 <= pos && pos <= getLength());
        if (pos + length > getLength()) {
            length = getLength() - pos;
        }
        return s.substr(pos, length);
    }
    String getSubstringBetween(int pos1, int pos2) const {
        return getSubstring(pos1, pos2 - pos1);
    }
    String& append(const String& rhs) {
        s.append(rhs.s);
        return *this;
    }
    String& append(const std::string& rhs) {
        s.append(rhs);
        return *this;
    }
    String& append(const char* rhs) {
        ASSERT(rhs != NULL);
        s.append(rhs);
        return *this;
    }
    String& append(const char* rhs, int length) {
        ASSERT(rhs != NULL);
        s.append(rhs, length);
        return *this;
    }
    String& appendSubstring(const String& rhs, int pos, int length) {
        ASSERT(pos + length <= rhs.getLength());
        s.append(rhs.toCString() + pos, length);
        return *this;
    }
    String& append(const byte* rhs, int length) {
        ASSERT(rhs != NULL);
        s.append((const char*)rhs, length);
        return *this;
    }
    String& append(char c) {
        s.push_back(c);
        return *this;
    }
    String& appendLowerChar(char c) {
        s.push_back((char)tolower(c));
        return *this;
    }
    String& append(int rhs) {
        return append(String(rhs));
    }

    bool consistsOfDigits() const {
        for (int i = 0, n = s.length(); i < n; ++i) {
            if (!isdigit(s[i])) {
                return false;
            }
        }
        return true;
    }
    
    int toInt() const {
        return atoi(s.c_str());
    }
    
    bool equals(const char* rhs, long len) const
    {
        ASSERT(len >= 0);
             
        if (len != getLength()) {
            return false;
        } else {
            return memcmp(toCString(), rhs, len) == 0;
        }
    }
    
    bool equalsSubstringAt(long pos, const char* rhs) const
    {
        long len = strlen(rhs);
        
        ASSERT(pos + len >= 0);
             
        if (pos + len > getLength()) {
            return false;
        } else {
            return memcmp(toCString() + pos, rhs, len) == 0;
        }
    }
    
    bool equalsIgnoreCase(const String& rhs) const {
        int j = getLength(), k = rhs.getLength();
        if (j != k) {
            return false;
        }
        for (int i = 0; i < j; ++i) {
            if (tolower(s[i]) != tolower(rhs[i])) {
                return false;
            }
        }
        return true;
    }
    
    bool isInt() const {
        int i = 0;
        int n = getLength();
        if (n == 0) {
            return false;
        }
        if (s[i] == '-' && i + 1 < n) {
            ++i;
        }
        for (; i < n; ++i) {
            if (!isdigit(s[i])) {
                return false;
            }
        }
        return true;
    }
    
    bool isHex() const {
        for (int i = 0, n = getLength(); i < n; ++i) {
            if (!isxdigit(s[i])) {
                return false;
            }
        }
        return true;
    }
    
    bool contains(char c) const {
        for (int i = 0, n = s.length(); i < n; ++i) {
            if (s[i] == c) {
                return true;
            }
        }
        return false;
    }
    
    bool endsWith(const char* str) const {
        long len = strlen(str);
        if (getLength() < len) {
            return false;
        } else {
            return memcmp(toCString() + getLength() - len, str, len) == 0;
        }
    }
    
    bool startsWith(const char* str) const {
        long len = strlen(str);
        if (getLength() < len) {
            return false;
        } else {
            return memcmp(toCString(), str, len) == 0;
        }
    }
    
    template<class T> String& operator<<(const T& rhs) {
        return append(rhs);
    }
    
    bool operator==(const String& rhs) const {
        return s == rhs.s;
    }
    bool operator!=(const String& rhs) const {
        return s != rhs.s;
    }
    void removeAmount(int pos, int amount) {
        ASSERT(0 <= amount);
        ASSERT(0 <= pos && pos + amount < getLength());
        s.erase(pos, amount);
    }
    void removeBetween(int pos1, int pos2) {
        removeAmount(pos1, pos2 - pos1);
    }
    void removeTail(int pos) {
        removeAmount(pos, getLength() - pos);
    }
    String getTail(int pos) const {
        ASSERT(0 <= pos && pos <= getLength());
        return s.substr(pos);
    }
    int findFirstOf(char c, int startPos = 0) const {
        ASSERT(0 <= startPos && startPos <= getLength());
        int rslt = s.find_first_of(c, startPos);
        if (rslt == std::string::npos) {
            return -1;
        } else {
            return rslt;
        }
    }
    
    String getTrimmedSubstring() const {
        int len = getLength();
        int i = 0, j = len;
        while (i < len && ((*this)[i] == ' ' || (*this)[i] == '\t')) {
            ++i;
        }
        while (j > i && ((*this)[j - 1] == ' ' || (*this)[j - 1] == '\t')) {
            --j;
        }
        return getSubstringBetween(i, j);
    }
    
    void trim() {
        *this = getTrimmedSubstring();
    }
    
    void clear() {
        s.clear();
    }

private:
    static std::string stringify(int i) {
        std::ostringstream o;
        o << i;
        return o.str();
    }
    
    std::string s;
};

} // namespace LucED

#endif // STRING_HPP
