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

#include "unicodemap.h"
#include "latin_tables.h"
#include <typeinfo>
#include <sstream>
#include <iostream>

const char UnicodeMap::WHITESPACE_CHAR = 0;

using namespace std;

/** -------------------------------------------------------------------------
 * Set base encoding
 */
void
UnicodeMap::setBaseEncoding(BaseEncoding enc) {
    baseEncoding = enc;
}
void
UnicodeMap::setBaseEncoding(string& encodingName) {
    if (encodingName == "WinAnsiEncoding") baseEncoding = WinAnsi;
    else if (encodingName == "MacRomanEncoding") baseEncoding = MacRoman;
    else baseEncoding = None;
}

/**-------------------------------------------------------------------------
 * Convert an encoded string to utf8
 */
string
UnicodeMap::convert(string& str) {
    string result;
    int32_t position = 0;
    while (position < (int32_t) str.length()) {
        int bytesRead = convertChar(str, position, result);
        position += (bytesRead > 0)? bytesRead : 1;        
    }    
    return result;
}

/**-------------------------------------------------------------------------
 * These methods correspond to the BfRange and BfChar commands in
 * ToUnicode streams
 */
void
UnicodeMap::addRange(string srcStart, string srcEnd, string dstStart) {
    int64_t start = stringToInt(srcStart);
    int64_t end = stringToInt(srcEnd);
    ranges.push_back(ConversionRange(start, end-start+1, dstStart));    
}    
void
UnicodeMap::addRange(string srcStart, PdfArray* dstCodes) {
    vector<string> array;
    for (PdfArray::iterator it = dstCodes->begin(); it != dstCodes->end(); it++) {
        array.push_back(string(*(PdfString*) (*it)));        
    }                    
    ranges.push_back(ConversionRange(stringToInt(srcStart), array));
}
void
UnicodeMap::addChar(string srcCode, string dstCode) {
    ranges.push_back(ConversionRange(stringToInt(srcCode), 1, dstCode));
}

/**-------------------------------------------------------------------------
 * Differences from baseEncoding. array is the value of the Differences entry
 * in a Font dictionary
 */
void
UnicodeMap::addDifferences(PdfArray* array) {    
    PdfArray::iterator it = array->begin();
    while (it != array->end()) {
        vector<string> dstCodes;
        unsigned char rangeStart;
        PdfNumber* num = dynamic_cast<PdfNumber*> (*it);        
        if  (!num) return;  // Unexpected object
        rangeStart = num->integerPart();        
        while (++it != array->end() && typeid(**it) == typeid(PdfName)) {
            PdfName* symbolName = (PdfName*) *it;
            // See if this is a standard latin symbol name
            for (int i=0; i<latinEncodingTableLen; i++) {
                if (*symbolName == latinEncodingTable[i].name) {
                    unsigned char pdfDocCode = latinEncodingTable[i].pdf;
                    dstCodes.push_back(intToString(pdfDocUnicodeCodes[pdfDocCode], 2));
                }
            }
        }       
        ranges.push_back(ConversionRange(rangeStart, dstCodes));
    }
}

/**-------------------------------------------------------------------------
 * Some information for debugging purposes
 */
string
UnicodeMap::toString() {
    stringstream buf;
    buf << "Unicode map, ranges " << ranges.size();
    return buf.str();
}

/**-------------------------------------------------------------------------
 * Convert a single character (one or more bytes long) in srcString at srcOffset.
 * Append result to dstString and return the number of bytes in the character in
 * srcString.
 */
int32_t
UnicodeMap::convertChar(string& srcString, int32_t srcOffset, string& dstString) {    
    if (srcString[srcOffset] == WHITESPACE_CHAR) {
        dstString += ' ';
        return 1;
    }    
    // First look through the ranges
    if (!ranges.empty()) {
        list<ConversionRange>::iterator it;
        for (it = ranges.begin(); it != ranges.end(); it++) {
            int32_t result = (*it).convertChar(srcString, srcOffset, dstString);            
            if (result != 0) return result;
        }    
    }    
    // Then fall back to base encoding
    char ch = (char) stringToInt(srcString, 1, srcOffset);
    if (baseEncoding != None) {
        for (int i=0; i<latinEncodingTableLen; i++) {
            if ((baseEncoding == WinAnsi && ch == latinEncodingTable[i].win) ||
                (baseEncoding == MacRoman && ch == latinEncodingTable[i].mac) ||
                (baseEncoding == Standard && ch == latinEncodingTable[i].std) ||
                (baseEncoding == PdfDoc && ch == (char) latinEncodingTable[i].pdf)) {
                dstString += toUtf8(pdfDocUnicodeCodes[latinEncodingTable[i].pdf]);
                return 1;
            }            
        }
        return 0;
    } else if (isascii(ch) && ch > 31) {
        // As a fallback, use the character as such if it's in the ascii range.
        dstString += ch;
        return 1;
    }  
    return 0;
}

/**-------------------------------------------------------------------------
 * Convert a utf16 string to utf8
 */
string
UnicodeMap::toUtf8(string& str) {
    string result;
    int position = 0;
    while (position < (int)str.length()-1) {
        result += toUtf8(stringToInt(str, 2, position));
        position += 2;
    }
    return result;
}

/**-------------------------------------------------------------------------
 * Convert a utf16 character to utf8
 * Ligatures (ff, fi, fl, ffi, ffl) are converted to normal characters
 */
string
UnicodeMap::toUtf8(int32_t utf16) {
    // Convert ligatures to normal chars
    if (utf16 == 0xFB00) return string("ff");
    if (utf16 == 0xFB01) return string("fi");
    if (utf16 == 0xFB02) return string("fl");
    if (utf16 == 0xFB03) return string("ffi");
    if (utf16 == 0xFB04) return string("ffl");    
    
    if (utf16 <= 0x007F) {
        char byte[1] = {(char) utf16};
        return string(byte, 1);
    }
    if (utf16 <= 0x07FF) {        
        char bytes[2];
        bytes[1] = (char) (0x80 + (0x003F & utf16));
        bytes[0] = (char) (0xC0 + ((0x00C0 & utf16) >> 6) + ((0x0700 & utf16) >> 6));
        return string(bytes, 2);
    }
    if (utf16 <= 0xFFFF) {
        char bytes[3];
        bytes[2] = (char) (0x80 + (0x003F & utf16));
        bytes[1] = (char) (0x80 + ((0x00C0 & utf16) >> 6) + ((0x0F00 & utf16) >> 6));
        bytes[0] = (char) (0xE0 + ((0xF000 & utf16) >> 12));
        return string(bytes, 3);
    }
    return string();
}
/**-------------------------------------------------------------------------
 * Number of bytes in an integer
 */
int 
UnicodeMap::bytesInInt(int64_t integer) {
    int nBytes = 0;
    while (integer >> (++nBytes)*8 != 0) {}
    return nBytes;
}

/**-------------------------------------------------------------------------
 * convert a sequence of bytes from integer to string representation and back
 */
string 
UnicodeMap::intToString(int64_t ch, int32_t nBytes) {
    if (nBytes == -1) nBytes = bytesInInt(ch);    
    string str(nBytes, ' ');
    for (int i=0; i<nBytes; i++) {
        str[nBytes-1-i] = (char) (ch >> (i*8)) & 0xFF;
    }
    return str;
}
int64_t 
UnicodeMap::stringToInt(string& str, int32_t nBytes, int32_t offset) {
    if (nBytes == -1) nBytes = str.length();
    int64_t val = 0;
    for(int i=0; i<nBytes; i++) {
        val += (int64_t) ((unsigned char) str[offset+nBytes-1-i])<<(i*8);
    }
    return val;
}

/**-------------------------------------------------------------------------
 * UnicodeMap::ConversionRange methods
 */
UnicodeMap::ConversionRange::ConversionRange(int64_t srcStart, int64_t len, string& dstStart) 
    : srcRangeStart(srcStart), dstRangeStart(dstStart), rangeLen(len) {
    nBytes = bytesInInt(srcRangeStart+rangeLen);
}
    
UnicodeMap::ConversionRange::ConversionRange(int64_t srcStart, vector<string>& dstCodes)
    : srcRangeStart(srcStart), charcodes(dstCodes) {
    rangeLen = charcodes.size();
    nBytes = bytesInInt(srcRangeStart+rangeLen);
}

/**-------------------------------------------------------------------------
 * Convert a single char in srcString at srcOffset. Append result to dstString
 * and return number of bytes in the src char.
 */
int32_t
UnicodeMap::ConversionRange::convertChar(string& srcString, int32_t srcOffset, string& dstString) {
    if (nBytes > 7) return 0;
    if ((int32_t) srcString.length() - srcOffset < nBytes) return 0;
    int64_t ch = stringToInt(srcString, nBytes, srcOffset);
    int64_t offset = ch - srcRangeStart;    
    if (offset >= 0 && offset < rangeLen) {
        if (!dstRangeStart.empty()) {            
            string result =  dstRangeStart;
            result[result.length()-1] += offset;
            dstString += toUtf8(result);            
            return nBytes;
        } else {           
            dstString += toUtf8(charcodes[offset]);            
            return nBytes;
        }
    } else return 0;
}        