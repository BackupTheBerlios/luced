-------------------------------------------------------------------------------------
--
--   LucED - The Lucid Editor
--
--   Copyright (C) 2005-2006 Oliver Schmidt, oliver at luced dot de
--
--   This program is free software; you can redistribute it and/or modify it
--   under the terms of the GNU General Public License Version 2 as published
--   by the Free Software Foundation in June 1991.
--
--   This program is distributed in the hope that it will be useful, but WITHOUT
--   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
--   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
--   more details.
--
--   You should have received a copy of the GNU General Public License along with 
--   this program; if not, write to the Free Software Foundation, Inc., 
--   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
--
-------------------------------------------------------------------------------------

local topChildPatterns = {
                            "emluaexpr", "emlualine",
                            "string1",   "string2",  "string3", 
                            "comment1",  "comment2", "keyword"
                         }
return
{
	root = {
        	style = "default",
                childPatterns = {"shebang", "noshebang"},
        },
        
        shebang = {
                style = "default",
                beginPattern     = [[ ^(?P<shebangLine>\#\!.*?)$ ]],
                endPattern       = [[ (?!.|\n) ]],
                beginSubstyles   = {shebangLine = "comment"},
                maxBeginExtend   = 150,
                maxEndExtend     = 150,
                childPatterns = topChildPatterns,
        },
        noshebang = {
                style = "default",
                beginPattern     = [[ (?=.) ]],
                endPattern       = [[ (?!.|\n) ]],
                maxBeginExtend   = 1,
                maxEndExtend     = 1,
                childPatterns = topChildPatterns,
        },
        comment1 = {
        	style = "comment",
                beginPattern     = [=[--\[(?P<comment1EqualSigns>(?>\=*))\[]=],
                endPattern       = [=[\]((?>\=*))(*comment1EqualSigns)\]]=],
                maxBeginExtend   = 300,
                maxEndExtend     = 300,
                pushSubpattern   = "comment1EqualSigns",
                childPatterns    = { "emluaexpr", "emlualine", },
        },
        
        comment2 = {
        	style = "comment",
                beginPattern     = [[--]],
                endPattern       = [[\n]],
                maxBeginExtend   = 2,
                maxEndExtend     = 1,
                childPatterns    = { "emluaexpr", "emlualine" },
        },
        
        keyword = {
        	style = "keyword",
                pattern     = [[
                                \+|\-|\*|\/|\%|\^|\#|\=\=|\~\=|\<\=|\>\=|\<|\>|\=|\(|\)|\{|\}|
                                \[|\]|\;|\:|\,|\.|\.\.|\.\.\.

                                |\b(?: local   |true  |false  |and    |false  |for    |
                                       break   |do    |else   |elseif |end    |or     |
                                       function|if    |in     |local  |nil    |not    |
                                       return  |then  |true   |until  |while  |repeat
                                 )\b
                              ]],
                maxExtend   = 10,
        },

        string1 = {
        	style = "string",
                beginPattern     = [[(?P<stringBegin1>")]],
                endPattern       = [[(?P<stringEnd1>")|\n]],
                maxBeginExtend   = 1,
                maxEndExtend     = 1,
                childPatterns    = {"emluaexpr", "stringescape"},
                beginSubstyles   = {stringBegin1 = "keyword"},
                endSubstyles     = {stringEnd1   = "keyword"},
        },

        string2 = {
        	style = "string",
                beginPattern     = [[(?P<stringBegin2>\')]],
                endPattern       = [[(?P<stringEnd2>\')|\n]],
                maxBeginExtend   = 1,
                maxEndExtend     = 1,
                childPatterns    = {"emluaexpr", "stringescape"},
                beginSubstyles   = {stringBegin2 = "keyword"},
                endSubstyles     = {stringEnd2   = "keyword"},
        },

        string3 = {
            style = "string",
            beginPattern     = [[(?P<string3Begin>\[(?P<string3EqualSigns>(?>\=*))\[)]],
            endPattern       = [[(?P<string3End>\]((?>\=*))(*string3EqualSigns)\])]],
            maxBeginExtend   = 300,
            maxEndExtend     = 300,
            childPatterns    = {"emluaexpr", "emlualine"},
            pushSubpattern   = "string3EqualSigns",
            beginSubstyles   = { string3Begin = "keyword" },
            endSubstyles     = { string3End   = "keyword" },
        },

        stringescape = {
        	style = "boldstring",
                pattern          = [[\\\d{1,3}|\\.|\\\n]],
                maxExtend        = 2,
        },
        
        emluaexpr = {
                style = "regex",
                beginPattern     = [[@\(]],
                endPattern       = [[\)|$]],
                maxBeginExtend   = 2,
                maxEndExtend     = 1,
                childPatterns    = { "emluaexpr2" },
        }, 
        emlualine = {
                style = "regex",
                beginPattern     = [[^@]],
                endPattern       = [[\n]],
                maxBeginExtend   = 1,
                maxEndExtend     = 1,
        }, 
        emluaexpr2 = {
                style = "regex",
                beginPattern     = [[\(]],
                endPattern       = [[\)|$]],
                maxBeginExtend   = 1,
                maxEndExtend     = 1,
                childPatterns    = { "emluaexpr2" },
        }, 
}


