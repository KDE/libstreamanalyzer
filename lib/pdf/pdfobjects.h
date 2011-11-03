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

#ifndef PDFOBJECTS_H
#define PDFOBJECTS_H

#include <strigi/streambase.h>
#include <string>
#include <vector>
#include <map>
#include <list>


/**
 * Classes for PDF objects including string, name, operator, number,
 * indirect reference, array and dictionary. We are not interested in
 * booleans or the null object so they are not included.
 */

class PdfObject {
public:
    virtual ~PdfObject() {}
    virtual std::string toString() = 0;
    virtual size_t totalSize();
};

/**
 * PdfString, PdfName and PdfOperator are essentially 
 * wrappers for std::string
 */

class PdfString : public std::string, public PdfObject {
public:
    PdfString () : std::string() {}
    PdfString (const PdfString& other) : std::string(other) {}
    PdfString ( const char * s, size_t n ) : std::string(s, n) {}
    std::string toString();
    size_t totalSize();
};

class PdfName : public std::string, public PdfObject {
public:
    PdfName () : std::string() {}
    PdfName (const PdfName& other) : std::string(other) {}
    PdfName ( const char * s ) : std::string(s) {}
    PdfName ( const char * s, size_t n ) : std::string(s, n) {}
    PdfName& assign(std::string& s);
    PdfName& assign(const char* s);
    std::string toString();
    size_t totalSize();
};

class PdfOperator : public std::string, public PdfObject {
public:
    PdfOperator () : std::string() {}
    PdfOperator (const PdfOperator& other) : std::string(other) {}
    PdfOperator ( const char * s, size_t n ) : std::string(s, n) {}
    std::string toString();
    size_t totalSize();
};

class PdfNumber : public std::pair<int32_t, int32_t>, public PdfObject {
public:
    int32_t integerPart() { return first; }
    int32_t fractionalPart() { return second; }
    
    PdfNumber() : pair() {}
    PdfNumber(int32_t intPart, int32_t fracPart = 0) : pair(intPart, fracPart) {}
    
    std::string toString();
};

class PdfReference : public std::pair<int32_t, int32_t>, public PdfObject {
public:
    int32_t index() {return first; }
    int32_t generation() {return second; }
    
    PdfReference() : pair() {}
    PdfReference(int32_t index, int32_t generation) : pair(index, generation) {}  
    
    std::string toString();

};
  
/**
 * Arrays and dictionaries are nested objects, they contain
 * other PdfObjects. The contents are deleted in the destructor,
 * but for example clear() won't call the destructor of the contents
 */

class PdfArray : public std::vector<PdfObject*>, public PdfObject {
public:
    PdfArray() : std::vector<PdfObject*>() {}    
    ~PdfArray() { deleteContents(); }
    
    void deleteContents();
        
    std::string toString();
    size_t totalSize();
};

class PdfDictionary : public std::map<PdfName, PdfObject*>, public PdfObject {
public:
    PdfDictionary() : std::map<PdfName, PdfObject*>() {}        
    ~PdfDictionary() { deleteContents(); }
    
    void deleteContents();
    
    bool hasKey(const char* key);
    bool isOfType(const char* type);
    PdfObject* get(const char* key) { return get(PdfName(key)); }
    PdfObject* get(const PdfName& key);
           
    std::string toString();
    size_t totalSize();
};

#endif