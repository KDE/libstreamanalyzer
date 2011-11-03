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
#include "pdfreader.h"
#include <strigi/subinputstream.h>
#include <strigi/stringterminatedsubstream.h>
#include <typeinfo>
#include <cstdlib>
#include <cstring>

using namespace Strigi;
using namespace std;

PdfReader::PdfReader(StreamBase<char>* s) 
    : start(0), end(0), pos(0), bufferStart(0), lastObject(0) {
    setStream(s);
}
    
PdfReader::~PdfReader() {
    if (lastObject) delete lastObject;
}

void
PdfReader::setStream(StreamBase<char>* s) {
    stream = s;
    if (stream) stream->reset(0);
}
 
/**-------------------------------------------------------------------------
 * Delete last object and set it to newObj
 */
void
PdfReader::replaceLastObject(PdfObject* newObj) {
     if (lastObject)  delete lastObject;
     lastObject = newObj;   
}

/**-------------------------------------------------------------------------
 * Expand the buffer with at least min bytes but reset the stream afterwards to
 * the old position
 */
StreamStatus
PdfReader::read(int32_t min, int32_t max) {
    int32_t off = (int32_t)(pos-start);
    int32_t d = (int32_t)(stream->position() - bufferStart);
    min += d;
    if (max > 0) max += d;
    stream->reset(bufferStart);
    int32_t n = stream->read(start, min, max);
    //printf("read: min = %i, n = %i\n", min, n);
    if (n < min) return stream->status();
    pos = start + off;
    end = start + n;
    return Ok;
}

/**-------------------------------------------------------------------------
 * Ensure that the buffer contains data up to pos+m
 */
StreamStatus
PdfReader::checkForData(int32_t m) {
    StreamStatus n = Ok;
    if (end - pos < m) {
        n = read(m - (int32_t) (end-pos), 0);
    }
//    fprintf(stderr, "checkForData %i\n", n);
    return n;
}

/**-------------------------------------------------------------------------
 * Helper methods
 */
bool
PdfReader::isInString(char c, const char* s, int32_t n) {
    for (int i=0; i<n; ++i) {
        if (s[i] == c) return true;
    }
    return false;
}
StreamStatus
PdfReader::skipDigits() {
    StreamStatus s;
    do {
        if ((s = checkForData(1)) != Ok) return s;
        while (pos < end && isdigit(*pos)) pos++;
    } while (pos == end);
    return Ok;
}
StreamStatus
PdfReader::skipFromString(const char*str, int32_t n) {
    StreamStatus s;
    do {
        if ((s = checkForData(1)) != Ok) return s;
        while (pos < end && isInString(*pos, str, n)) pos++;
    } while (pos == end);
    return Ok;
}
StreamStatus
PdfReader::skipNotFromString(const char*str, int32_t n) {
    StreamStatus s;
    do {
        if ((s = checkForData(1)) != Ok) return s;
        while (pos < end && !isInString(*pos, str, n)) pos++;
    } while (pos == end);
    return Ok;
}
StreamStatus
PdfReader::skipKeyword(const char* str, int32_t len) {
    StreamStatus s = checkForData(len);
    if (s != Ok) {
            m_error.assign("Premature end of stream.");
            return Error;
    }
//    printf("skipKeyword %s '%.*s'\n", str, (len>end-pos)?end-pos:len, pos);
    if (strncmp(pos, str, len) != 0) {
            m_error.assign("Keyword ");
            m_error.append(str, len);
            m_error.append(" not found.");
            return Error;
    }
    pos += len;
    return Ok;
}
/**-------------------------------------------------------------------------
 * Skip whitespace in the stream. Return amount of whitespace skipped.
 * After calling this function the position in the stream is after the
 * whitespace. Skip characters from \t\n\f\r\t and space.
 **/
StreamStatus
PdfReader::skipWhitespace() {
    StreamStatus s;
    do {
        if ((s = checkForData(1)) != Ok) return s;
        while (pos < end && (isspace(*pos) || *pos == 0x00)) pos++;
    } while (pos == end);
    return Ok;
}
/**-------------------------------------------------------------------------
 * Parse PDF comment
 */
StreamStatus
PdfReader::parseComment() {
    if (*pos != '%') return Ok;
    pos++; // skip '%'
    StreamStatus s = skipNotFromString("\r\n", 2);    
    return s;    
}

/**-------------------------------------------------------------------------
 * Skip all whitespace and comments between PDF objects
 */
StreamStatus
PdfReader::skipWhitespaceOrComment() {
//    printf("skipWhitespaceOrComment\n");
    int64_t o;
    int64_t no = pos - start;
    StreamStatus s;
    do {
        o = no;
        if ((s = skipWhitespace()) != Ok) return s;
        if ((s = parseComment()) != Ok) return s;
        no = pos - start;
    } while (o != no);
    return Ok;
}

/**-------------------------------------------------------------------------
 * Skip a number object
 */
StreamStatus
PdfReader::skipNumber() {
    char ch = *pos;
    if (ch == '+' || ch == '-') pos++;
    StreamStatus n = skipDigits();
    if (n != Ok) return n;
    if (pos < end && *pos == '.') {
        pos++;
        n = skipDigits();
    }
    return n;
}
/**-------------------------------------------------------------------------
 * Parse a PDF number object
 * - number : [+-]?\d+(.\d+)?
 */
StreamStatus
PdfReader::parseNumber() {
    int32_t intPart;
    int32_t fracPart = 0;    
    int64_t p = pos - start;
    char ch = *pos;
    if (ch == '+' || ch == '-') pos++;
    StreamStatus n = skipDigits();
    if (n != Ok) return n;
    const char* s = start + p;
    intPart = strtol(s, 0, 10);
    if (pos < end && *pos == '.') {
        // a '.' so this is a float
        pos++;
        p = pos - start;
        n = skipDigits();
        if (n != Ok) return n;
        s = start + p;
        fracPart = strtol(s, 0, 10);
    }
    replaceLastObject(new PdfNumber(intPart, fracPart));    
    return n;
}

/**-------------------------------------------------------------------------
 * An object starting with a digit can be either a number or an indirect
 * reference
 */
StreamStatus
PdfReader::parseNumberOrIndirectObject() {
//    printf("parseNumberOrIndirectObject\n");
    StreamStatus s = parseNumber();
    if (s != Ok) return s;
    s = skipWhitespace();
    if (s != Ok) return s;
    // now we must check if this is an indirect object
    int64_t p = pos - start;
    if (isdigit(*pos)) {
        skipDigits();
        s = skipWhitespace();
        if (s != Ok) return s;
        if (*pos == 'R') {
            pos++;
            const char* s = start + p;
            int32_t generation = strtol(s, 0, 10);
            int32_t index = lastNumber()->integerPart();
            replaceLastObject(new PdfReference(index, generation));
        } else {
            // set the position in front of the previous number
            // because it is a separate number and not part of a reference
            pos = start + p;
        }        
    }
    return Ok;
}


/**-------------------------------------------------------------------------
 * Parse a PDF literal string
 */
StreamStatus
PdfReader::parseLiteralString() {
    StreamStatus s;
    int par = 1;
    pos++;
    bool escape = false;
    
    PdfString* str = new PdfString();   
    while ((s = checkForData(1)) == Ok) {
        char c = *pos;
        if (escape) {
            escape = false;
            if (isdigit(c)) {  // This is an 3 number octal code
                if ((s = checkForData(3)) != Ok 
                    || !isdigit(*(pos+1)) || !isdigit(*(pos+2))) {
                    continue;
                }
                char buf[4] = {c, ' ', ' ', '\n'};                
                for (int i=1; i<3; i++) buf[i] = *(++pos);
                *str += (char) strtol(buf, 0, 8);                    
            } else if (c == 'n' || c == 'r' || c == 't') {
                *str += ' ';
            } else if (c == '\\' || c == '(' || ')') {
                *str += c;
            }
        } else {
            if (c == ')') {
                if (--par == 0) {
                    pos++;
                    break;
                }
                *str += c;
            } else if (c == '(') {
                *str += c;
                par++;
            } else if (c == '\\') {
                escape = true;
            } else {
                *str += c;
            }
        }
        pos++;
    } 
    replaceLastObject(str);
    return s;
}

/**-------------------------------------------------------------------------
 * Parse a hex string, such as <0D351B86>
 */
StreamStatus
PdfReader::parseHexString() {
    skipKeyword("<", 1);
//    fprintf(stderr, "parseHexString\n");
    char chBuf[3] = "00";
    PdfString* str = new PdfString();
    while (true) {
        StreamStatus r = checkForData(2);
        if (r != Ok) {
            delete str;
            return r;
        }
        if (!isxdigit(*pos) || !isxdigit(*(pos+1))) break;
        chBuf[0] = *(pos++);
        chBuf[1] = *(pos++);
        *str += (char) strtol(chBuf, 0, 16);
    }    
    replaceLastObject(str);
//    printf("parseHexString ok\n");
    return skipKeyword(">", 1);
}

/**-------------------------------------------------------------------------
 * Parse a PDF name object. Names start with a slash '/'
 */
StreamStatus
PdfReader::parseName() {
//    printf("parseName %.*s\n", (10>end-pos)?end-pos:10, pos);
    pos++;
    int64_t p = pos - start;
    StreamStatus r = skipNotFromString("()<>[]{}/%\t\n\f\r ", 16);
    if (r == Error) {
        m_error.assign(stream->error());
        return r;
    }
    const char *s = start + p;
    replaceLastObject(new PdfName(s, pos-s));    
    return r;
}

/**-------------------------------------------------------------------------
 * Recursively parse the contents of a PDF dictionary
 */
StreamStatus
PdfReader::parseDictionary() {    
    pos += 2;
    StreamStatus r = skipWhitespaceOrComment();
    if (r != Ok) return r;
    
    PdfDictionary* dict = new PdfDictionary();    
    while (*pos != '>') {
        if (parseName() != Ok) {
            m_error.assign("Expected a name.");
            r = Error;
            break;
        }        
        PdfName key(*(PdfName*) lastObject);
        
        if (skipWhitespaceOrComment() != Ok) {
            m_error.assign("Error parsing whitespace in dictionary.");
            r = Error;
            break;
        }
        replaceLastObject(0);
        if (parseObject(0) != Ok) {
            m_error.assign("Error parsing dictionary value.");
            r = Error;
            break;
        }                  
        if (lastObject) {
            (*dict)[key] = lastObject;
            lastObject = 0;
        }        
        if (skipWhitespaceOrComment() != Ok) {
            m_error.assign("Error reading whitespace after dictionary value.");
            r = Error;
            break;
        }
    }
    if (r == Ok) {
        replaceLastObject(dict);
        return skipKeyword(">>", 2);
    } else {
        delete dict;
        return Error;
    }
}

/**-------------------------------------------------------------------------
 * Recursively parse the contents of an array
 */
StreamStatus
PdfReader::parseArray(int nestDepth) {
    pos++; 
    StreamStatus r;
    
    r = skipWhitespaceOrComment();
    if (r != Ok) return r;
    
    PdfArray* arr = new PdfArray();
    while (*pos != ']') {
        replaceLastObject(0);
        r = parseObject(nestDepth+1);
        if (r != Ok) break;
        if (lastObject) {
            arr->push_back(lastObject);
            lastObject = 0;
        }
        r = skipWhitespaceOrComment();
        if (r != Ok) break;
    }
    if (r == Error) {
        delete arr;
    } else {   
        pos++;
        replaceLastObject(arr);
    }
    return r;
}

/**-------------------------------------------------------------------------
 * Whether a character can belong to a keyword
 */
bool
PdfReader::isKeywordChar(char ch) {
    return (isalpha(ch) || ch == '\'' || ch == '"' || ch == '*');
}

/**-------------------------------------------------------------------------
 * Parse a keyword
 */
StreamStatus
PdfReader::parseKeyword() {
    int64_t p = pos - start;    
    while ((checkForData(1) == Ok) && isKeywordChar(*pos)) {        
        pos++;
    } 
    StreamStatus r = stream->status();
    if (r == Error) {
        m_error.assign(stream->error());
        return r;
    }
    const char *s = start + p;
    lastKeyword.assign(s, pos-s);
    return r;
}

/**-------------------------------------------------------------------------
 * Parse one PDF object. If the object is one of the following
 * - literal string or hex string
 * - name
 * - indirect reference
 * - number
 * - array
 * - dictionary
 * it will be saved to lastObject. The previous lastObject will be deleted
 * unless lastObject is set to 0. Keywords are handled differently because
 * they can represent different types of PDF objects (boolean, null, operator)
 * or can be just keywords like 'endobj'. There is a separate lastKeyword member.
 */
StreamStatus
PdfReader::parseObject(int nestDepth) {
    //printf("parseObject %.*s\n", (5>end-pos)?end-pos:5, pos);
    StreamStatus r = checkForData(2);
    if (r != Ok) return r;
    if (nestDepth > 1000) return Error; // treat nesting 1000 levels as an error
    lastKeyword.clear();
    
    char ch = *pos;
    if (ch == '+' || ch == '-' || ch == '.' || isdigit(ch)) {
        //printf("Parse number or indirect object\n");
        r = parseNumberOrIndirectObject();
    } else if (ch == '(') {
        //printf("Parse literal string\n");
        r = parseLiteralString();
    } else if (ch == '/') {
        //printf("Parse name\n");
        r = parseName();
    } else if (ch == '<') {
        if (end-pos > 1 && pos[1] == '<') {
            //printf("Parse dictionary\n");
            r = parseDictionary();
        } else {
            //printf("Parse hex string\n");
            r = parseHexString();
        }
    } else if (ch == '[') {
        //printf("Parse array\n");
        r = parseArray(nestDepth+1);
    } else if (isKeywordChar(ch)) {        
        //printf("Parse keyword\n");
        r = parseKeyword();
    } else {
        m_error.assign("Invalid Pdf object\n");
        return Error;
    }
    if (r != Ok) return r;
    return skipWhitespaceOrComment();    
}

/**-------------------------------------------------------------------------
 * PdfBodyReader methods
 */

PdfBodyReader::PdfBodyReader(StreamBase<char>* s) 
    : PdfReader(s), lastDictionary(0), trailer(0), nextStream(0) {}
    
PdfBodyReader::~PdfBodyReader() {
    if (lastDictionary) delete lastDictionary;
    if (nextStream) delete nextStream;
    if (trailer) delete trailer;
}

/**-------------------------------------------------------------------------
 * We are not interested in the cross reference table
 */
StreamStatus
PdfBodyReader::skipXRef() {
    // skip header
    if (skipKeyword("xref", 4) != Ok || skipWhitespaceOrComment() != Ok) {
        return Error;
    }
    while (isdigit(*pos)) {
        // parse number of entries
        if (skipNumber() != Ok || skipWhitespaceOrComment() != Ok
                || parseNumber() != Ok || skipWhitespaceOrComment() != Ok) {
            return Error;
        }        
        int32_t entries = lastNumber()->integerPart();
        for (int i = 0; i != entries; ++i) {
            if (skipNumber() != Ok || skipWhitespaceOrComment() != Ok
                || skipNumber() != Ok || skipWhitespaceOrComment() != Ok
                || skipFromString("fn", 2) != Ok
                || skipWhitespaceOrComment() != Ok) {
                    return Error;
            }
        }
    }
    return Ok;
}

StreamStatus
PdfBodyReader::skipStartXRef() {
    if (skipKeyword("startxref", 9) != Ok || skipWhitespaceOrComment() != Ok
            || skipNumber() != Ok) {
        return Error;
    }
    return skipWhitespaceOrComment();
}

/**-------------------------------------------------------------------------
 * Parse the trailer. The trailer dictionary is saved
 */
StreamStatus
PdfBodyReader::parseTrailer() {
    if (skipKeyword("trailer", 7) != Ok || skipWhitespaceOrComment() != Ok
            || parseDictionary() != Ok || skipWhitespaceOrComment() != Ok) {
        return Error;
    }
    if (!trailer) {
        trailer = dynamic_cast<PdfDictionary*> (lastObject);
        lastObject = 0;
    }    
    return skipWhitespaceOrComment();
}

/**-------------------------------------------------------------------------
 * Go the beginning of the stream and create the strigi substream
 */
StreamStatus
PdfBodyReader::initializeNextStream() {   
    if (checkForData(3) != Ok) return Error;
    if (*pos == 0x0D) pos++; // Carriage return
    if (*pos != 0x0A) {      // Line feed        
        return Error;
    }
    pos++;
    int64_t p = bufferStart + pos-start;
    if (p != stream->reset(p)) {
        m_error.assign("Error resetting stream");
        return Error;
    }            
    PdfNumber* length = dynamic_cast<PdfNumber*> (lastDictionary->get("Length"));
    if (length) {                
        nextStream = new SubInputStream(stream, length->integerPart());
    } else {
        nextStream = new StringTerminatedSubStream(stream, "endstream");            
    }            
    return Ok;
}

/**-------------------------------------------------------------------------
 * Delete the last stream and seek to the position after 'endobj' keyword
 */
StreamStatus
PdfBodyReader::finalizeLastStream() {
    if (nextStream->status() != Eof) {
        m_error.assign("Previous stream not read to Eof");
        return Error;
    }
    delete nextStream;
    nextStream = 0;
    // After reading the substream the pointers to the buffer are invalid.
    // Reset the buffer to the current stream position
    start = pos = end = 0;
    bufferStart = stream->position();
    if (skipWhitespaceOrComment() != Ok || parseKeyword() != Ok) {
        m_error.assign("Read error after stream");
        return Error;
    }
    if (lastKeyword == "endstream") {
        if (skipWhitespaceOrComment() != Ok || parseKeyword() != Ok) {                                
            return Error;
        }            
    }    
    if (lastKeyword != "endobj") {
        m_error.assign("Keyword endobj not found");
        return Error;        
    }
    return Ok;
}

/**-------------------------------------------------------------------------
 * Parse next PDF body object
 */
StreamStatus
PdfBodyReader::parseNextObject() {
    if (nextStream && finalizeLastStream() != Ok) {
        return Error;
    } 
    if (lastDictionary) delete lastDictionary;
    lastDictionary = 0;
    
    StreamStatus r = skipWhitespaceOrComment();
    if (r != Ok) {
        m_error.assign("Stream not readable");       
        return r;
    }
   
    if (*pos == 'x') return skipXRef();
    if (*pos == 't') return parseTrailer();
    if (*pos == 's') return skipStartXRef();
            
    // Parse the object index and generation number
    if (parseNumber() != Ok) {
        m_error.assign("Error parsing a Pdf body object\n");
        return Error;
    }    
    int32_t objIndex = lastNumber()->integerPart();      
    if (skipWhitespaceOrComment() != Ok || parseNumber() != Ok) {
         m_error.assign("Error parsing a Pdf body object\n");
         return Error;
    }  
    objectRef = PdfReference(objIndex, lastNumber()->integerPart());
    if (skipWhitespaceOrComment() != Ok
        || skipKeyword("obj", 3) != Ok || skipWhitespaceOrComment() != Ok
        || parseObject(0) != Ok || skipWhitespaceOrComment() != Ok) {
        return Error;
    }       
    
    if (lastObject && typeid(*lastObject) == typeid(PdfDictionary)) {
        lastDictionary = (PdfDictionary*) lastObject;
        lastObject = 0;       
    }
    if (parseKeyword() == Error) {
        m_error.assign("Error in reading after object.");
        return Error;
    }
    if (lastKeyword == "stream" && lastDictionary) {
        if (initializeNextStream() != Ok) return Error;
    }
    else if (lastKeyword != "endobj") {
        m_error.assign("Keyword endobj not found.");
        return Error;
    }
    r = skipWhitespaceOrComment();  
    return r;
}

/**-------------------------------------------------------------------------
 * ContentStreamReader methods
 */

ContentStreamReader::ContentStreamReader(StreamBase<char>* s) 
    : PdfReader(s) {}
    
ContentStreamReader::~ContentStreamReader() {}
    
/**-------------------------------------------------------------------------
 * Parse next content stream operator and its arguments
 */
StreamStatus
ContentStreamReader::parseContentStreamObject() {
    lastKeyword.clear();    
    StreamStatus r = parseObject(0);
    if (r == Error) return r;
    
    if (!lastKeyword.empty() && lastKeyword != "null" && lastKeyword != "true" && lastKeyword != "false") {
        lastOperator.assign(lastKeyword);
    }    
    if (r != Ok) return r;
    return skipWhitespaceOrComment();    
}

/**-------------------------------------------------------------------------
 * Parse until the next operator. Save the operator to lastOperator and the
 * preceding objects to lastArguments.
 */
StreamStatus
ContentStreamReader::parseNextCommand() {
    lastOperator.clear();    
    lastArguments.deleteContents();
    lastArguments.clear();    
    replaceLastObject(0);
    StreamStatus r = skipWhitespaceOrComment();
    if (r != Ok) return r;       
    while (r == Ok && lastOperator.empty()) {
        r = parseContentStreamObject();
        if (r == Error) return r;
        if (lastObject) {
            lastArguments.push_back(lastObject);
            lastObject = 0;            
        }
    }    
    return r;
}

/**-------------------------------------------------------------------------
 * ObjectStreamReader methods
 */

ObjectStreamReader::ObjectStreamReader(StreamBase<char>* s, PdfDictionary* dict)
        : PdfReader(s), lastDictionary(0) {
    PdfNumber* nValue = dynamic_cast<PdfNumber*> ((*dict)[PdfName("N")]);
    if (nValue) {
        nObjects = nValue->integerPart();
    } else {
        nObjects = 0;
    }
}
ObjectStreamReader::~ObjectStreamReader(){
    if (lastDictionary) delete lastDictionary;
}

/**-------------------------------------------------------------------------
 * Parse next object in object stream
 */
StreamStatus
ObjectStreamReader::parseNextObject() {    
    //printf("Parse next object stream object\n");    
    if (lastDictionary) delete lastDictionary;
    lastDictionary = 0;
    StreamStatus r = skipWhitespaceOrComment();
    if (r != Ok) return r;    
    if (objectIndices.empty()) {
        for (int i=0; i<nObjects; i++) {
            if(parseNumber() != Ok || skipWhitespaceOrComment() != Ok) return Error;
            objectIndices.push_back(lastNumber()->integerPart());
            if(parseNumber() != Ok || skipWhitespaceOrComment() != Ok) return Error;
        }
    }
    r = parseObject(0);   
    if (lastObject && typeid(*lastObject) == typeid(PdfDictionary)) {
        lastDictionary = (PdfDictionary*) lastObject;
        lastObject = 0;        
    }
    objectRef = PdfReference(objectIndices.front(), 0);
    objectIndices.pop_front();
    r = skipWhitespaceOrComment();
    return r;
}  
