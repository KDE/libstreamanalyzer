/* This file is part of Strigi Desktop Search
 *
 * Copyright (C) 2006 Jos van den Oever <jos@vandenoever.info>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifndef UNICODEMAP_H
#define UNICODEMAP_H

#include "pdfobjects.h"
#include <strigi/streambase.h>
#include <string>
#include <vector>
#include <list>

/**
 * UnicodeMap objects are used to convert encoded PDF string to utf8.
 * The following pieces of information can be used to construct the object:
 * - BaseEncoding (occurring either in the Encoding entry of a Font dict
 *   or the BaseEncoding entry of an Encoding dict)
 * - Differences arrays (differences arrays are found the Differences entry
 *   of a Encoding dict)
 * - conversion ranges from ToUnicode streams
 * 
 * Note: ligatures such as ff, fi or fl are converted to normal characters.
 */
class UnicodeMap {
public:
    enum BaseEncoding {WinAnsi, MacRoman, Standard, PdfDoc, None};
    
    /** This character is used to denote whitespace with all encodings;
     */
    static const char WHITESPACE_CHAR;
    
    /**
     * Convert one PDF string to utf8
     */
    std::string convert(std::string&);

    UnicodeMap() : baseEncoding(None) {}
    UnicodeMap(BaseEncoding baseEnc) : baseEncoding(baseEnc) {}
        
    /**
     * Set base encoding. Normally "MacRomanEncoding" or 
     * "WinAnsiEncoding"
     */
    void setBaseEncoding(BaseEncoding baseEnc);
    void setBaseEncoding(std::string& encodingName);
    
    /**
     * Add a differences array
     */
    void addDifferences(PdfArray* array);
    
    /**
     * Add a ToUnicode range or a single char
     */
    void addRange(std::string srcStart, std::string srcEnd, std::string dstStart);
    void addRange(std::string srcStart, PdfArray* dstCodes);
    void addChar(std::string srcCode, std::string dstCode);
    
    // For debugging
    std::string toString();   
    
private:    
    class ConversionRange {
    public:
        int32_t convertChar(std::string& srcString, int32_t srcOffset, std::string& dstString);        
        ConversionRange(int64_t srcRangeStart, int64_t rangeLen, std::string& dstRangeStart);
        ConversionRange(int64_t srcRangeStart, std::vector<std::string>& dstChars);        
    private:
        int64_t srcRangeStart;
        std::string dstRangeStart;
        int64_t rangeLen;
        int32_t nBytes;        
        std::vector<std::string> charcodes;
    };
    
    BaseEncoding baseEncoding;    
    std::list<ConversionRange> ranges;
    
    int32_t convertChar(std::string& srcString, int32_t srcOffset, std::string& dstString);
        
    static std::string toUtf8(int32_t);
    static std::string toUtf8(std::string&);
    static int bytesInInt(int64_t integer);    
    static std::string intToString(int64_t ch, int32_t nBytes = -1);
    static int64_t stringToInt(std::string& str, int32_t nBytes = -1, int32_t offset = 0);
};

#endif