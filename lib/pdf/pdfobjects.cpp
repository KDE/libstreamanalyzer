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

#include "pdfobjects.h"
#include <iostream>
#include <sstream>

using namespace std;

size_t
PdfObject::totalSize() { return sizeof(*this); }

size_t
PdfString::totalSize() {return sizeof(*this) + length(); }

size_t
PdfName::totalSize() {return sizeof(*this) + length(); }

size_t
PdfOperator::totalSize() {return sizeof(*this) + length(); }

string
PdfString::toString() { return *this; }

string
PdfName::toString() { return "/" + *this; }

string
PdfOperator::toString() { return *this; }

PdfName&
PdfName::assign(string& s) { string::assign(s); return *this; }

PdfName&
PdfName::assign(const char* s) { string::assign(s); return *this; } 

string
PdfNumber::toString() {stringstream buf; buf<<first<<"."<<second; return buf.str(); }

string
PdfReference::toString() { stringstream buf; buf<<first<<' '<<second<<" R"; return buf.str(); }

void
PdfArray::deleteContents() { for (iterator it=begin();it!=end();it++) delete (*it); }

string
PdfArray::toString() { 
    string buf = "[";
    for (iterator it=begin(); it!=end(); it++) buf += (*it)->toString()+" ";
    return buf + "]";
}

size_t
PdfArray::totalSize() {    
    size_t size = sizeof(*this) + this->size()*sizeof(PdfObject*);
    for (iterator it=begin(); it!=end(); it++) {
        size += (*it)->totalSize();
    }
    return size;
}

void
PdfDictionary::deleteContents() { for (iterator it=begin(); it!=end(); it++) delete (*it).second; }

bool
PdfDictionary::hasKey(const char* key) { return count(PdfName(key)) != 0; }

bool
PdfDictionary::isOfType(const char* type) {
    PdfName* t = dynamic_cast<PdfName*>(get("Type"));
    return (t && *t==type);     
}

PdfObject*
PdfDictionary::get(const PdfName& key) { return (count(key))? (*this)[key] : 0; }

size_t
PdfDictionary::totalSize() {
    size_t size = sizeof(*this) + this->size()*sizeof(pair<PdfName, PdfObject*>);
    for (iterator it=begin(); it!=end(); it++) {            
        if ((*it).second) size += (*it).second->totalSize();
    }
    return size;
}

string
PdfDictionary::toString() { 
    string buf = "<<"; 
    for (iterator it=begin(); it!=end(); it++) {
        buf += "/"+(*it).first+" "+(*it).second->toString()+"  ";
    }    
    return buf + ">>";         
}