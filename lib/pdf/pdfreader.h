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
#ifndef PDFREADER_H
#define PDFREADER_H

#include "pdfobjects.h"
#include <strigi/streambase.h>
#include <string>


/**
 * PdfReader reads one primitive PDF object at a time. It saves
 * the potentially interesting objects to lastObject. It is not
 * meant to be used directly, use its subclasses instead.
 */
class PdfReader {
protected:
    Strigi::StreamBase<char>* stream;
    const char* start;
    const char* end;
    const char* pos;
    int64_t bufferStart;    
    std::string m_error;   
    PdfObject* lastObject;
    std::string lastKeyword;
    
    PdfReader();
    PdfReader(Strigi::StreamBase<char>* s = 0);
    virtual ~PdfReader();
    
    void replaceLastObject(PdfObject* newObj);
    PdfNumber* lastNumber() { return dynamic_cast<PdfNumber*> (lastObject); }    
    
    Strigi::StreamStatus read(int32_t min, int32_t max);
    Strigi::StreamStatus checkForData(int32_t m);    
    Strigi::StreamStatus skipFromString(const char*str, int32_t n);
    Strigi::StreamStatus skipNotFromString(const char*str, int32_t n);
    Strigi::StreamStatus skipWhitespaceOrComment();
    Strigi::StreamStatus skipWhitespace();
    Strigi::StreamStatus skipKeyword(const char* str, int32_t len);
    Strigi::StreamStatus skipDigits();    
    Strigi::StreamStatus skipNumber();
    Strigi::StreamStatus parseComment();
    Strigi::StreamStatus parseNumber();
    Strigi::StreamStatus parseNumberOrIndirectObject();
    Strigi::StreamStatus parseLiteralString();
    Strigi::StreamStatus parseHexString();
    Strigi::StreamStatus parseName();    
    Strigi::StreamStatus parseKeyword();
    Strigi::StreamStatus parseDictionary();    
    Strigi::StreamStatus parseArray(int nestDepth);
    Strigi::StreamStatus parseObject(int nestDepth);    
    bool isInString(char c, const char* s, int32_t n);
    bool isKeywordChar(char ch);        
    
public:
    void setStream(Strigi::StreamBase<char>* s);
    std::string& error() { return m_error; }
};

/**
 * A class for parsing the PDF body.
 */
class PdfBodyReader : public PdfReader {
private: 
    Strigi::StreamStatus skipXRef();
    Strigi::StreamStatus skipStartXRef();
    Strigi::StreamStatus parseTrailer();
    Strigi::StreamStatus initializeNextStream();
    Strigi::StreamStatus finalizeLastStream();
    
public:    
    /**
     * The trailer contains the PDF trailer if has been found, otherwise it is 0
     */    
    PdfReference objectRef;
    PdfDictionary* lastDictionary;    
    Strigi::StreamBase<char>* nextStream;
    PdfDictionary* trailer;
    
    PdfBodyReader(Strigi::StreamBase<char>* s = 0);
    ~PdfBodyReader();
    
    /**
    * Parse the next object in the PDF body. The object number and generation
    * is saved to the objectRef field. If the object is a dictionary, it can be 
    * accessed from the lastDictionary field. To prevent it from being deleted when
    * the method is next called, set it to 0. If there is a stream after a
    * dictionary, nextStream contains it as a strigi stream that can be handled
    * with other readers. If not, nextStream is 0. Streams must be read until
    * EOF before calling PdfBodyReader::parseNextObject again. 
    */    
    Strigi::StreamStatus parseNextObject();
};

/**
 * A class for parsing content streams, which are sequences or arguments and operators
 */
class ContentStreamReader : public PdfReader {
private:   
    Strigi::StreamStatus parseContentStreamObject();
        
public:
    PdfOperator lastOperator;
    PdfArray lastArguments;
    
    ContentStreamReader(Strigi::StreamBase<char>* s = 0);
    ~ContentStreamReader();
    
    /**
    * Parse the next operator and its arguments. The operator is saved to
    * lastOperator and arguments to lastArguments. lastArguments will be deleted
    * on the next call if it is not set to 0.
    */
    Strigi::StreamStatus parseNextCommand();
};

/**
 * Object streams can contain the same objects as the body except streams
 */
class ObjectStreamReader : public PdfReader {
private:    
    int32_t nObjects;
    std::list<int32_t> objectIndices;
    
public:
    PdfDictionary* lastDictionary;
    PdfReference objectRef;
    
    ObjectStreamReader(Strigi::StreamBase<char>* s, PdfDictionary* dict);
    ~ObjectStreamReader();
    
    /**
     * Parse the next object stream object. Similar to PdfBodyReader, except that
     * there are no streams.
     */
    Strigi::StreamStatus parseNextObject();
};

#endif
