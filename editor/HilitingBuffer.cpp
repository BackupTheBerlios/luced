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

#include "util.h"
#include "HilitingBuffer.h"

using namespace LucED;

static long calculateMaxDistance(HilitedText::Ptr hiliting)
{
    if (hiliting->getSyntaxPatterns().isValid()) {
        return 3 * util::maximum(
            hiliting->getLanguageMode()->getHilitingBreakPointDistance(),
            hiliting->getSyntaxPatterns()->getTotalMaxExtend());
    } else {
        return 3 * hiliting->getLanguageMode()->getHilitingBreakPointDistance();
    }
}

HilitingBuffer::HilitingBuffer(HilitedText::Ptr hiliting)
  : startPos(0),
    hiliting(hiliting),
    syntaxPatterns(hiliting->getSyntaxPatterns()),
    languageMode(hiliting->getLanguageMode()),
    iterator(hiliting->createNewIterator()),
    slotForHilitingUpdateTreatment(this, &HilitingBuffer::treatHilitingUpdate),
    slotForTextDataUpdateTreatment(this, &HilitingBuffer::treatTextDataUpdate),
    maxDistance(calculateMaxDistance(hiliting))
    
{
    textData = hiliting->getTextData();
    if (syntaxPatterns.isValid()) {
        ovector.increaseTo(syntaxPatterns->getMaxOvecSize());
    }
    hiliting->registerUpdateListener(slotForHilitingUpdateTreatment);
    textData->registerUpdateListener(slotForTextDataUpdateTreatment);
}


void HilitingBuffer::treatHilitingUpdate(HilitedText::UpdateInfo update)
{
    ASSERT(update.beginPos < update.endPos);
    
    if (update.beginPos < this->startPos + styleBuffer.getLength() && update.endPos > this->startPos)
    {
        styleBuffer.clear();
        patternStack.clear();
    }
    updateListeners.invokeAllCallbacks(UpdateInfo(update.beginPos, update.endPos));
}


void HilitingBuffer::treatTextDataUpdate(TextData::UpdateInfo update)
{
    if (update.beginChangedPos  < this->startPos + styleBuffer.getLength()
     && update.oldEndChangedPos > this->startPos)
    {
        styleBuffer.clear();
        patternStack.clear();
    }
    else if (update.oldEndChangedPos <= this->startPos)
    {
        this->startPos                   += update.changedAmount;
        this->rememberedSearchRestartPos += update.changedAmount;
    }
}


static inline void fillSubs(ObjectArray<CombinedSubPatternStyle>& sps, ByteArray& array, int pos, MemArray<int>& ovector)
{
    for (int i = 0; i < sps.getLength(); ++i) {
        int subNo = sps[i].substrNo * 2;
        if (ovector[subNo] != -1) {
            array.fillAmountWith(pos + ovector[subNo], ovector[subNo + 1] - ovector[subNo], sps[i].style);
        }
    }
}

int HilitingBuffer::getNonBufferedTextStyle(long pos)
{
    SyntaxPattern* sp = NULL;
    long searchStartPos = 0;
    
    if (!syntaxPatterns.isValid()) {
        return 0;
    }

    if (patternStack.getLength() > 0 && styleBuffer.getLength() > 0) {
        // try to reuse patternStack
        searchStartPos = this->rememberedSearchRestartPos; //this->startPos + styleBuffer.getLength();
        ASSERT(this->startPos <= searchStartPos);
        ASSERT(searchStartPos < this->startPos + styleBuffer.getLength());
        if (pos >= searchStartPos && pos - searchStartPos < maxDistance) {
            sp = syntaxPatterns->get(patternStack.getLast());
        }
    }
    if (sp == NULL)
    {

        patternStack.clear();
    
        // find new BreakPoint that must end really before pos, because new chars
        // at pos could have lead to another break at this position, 
        // therefore take (pos - 1)
    
        hiliting->moveIteratorToNextBefore(iterator, (pos > 0) ? (pos - 1) : pos);    
        
        searchStartPos = hiliting->getBreakEndPos(iterator);
        ASSERT(0 <= searchStartPos && searchStartPos <= pos);

        // TODO: vielleicht besser PROCESS_AMOUNT, da das gr��te vorkommende Pattern
        //       gr��er als 2 * getHilitingBreakPointDistance() sein k�nnte.
        if (pos - searchStartPos >= maxDistance) {
            // HilitedText would be too expensive -> return default style or approximate
            if (languageMode->hasApproximateUnknownHilitingFlag()) {
                sp = syntaxPatterns->get(0);
                patternStack.clear().append(0);
                searchStartPos = pos - languageMode->getApproximateUnknownHilitingReparseRange();
                if (searchStartPos < 0) {
                    searchStartPos = 0;
                }
            } else {
                return 0;
            }
        }
        else
        {
            hiliting->copyBreakStackTo(iterator, patternStack);
            sp = syntaxPatterns->get(patternStack.getLast());
        }
    }
    if (searchStartPos > this->startPos && searchStartPos - this->startPos 
            < util::minimum(
                    2 * maxDistance + 2 * languageMode->getApproximateUnknownHilitingReparseRange(),
                    styleBuffer.getLength()))
    {
        styleBuffer.removeTail(searchStartPos - this->startPos);
    }
    else
    {
        styleBuffer.clear();
        this->startPos = searchStartPos;
    }
    
    // searchEndPos

    long searchEndPos = pos + maxDistance;
    util::minimize(&searchEndPos, textData->getLength());
    
    while (true) 
    {
        Regex::MatchOptions additionalOptions;
        
        long extendedSearchEndPos = searchEndPos + sp->maxREBytesExtend;
        
        util::minimize(&extendedSearchEndPos, textData->getLength());

        if (!textData->isBeginOfLine(searchStartPos)) {
            additionalOptions |= Regex::NOTBOL;
        }
        if (!textData->isEndOfLine(extendedSearchEndPos)) {
            additionalOptions |= Regex::NOTEOL;
        }

        // do searching
        
        this->rememberedSearchRestartPos = searchStartPos;
        
        bool matched = sp->re.findMatch((const char*) textData->getAmount(searchStartPos, extendedSearchEndPos - searchStartPos), 
                extendedSearchEndPos - searchStartPos, 0,
                /*PCRE_NOTEMPTY |*/ additionalOptions, ovector);

        if (matched) {
            // something matched
            int cid;

            cid = sp->getMatchedChild(ovector);
            if (cid == -1) {
                
                // EndPattern matched

                long styleBufferPos = styleBuffer.getLength();
                styleBuffer.appendAndFillAmountWith(ovector[1], sp->style);
                fillSubs(sp->combinedSubs, styleBuffer, styleBufferPos, ovector);

                if (pos < searchStartPos + ovector[1]) {
                    return styleBuffer[pos - this->startPos];
                }
                patternStack.removeLast();
                sp = syntaxPatterns->get(patternStack.getLast());
                searchStartPos += ovector[1];

            } else {

                // normal Child matched
                
                SyntaxPattern *childPat = syntaxPatterns->getChildPattern(sp, cid);

                if (patternStack.getLength() >= STACK_SIZE 
                        && childPat->hasEndPattern) {

                    // New Begin-Child, but Stack is too big
                    
                    long styleBufferPos = styleBuffer.getLength();
                    styleBuffer.appendAndFillAmountWith(ovector[1], sp->style);

                    if (pos < searchStartPos + ovector[1]) {
                        return styleBuffer[pos - this->startPos];
                    }
                    searchStartPos += ovector[1];
                    
                } else {
                    // Stack is ok or doesn't need to grow
                    
                    long styleBufferPos = styleBuffer.getLength();
                    styleBuffer.appendAmount(ovector[1]);
                    styleBuffer.fillAmountWith(styleBufferPos, ovector[0], sp->style);
                    styleBuffer.fillAmountWith(styleBufferPos + ovector[0], ovector[1] - ovector[0], childPat->style);
                    fillSubs(sp->combinedSubs, styleBuffer, styleBufferPos, ovector);
                    
                    if (pos < searchStartPos + ovector[1]) {
                        return styleBuffer[pos - this->startPos];
                    }
                    if (childPat->hasEndPattern) {
                        patternStack.append(syntaxPatterns->getChildPatternId(sp, cid));
                        sp = childPat;
                    }
                    searchStartPos += ovector[1];
                }

            }

        } else {
            // nothing matched
            styleBuffer.appendAndFillAmountWith(searchEndPos - searchStartPos, sp->style);
            return styleBuffer[pos - this->startPos];
        }

    }
}

void HilitingBuffer::registerUpdateListener(const Callback1<UpdateInfo>& updateCallback)
{
    updateListeners.registerCallback(updateCallback);
}

