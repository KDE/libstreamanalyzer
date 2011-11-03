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
#ifndef PDFPARSER_H
#define PDFPARSER_H

#include "unicodemap.h"
#include "pdfobjects.h"
#include "pdfreader.h"
#include <strigi/streambase.h>
#include <vector>
#include <list>
#include <map>
#include <set>

/**
 * PdfParser parses PDF files and sends all text occurring in the file
 * to texthandler in utf8 format. Streams are passed to streamhandler.
 * The PDF metadata can be accessed after parsing.
 */
class PdfParser {
public:
    class StreamHandler {
    public:
        virtual ~StreamHandler() {}
        virtual Strigi::StreamStatus handle(Strigi::StreamBase<char>* s) = 0;
    };
    class DefaultStreamHandler : public StreamHandler {
    public:
        Strigi::StreamStatus handle(Strigi::StreamBase<char>* s);
    };
    class TextHandler {
    public:
        virtual ~TextHandler() {}
        virtual Strigi::StreamStatus handle(const std::string& s) = 0;
    };
    class DefaultTextHandler : public TextHandler {
        Strigi::StreamStatus handle(const std::string& s);
    };

public:
    PdfParser();
    ~PdfParser();
    
    /**
     * Parse PDF file
     */
    Strigi::StreamStatus parse(Strigi::StreamBase<char>* stream);
    
    /**
     * Return the metadata dictionary found in the Info entry of the trailer
     * Possible keys include Title, Author, Subject, Keywords, Creator, Producer,
     * CreationDate, ModDate, Trapped.
     */     
    std::map<std::string, std::string> getMetadata();
    
    /**
     * Return the error description
     */
    const std::string& error() { return bodyReader.error(); }
    
    /**
     * Set handlers
     */    
    void setStreamHandler(StreamHandler* handler) { streamhandler = handler; }
    void setTextHandler(TextHandler* handler) { texthandler = handler; }    
    
private:
    PdfBodyReader bodyReader;
    
    // event handlers
    StreamHandler* streamhandler;
    TextHandler* texthandler;

    std::set<PdfReference> parsed;    
    std::map<PdfReference, PdfDictionary*> dictionaries;    
    std::list<PdfDictionary*> pageDictionaries;
    std::map<PdfReference, std::list<std::string> > textCommands;
    std::map<PdfReference, std::list<PdfArray> > bfCharCommands;
    std::map<PdfReference, std::list<PdfArray> > bfRangeCommands;
    std::map<PdfReference, UnicodeMap> unicodeMaps;
    UnicodeMap defaultMap;
    
    void commitTexts(bool force = false);
    bool processTextCommands(std::list<PdfReference> contentRefs, PdfDictionary* resources, bool force);
    void saveTextCommand(PdfObject* cmd);
    bool saveDictIfNeeded(PdfDictionary* dict);
    PdfDictionary* getDictionary(PdfObject* refOrDict);
    UnicodeMap* getUnicodeMap(std::string& fontName, PdfDictionary& page);
    bool createUnicodeMap(PdfReference& fontRef);
    void processToUnicodeCommands(PdfReference& ref, UnicodeMap& uMap);
    void processBfCharCommand(PdfArray& command, UnicodeMap& uMap);
    void processBfRangeCommand(PdfArray& command, UnicodeMap& uMap);    
    std::list<PdfReference> toRefList(PdfObject* contentRefs);    
    size_t getTextCommandsMemoryUsage();
    size_t getDictionariesMemoryUsage();
   
    Strigi::StreamStatus handleStream(Strigi::StreamBase<char>* s, PdfDictionary* dict);    
    Strigi::StreamStatus handleObjectStream(Strigi::StreamBase<char>* s, PdfDictionary* infoDict);
    Strigi::StreamStatus handleContentStream(Strigi::StreamBase<char>* s);
    Strigi::StreamStatus forwardStream(Strigi::StreamBase<char>* s);
};

#endif
