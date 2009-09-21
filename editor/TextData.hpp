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

#ifndef TEXT_DATA_HPP
#define TEXT_DATA_HPP

#include "String.hpp"
#include "HeapObject.hpp"
#include "MemBuffer.hpp"
#include "ByteArray.hpp"
#include "ObjectArray.hpp"
#include "CallbackContainer.hpp"
#include "OwningPtr.hpp"
#include "EditingHistory.hpp"
#include "TimeVal.hpp"
#include "File.hpp"
#include "RawPtr.hpp"

namespace LucED
{

class TextData : public HeapObject
{
public:

    typedef OwningPtr<TextData> Ptr;

    struct UpdateInfo
    {
        long beginChangedPos;
        long oldEndChangedPos;
        long changedAmount;
        
        UpdateInfo(long beginChangedPos, long oldEndChangedPos, long changedAmount)
          : beginChangedPos(beginChangedPos), oldEndChangedPos(oldEndChangedPos), changedAmount(changedAmount)
        {}
    };

    class MarkHandle
    {
    protected:
        friend class TextData;
        explicit MarkHandle() : index(0)
        {}
        explicit MarkHandle(long index) : index(index)
        {}
        long index;
    };

    class TextMark : public MarkHandle
    {
    public:
        TextMark() {}
        ~TextMark() {
            if (textData.isValid()) {
                textData->getTextMarkData(index)->inUseCounter -= 1;
            }
        }
        TextMark(const TextMark& src) {
            index = src.index;
            textData = src.textData;
            if (textData.isValid()) {
                textData->getTextMarkData(index)->inUseCounter += 1;
            }
        }
        TextMark& operator=(const TextMark& src) {
            ASSERT(!textData.isValid() || !src.textData.isValid() || textData == src.textData);
            if (src.textData.isValid()) {
                src.textData->getTextMarkData(src.index)->inUseCounter += 1;
            }
            if (textData.isValid()) {
                textData->getTextMarkData(index)->inUseCounter -= 1;
            }
            index = src.index;
            textData = src.textData;
            return *this;
        }
        void moveToLineAndColumn(long line, long column) {
            textData->moveMarkToLineAndColumn(*this, line, column);
        }
        void moveToPos(long pos) {
            textData->moveMarkToPos(*this, pos);
        }
        void moveForwardToPos(long nextPos) {
            textData->moveMarkForwardToPos(*this, nextPos);
        }
        long getPos() {
            return textData->getTextPositionOfMark(*this);
        }
        long getLine() {
            return textData->getLineNumberOfMark(*this);
        }
        long getColumn() {
            return textData->getColumnNumberOfMark(*this);
        }
        void moveToBeginOfLine() {
            textData->moveMarkToBeginOfLine(*this);
        }
        void moveToEndOfLine() {
            textData->moveMarkToEndOfLine(*this);
        }
        void moveToNextLineBegin() {
            textData->moveMarkToNextLineBegin(*this);
        }
        byte getChar() {
            return textData->getChar(*this);
        }
        void inc() {
            textData->incMark(*this);
        }
        bool isEndOfText() {
            return textData->isEndOfText(*this);
        }
        bool isValid() {
            return textData.isValid();
        }
        bool isAtBeginOfLine() {
            return textData->isBeginOfLine(getPos());
        }
        bool isAtEndOfLine() {
            return textData->isEndOfLine(*this);
        }
    private:
        friend class TextData;
        TextMark(RawPtr<TextData> textData, long index) {
            this->index = index;
            this->textData = textData;
            textData->getTextMarkData(index)->inUseCounter += 1;
        }
        
        RawPtr<TextData> textData;
    };
    
    class TextMarkData
    {
    private:
        friend class TextData;
        friend class TextMark;
        int inUseCounter;
        long pos;
        long line;
        long column;
    };
    
    static Ptr create() {
        return Ptr(new TextData());
    }

    void loadFile(const String& filename);
    void reloadFile();
    void setRealFileName(const String& filename);
    void setPseudoFileName(const String& filename);
    void save();

    long getLength() const {
        return buffer.getLength();
    }

    long getNumberOfLines() const {
        return numberLines;
    }

    TextMark createNewMark();
    TextMark createNewMark(MarkHandle src);
    TextMarkData* getTextMarkData(long index) {
        return &(marks[index]);
    }

    byte* getAmount(long pos, long amount) {
        return buffer.getAmount(pos, amount);
    }
    String getSubstring(Pos pos, Len amount) {
        return String((const char*) getAmount(pos, amount), amount);
    }
    String getSubstring(Pos pos1, Pos pos2) {
        long amount = pos2 - pos1;
        return String((const char*) getAmount(pos1, amount), amount);
    }
    String getSubstring(const MarkHandle& beginMark, const MarkHandle& endMark) {
        long amount = getTextPositionOfMark(endMark) - getTextPositionOfMark(beginMark);
        ASSERT(0 <= amount);
        return String((const char*) getAmount(getTextPositionOfMark(beginMark), amount), amount);
    }
    String getHead(int length) {
        return getSubstring(Pos(0), Len(length));
    }
    String getAsString() {
        return getSubstring(Pos(0), Len(getLength()));
    }
    
    void setToString(const String& newContent) {
        clear();
        insertAtMark(createNewMark(), newContent);
    }

    void setToData(const char* buffer, int length) {
        clear();
        insertAtMark(createNewMark(), (const byte*) buffer, length);
    }

    void setToData(const byte* buffer, int length) {
        clear();
        insertAtMark(createNewMark(), buffer, length);
    }

    const byte& operator[](long pos) const {
        return buffer[pos];
    }
    byte getChar(long pos) const {
        return buffer[pos];
    }
    byte getChar(MarkHandle m) const {
        return buffer[marks[m.index].pos];
    }
    
    int getWCharAndIncrementPos(long* pos) const
    {
        ASSERT(isBeginOfWChar(*pos));

        int  rslt = 0;
        byte b    = buffer[*pos];

        if ((b & 0x80) == 0)                     // 0x80 = 1000 0000
        {
            rslt  = b;
            *pos += 1;
        }
        else
        {   
            int count;
                                                 // 0xE0 = 1110 0000
            if      ((b & 0xE0) == 0xC0) {       // 0xC0 = 1100 0000
                rslt  = (b & 0x1F);              // 0x1F = 0001 1111
                count = 1;
            }
                                                 // 0xF0 = 1111 0000
            else if ((b & 0xF0) == 0xE0) {       // 0xE0 = 1110 0000
                rslt  = (b & 0x0F);              // 0x0F = 0000 1111
                count = 2;
            }
                                                 // 0xF8 = 1111 1000
            else if ((b & 0xF8) == 0xF0) {       // 0xF0 = 1111 0000
                rslt = (b & 0x07);               // 0x07 = 0000 0111
                count = 3;
            }                
            else {
                count = -1;
            }
            while (true)
            {
                *pos += 1;
                
                if (isEndOfText(*pos)) {
                    break;
                }
                b = buffer[*pos];
                                                     // 0xC0 = 1100 0000
                if ((b & 0xC0) == 0x80) {            // 0x80 = 1000 0000
                    rslt = (rslt << 6) + (b & 0x3F); // 0x3F = 0011 1111
                    count -= 1;
                }
                else {
                    break;
                }
            }
            if (count != 0) {
                rslt = -1;
            }
        }
        return rslt;
    }
    int getWChar(long pos) const {
        return getWCharAndIncrementPos(&pos);
    }
    
    bool isBeginOfWChar(long pos) const {
        return pos == 0 || (buffer[pos - 1] & 0x80) == 0      // 0x80 = 1000 0000
                        || (buffer[pos    ] & 0xC0) != 0x80;  // 0xC0 = 1100 0000
    }
    long getPrevWCharPos(long pos) const {
        ASSERT(pos > 0);
        ASSERT(isBeginOfWChar(pos));
        do { pos -= 1; } while (!isBeginOfWChar(pos));
    }
    long getNextWCharPos(long pos) const {
        ASSERT(!isEndOfText(pos));
        ASSERT(isBeginOfWChar(pos));
        do { pos += 1; } while (!isEndOfText(pos) && !isBeginOfWChar(pos));
    }
    
    bool isEndOfText(long pos) const {
        return pos == buffer.getLength();
    }
    bool isEndOfLine(long pos) const {
        return isEndOfText(pos) || buffer[pos] == '\n';
    }
    bool isBeginOfLine(long pos) const {
        return pos == 0 || buffer[pos - 1] == '\n'; 
    }
    long getLengthOfLineEnding(long pos) const {
        ASSERT(isEndOfLine(pos));
        return isEndOfText(pos) ? 0 : 1;
    }
    long getLengthOfPrevLineEnding(long pos) const {
        ASSERT(isBeginOfLine(pos));
        return pos == 0 ? 0 : 1;
    }
    long getLengthToEndOfLine(long pos) const {
        long epos = pos;
        for (; epos < buffer.getLength() && buffer[epos] != '\n'; ++epos);
        return epos - pos;
    }
    long getNextLineBegin(long pos) const {
        pos += getLengthToEndOfLine(pos);
        pos += getLengthOfLineEnding(pos);
        return pos;
    }
    long getThisLineBegin(long pos) const {
        ASSERT(pos <= getLength());
        while (pos > 0) {
            if (buffer[pos - 1] == '\n')
                return pos;
            --pos;
        }
        return pos;
    }
    long getThisLineEnding(long pos) const {
        while (!isEndOfLine(pos)) {
            pos += 1;
        }
        return pos;
    }
    long getPrevLineBegin(long pos) const {
        pos = getThisLineBegin(pos);
        pos -= getLengthOfPrevLineEnding(pos);
        return getThisLineBegin(pos);
    }
    
    int getViewCounter() const {
        return viewCounter;
    }

private:
    long internalInsertAtMark(MarkHandle m, const byte* buffer, long length);
    void internalRemoveAtMark(MarkHandle m, long amount);

public:
    long insertAtMark(MarkHandle m, const byte* buffer, long length);

    long insertAtMark(MarkHandle m, const String& chars) {
        return insertAtMark(m, (const byte*) chars.toCString(), chars.getLength());
    }

    long insertAtMark(MarkHandle m, byte c) {
        return insertAtMark(m, &c, 1);
    }
    long insertAtMark(MarkHandle m, const ByteArray& insertBuffer) {
        return insertAtMark(m, insertBuffer.getPtr(0), insertBuffer.getLength());
    }

    long undo(MarkHandle m);
    long redo(MarkHandle m);

    void removeAtMark(MarkHandle m, long amount);
    void clear();
    void reset();
    
    void moveMarkToLineAndColumn(MarkHandle m, long line, long column);
    void moveMarkToBeginOfLine(MarkHandle m);
    void moveMarkToEndOfLine(MarkHandle m);
    void moveMarkToNextLineBegin(MarkHandle m);
    void moveMarkToPrevLineBegin(MarkHandle m);
    void moveMarkToPos(MarkHandle m, long pos);
    void moveMarkToPosOfMark(MarkHandle m, MarkHandle toMark);

    void moveMarkForwardToPos(MarkHandle m, long pos) {
        TextMarkData& mark = marks[m.index];
        ASSERT(mark.pos <= pos);
        long markPos  = mark.pos;
        long markLine = mark.line;
        while (markPos < pos) {
            if (isEndOfLine(markPos)) {
                markPos += getLengthOfLineEnding(markPos);
                markLine += 1;
            } else {
                markPos += 1;
            }
        }
        mark.pos    = markPos;
        mark.line   = markLine;
        mark.column = pos - getThisLineBegin(pos);
    }
    
    void incMark(MarkHandle m) {
        moveMarkToPos(m, marks[m.index].pos + 1);
    }
    bool isEndOfText(MarkHandle m) {
        return marks[m.index].pos == buffer.getLength();
    }
    bool isEndOfLine(MarkHandle m) {
        return isEndOfLine(marks[m.index].pos);
    }
    void setInsertFilterCallback(Callback<const byte**, long*>::Ptr filterCallback);
    void registerUpdateListener(Callback<UpdateInfo>::Ptr updateCallback);
    void registerFileNameListener(Callback<const String&>::Ptr fileNameCallback);
    void registerReadOnlyListener(Callback<bool>::Ptr readOnlyCallback);
    void registerLengthListener(Callback<long>::Ptr lengthCallback);
    void registerModifiedFlagListener(Callback<bool>::Ptr modifiedFlagCallback);
    
    void flushPendingUpdatesIntern();
    void flushPendingUpdates() {
        if (changedAmount != 0 || oldEndChangedPos != 0) {
            flushPendingUpdatesIntern();
        }
    }
    
    long getBeginChangedPos() {return beginChangedPos;}
    long getChangedAmount()   {return changedAmount;}
    long getTextPositionOfMark(MarkHandle mark) const {
        return marks[mark.index].pos;
    }
    long getColumnNumberOfMark(MarkHandle mark) const {
        return marks[mark.index].column;
    }
    long getLineNumberOfMark(MarkHandle mark) const {
        return marks[mark.index].line;
    }
    
    String getFileName() const {
        return fileName;
    }
    
    bool isFileNamePseudo() const {
        return fileNamePseudoFlag;
    }
    
    bool getModifiedFlag() const {
        return modifiedFlag;
    }
    void setModifiedFlag(bool flag);
    
    bool hasHistory() const {
        return hasHistoryFlag;
    }
    
    void clearHistory();
    
    void activateHistory() {
        if (!hasHistory()) {
          history = EditingHistory::create();
          hasHistoryFlag = true;
        }
    }
    
    void setHistorySeparator();
    void setMergableHistorySeparator();
    
    void rememberChangeAreaInHistory(long spos, long epos) {
        if (hasHistory()) {
            history->rememberSelectAction(spos, epos - spos);
        }
    }
    
    class HistorySection : public HeapObject
    {
    public:
        typedef OwningPtr<HistorySection> Ptr;
        
        ~HistorySection() {
            if (textData.isValid() && modus == SET_SEPARATOR_AT_END) {
                historySectionHolder.invalidate();
                textData->setHistorySeparator();
            }
        }
        
    private:
        friend class TextData;
        
        enum Modus {
            DEFAULT_MODUS,
            SET_SEPARATOR_AT_END
        };
        
        static Ptr create(TextData*                          textData, 
                          EditingHistory::SectionHolder::Ptr historySectionHolder,
                          Modus                              modus = DEFAULT_MODUS)
        {
            return Ptr(new HistorySection(textData, historySectionHolder, modus));
        }
        
        HistorySection(TextData*                          textData, 
                       EditingHistory::SectionHolder::Ptr historySectionHolder,
                       Modus                              modus)
            : textData(textData),
              historySectionHolder(historySectionHolder),
              modus(modus)
        {}
        
        EditingHistory::SectionHolder::Ptr historySectionHolder;
        WeakPtr<TextData> textData;
        Modus modus;
    };
    
    HistorySection::Ptr createHistorySection() {
        if (hasHistory()) {
            setHistorySeparator();
            return HistorySection::create(this, 
                                          history->getSectionHolder(), 
                                          HistorySection::SET_SEPARATOR_AT_END);
        } else {
            return HistorySection::Ptr();
        }
    }
    
    HistorySection::Ptr getHistorySectionHolder() {
        if (hasHistory()) {
            return HistorySection::create(this, 
                                          history->getSectionHolder());
        } else {
            return HistorySection::Ptr();
        }
    }

    bool wasFileModifiedOnDisk() const {
        return modifiedOnDiskFlag;
    }
    
    bool wasFileModifiedOnDiskSinceLastIgnore() const {
        if (ignoreModifiedOnDiskFlag == true && fileInfo.exists()) {
            return !fileInfo.getLastModifiedTimeValSinceEpoche()
                            .isEqualTo(ignoreModifiedOnDiskTime);
        } else {
            return false;
        }
    }
    
    bool hasModifiedOnDiskFlagBeenIgnored() const {
        return ignoreModifiedOnDiskFlag;
    }
    
    void setIgnoreModifiedOnDiskFlag(bool newValue) {
        this->ignoreModifiedOnDiskFlag = newValue;
        if (newValue == true && fileInfo.exists()) {
            ignoreModifiedOnDiskTime = fileInfo.getLastModifiedTimeValSinceEpoche();
        }
    }
    
    bool isReadOnly() const {
        return isReadOnlyFlag;
    }
    
    const File::Info& getLastFileInfo() {
        return fileInfo;
    }
    
    void checkFileInfo();
    
private:

    friend class ViewCounterTextDataAccess;

    TextData();
    MemBuffer<byte> buffer;
    long numberLines;
    long beginChangedPos;
    long changedAmount;
    long oldEndChangedPos;
    ObjectArray<TextMarkData> marks;

    void updateMarks(
        long beginChangedPos, long oldEndChangedPos, long changedAmount,
        long beginLineNumber, long changedLineNumberAmount);
        
    CallbackContainer<UpdateInfo> updateListeners;
    CallbackContainer<const String&> fileNameListeners;
    CallbackContainer<bool> readOnlyListeners;
    CallbackContainer<long> lengthListeners;
    CallbackContainer<bool> changedModifiedFlagListeners;
    
    Callback<const byte**, long*>::Ptr filterCallback;
    
    String fileName;
    long oldLength;
    bool modifiedFlag;
    int viewCounter;
    bool hasHistoryFlag;
    EditingHistory::Ptr history;
    bool isReadOnlyFlag;
    bool modifiedOnDiskFlag;
    bool ignoreModifiedOnDiskFlag;
    TimeVal ignoreModifiedOnDiskTime;
    File::Info fileInfo;
    
    bool fileNamePseudoFlag;
};


class ViewCounterTextDataAccess
{
protected:
    int getViewCounter(RawPtr<TextData> textData) {
        return textData->viewCounter;
    }
    void decViewCounter(RawPtr<TextData> textData) {
        textData->viewCounter -= 1;
    }
    void incViewCounter(RawPtr<TextData> textData) {
        textData->viewCounter += 1;
    }
};

} // namespace LucED

#endif // TEXT_DATA_HPP
