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

#ifndef NULLABLE_HPP
#define NULLABLE_HPP

namespace LucED
{

template
<
    class V
>
class Nullable
{
public:
    Nullable()           : valid(false), v()  {}
    Nullable(const V& v) : valid(true),  v(v) {}

    bool     isValid() const { return valid; }
    bool     isNull()  const { return !valid; }
    operator V()       const { return v; }
    V        get()     const { return v; }

private:
    bool valid;
    V v;
};

} // namespace LucED

#endif // NULLABLE_HPP