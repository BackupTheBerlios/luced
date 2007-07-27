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

#ifndef COMMANDLINE_HPP
#define COMMANDLINE_HPP

#include "HeapObjectArray.hpp"
#include "String.hpp"

namespace LucED
{

class Commandline : public HeapObjectArray<String>
{
public:

    typedef OwningPtr<Commandline> Ptr;
    
    static Ptr create(int argc, char** argv) {
        return Ptr(new Commandline(argc, argv));
    }
    
private:

    Commandline(int argc, char** argv)
    {
        for (int i = 1; i < argc; ++i) {
            this->append(String(argv[i]));
        }
    }
};

} // namespace LucED

#endif // COMMANDLINE_HPP
