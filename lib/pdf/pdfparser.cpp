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
#include "pdfparser.h"
#include <strigi/gzipinputstream.h>
#include <typeinfo>
#include <iostream>

using namespace std;
using namespace Strigi;

PdfParser::PdfParser() : streamhandler(0), texthandler(0) {}

PdfParser::~PdfParser() {
    while (!dictionaries.empty()) {
        PdfDictionary* dict =  dictionaries.begin()->second;
        delete dictionaries.begin()->second;
        dictionaries.erase(dictionaries.begin());    
    }
    while (!pageDictionaries.empty()) {        
        delete pageDictionaries.front();
        pageDictionaries.pop_front();
    }
}

/**-------------------------------------------------------------------------
 * Parse the PDF stream.
 */
StreamStatus
PdfParser::parse(StreamBase<char>* s) {
    const int64_t TEXT_MEMORY_LIMIT = 2000000;
    bodyReader.setStream(s);    
    StreamStatus r;
    while ((r = bodyReader.parseNextObject()) != Error) { 
        parsed.insert(bodyReader.objectRef);
        //cout << "Object index "<<bodyReader.objectRef.index()<<endl;
        if (bodyReader.lastDictionary) {            
            PdfDictionary* dict = bodyReader.lastDictionary;
            if (saveDictIfNeeded(dict)) {
                bodyReader.lastDictionary = 0;  
            }
            if (bodyReader.nextStream) {
                if (handleStream(bodyReader.nextStream, dict) != Eof) {
                    cerr << "Error parsing stream: " <<bodyReader.nextStream->error()<<endl;                    
                    break;
                }   
            }
        }
        if (parsed.size() % 200 == 0) {
            if (getTextCommandsMemoryUsage() > TEXT_MEMORY_LIMIT) {
                commitTexts(true);
            } else commitTexts();
        }
        if (r == Eof) break;
    }
    if (r == Error) {
        cerr << "Error parsing Pdf file: "<<bodyReader.error()<<endl;        
    }
    //cout << "Parsing finished"<<endl;
    cout << "Memory used by dictionaries: " << getDictionariesMemoryUsage()<<endl;
    commitTexts(true);  
    return r;
}

/**-------------------------------------------------------------------------
 * Convert the metadata dictionary to map<string, string>. 
 */
map<string, string>
PdfParser::getMetadata() {
    map<string, string> metadata;
    if (!bodyReader.trailer) return metadata;
    PdfDictionary* infoDict = dynamic_cast<PdfDictionary*> (bodyReader.trailer->get("Info"));
    if (!infoDict) return metadata;
    PdfDictionary::iterator it;
    for (it = infoDict->begin(); it != infoDict->end(); it++) {
        if (typeid(*(*it).second) != typeid(PdfString)) continue;
        const PdfName& key = (*it).first;
        PdfString& val = *(PdfString*) (*it).second;
        metadata[key] = val;
    }
    return metadata;
}

/**-------------------------------------------------------------------------
 * Convert text strings to unicode and send them to texthandler. If force is
 * false, the method will stop if no UnicodeMap can be found for a fonts.
 * If force = true, defaultMap (only ascii chars) is used if necessary. 
 * Content streams are handled in the order they appear on a page, but the
 * order of pages is arbitrary.
 */
void
PdfParser::commitTexts(bool force) {
    if (!texthandler) return;
    //cout << "Commit texts, force = "<<force<<endl;
    while (!pageDictionaries.empty()) {
        PdfDictionary* page = pageDictionaries.front();
        PdfDictionary* resources = getDictionary(page->get("Resources"));
        if (!resources) {
            //cerr << "Resources dict not found"<<endl;
            if (!force) return;
        }
        PdfObject* contents = page->get("Contents");
        if (!contents) {
            //cerr << "Contents not found not found"<<endl;
            if(force) continue;
            else return;
        }
        if (!processTextCommands(toRefList(contents), resources, force)) {
            return;
        }
        // All text from the page processed
        delete page;
        pageDictionaries.pop_front();        
    }
}

/**-------------------------------------------------------------------------
 * Process the given text commands. Returns false if not all texts strings have
 * been sent to the text handler, true otherwise. If force is true, returns
 * true always. If force is true, resources can be 0.
 */

bool
PdfParser::processTextCommands(list<PdfReference> contentRefs, PdfDictionary* resources, bool force) {
    UnicodeMap* uMap = &defaultMap;
    string textToHandler;
    while(!contentRefs.empty()) {
        PdfReference& ref = contentRefs.front();
        if (parsed.count(ref) == 0) {
            //cerr << "Stream "<<ref.toString()<<" not yet parsed."<<endl;
            if (force) {
                contentRefs.pop_front();
                continue;
            }
            else return false;
        }
        list<string>& cmds = textCommands[ref];
        //cout << cmds.size()<<" commands from stream "<<ref.toString()<<endl;
        while (!cmds.empty()) {
            string& command = cmds.front();
            if (*command.rbegin() == 'f') {
                // This is a font name                   
                // Remove the 'f' marker from the end
                string fontName = command.substr(0, command.length()-1);
                if (resources) {
                    uMap = getUnicodeMap(fontName, *resources);
                    if (!uMap) {
                        // If force is false, try again later
                        if (force) uMap = &defaultMap;
                        else return false;
                    }
                }
            } else {
                // This is a text string
                // Remove the 's' marker from the end
                command.resize(command.length()-1);                    
                if (!textToHandler.empty()) {
                    size_t len = textToHandler.length();
                    // Remove hyphen from the end if present
                    if (textToHandler[len-1] == '-') {                            
                        textToHandler.resize(len-1);
                    } else {
                        texthandler->handle(textToHandler);
                        textToHandler.clear();
                    }
                }
                textToHandler += uMap->convert(command);
            }
            if (!textToHandler.empty()) {
                
            }
            // Command processed successfully
            cmds.pop_front();
        }
        // All commands from the content stream have been processed
        textCommands[ref].clear();
        contentRefs.pop_front();
    }
    // If a page ends with a hyphenated word this check is needed
    if (!textToHandler.empty()) {
        texthandler->handle(textToHandler);            
    }
    return true;
}

/**-------------------------------------------------------------------------
 * Save the dictionary to dictionaries if it can potentially be one of
 * the following:
 * - Page dictionary
 * - the dictionary corresponding to the "Font" key of a page dict
 * - Resources dictionary
 * - Font dictionary
 * - Encoding dictionary
 * - Metadata dictionary (corresponding to the "Info" key of the trailer
 */
bool 
PdfParser::saveDictIfNeeded(PdfDictionary* dict) {    
    PdfName* type = dynamic_cast<PdfName*> (dict->get("Type"));    
    if (type && *type == "Page") {
        // Save pages separately
        pageDictionaries.push_back(dict);        
        return true;
    }
    // Filter out some uninteresting dictionaries
    if (type && !(*type == "Font" || *type == "Encoding")) {        
        return false;
    }
    if (!type) {
        PdfDictionary::iterator it;
        for (it = dict->begin(); it != dict->end(); it++) {
            const PdfName& key = (*it).first;
            // Filter out content stream, name and number tree dicts.
            // P, D and Height entries appear in various kinds of dicts that we don't want
            if (key == "Length" || key == "Limits" || key == "P" || key == "D" || key == "Height") {
                return false;
            }
        }
    }
    dictionaries[bodyReader.objectRef] = dict;
    //cout << "Save dict: "<<dict->toString()<<endl;
    return true;
}

/**-------------------------------------------------------------------------
 * This method returns, depending on the type of argument:
 * - if refOrDict is a PdfDictionary, return it as such
 * - if refOrDict is a PdfReference, look for a parsed
 *   dictionary corresponding to the reference. Return the
 *   dictionary or 0 if not found
 * - else return 0
 */
PdfDictionary*
PdfParser::getDictionary(PdfObject* refOrDict) {
    if (!refOrDict) return 0;
    if (typeid(*refOrDict) == typeid(PdfDictionary)) {
        return (PdfDictionary*) refOrDict;
    }
    PdfReference* ref = dynamic_cast<PdfReference*> (refOrDict);
    if (!ref || dictionaries.count(*ref) == 0) return 0;
    return dictionaries[*ref];
}

/**-------------------------------------------------------------------------
 * Use the resources dict to determine the object index of the font dict
 * corresponding to FontName. If a UnicodeMap for the font doesn't exist,
 * call createUnicodeMap. Return 0 if all the needed data has not yet been
 * parsed.
 */
UnicodeMap*
PdfParser::getUnicodeMap(string& fontName, PdfDictionary& resources) {    
    // Now look for a "Font" key, the value of which is a dictionary that
    // associates font names to font objects
    PdfDictionary* fontNameDict = getDictionary(resources.get("Font"));
    if (!fontNameDict) {
        //cerr << "Font name dictionary cannot be found!"<<endl;
        return 0;
    }
    PdfName key;
    key.assign(fontName);
    PdfReference* fontRef = dynamic_cast<PdfReference*> (fontNameDict->get(key));
    if (!fontRef) {        
        cerr << "No font for font name "<<fontName<<endl;;
        return 0;
    }    
    if (unicodeMaps.count(*fontRef) == 0 && !createUnicodeMap(*fontRef)) {        
        return 0;
    }
    return &unicodeMaps[*fontRef];
}

/**-------------------------------------------------------------------------
 * Creates a unicode map for a font with a given object index and saves it to
 * unicodeMaps. Returns true if a map was created, false otherwise.
 */
bool
PdfParser::createUnicodeMap(PdfReference& fontRef) { 
    PdfDictionary* font = getDictionary(&fontRef);
    if (!font) {
        //fprintf(stderr, "Font %s not found\n", fontRef.toString().c_str());
        return false;
    }

    PdfName key;
    PdfReference* ref;
    
    // See if the font has a ToUnicode CMap associated to it
    if (font->count(key.assign("ToUnicode")) != 0) {        
        ref = dynamic_cast<PdfReference*> ((*font)[key]);
        if (ref) {
            UnicodeMap& uMap = unicodeMaps[fontRef];
            processToUnicodeCommands(*ref, uMap);
            return true;
        }
    }
    // Otherwise look for an encoding    
    PdfObject* encodingValue = font->get("Encoding");
    if (!encodingValue) {
        //cerr << "No encoding entry in font "<<fontRef.toString()<<endl;
        unicodeMaps[fontRef];
        return true;
    }
    if (typeid(*encodingValue) == typeid(PdfName)) {  
        // one of the standard encondings        
        //printf("\nEncoding: %s\n", encodingValue->toString().c_str());
        UnicodeMap& uMap = unicodeMaps[fontRef];
        uMap.setBaseEncoding(*(PdfName*) encodingValue);
        return true;
    } 
    // An encoding dictionary. Look for BaseEncoding and Differences keys
    PdfDictionary* encodingDict = getDictionary(encodingValue);
    if (!encodingDict) {
        //cerr << "Encoding dict not found"<<endl;
        return false;
    }
    UnicodeMap& uMap = unicodeMaps[fontRef];
    if (encodingDict->count(key.assign("BaseEncoding")) == 0) {        
        uMap.setBaseEncoding(UnicodeMap::Standard);
    } else {
        //cout << "Base encoding: "<<(*(PdfName*) (*encodingDict)[key]))<<endl;
        uMap.setBaseEncoding(*(PdfName*) (*encodingDict)[key]);   
    }
    if (encodingDict->count(key.assign("Differences"))) {
        //cout <<"Differences found"<<endl;
        PdfArray* differences = dynamic_cast<PdfArray*> ((*encodingDict)[key]);
        if (differences) {
            uMap.addDifferences((PdfArray*) differences);
        } 
    }
        
    return true;  
}

/**-------------------------------------------------------------------------
 * Go through the bfChar and bfRange commands in the ToUnicode stream
 * corresponding to the given object index. The ranges are saved to the
 * uMap
 */
void
PdfParser::processToUnicodeCommands(PdfReference& ref, UnicodeMap& uMap) {
    list<PdfArray>& charCommands = bfCharCommands[ref];
    list<PdfArray>& rangeCommands = bfRangeCommands[ref];
    list<PdfArray>::iterator it;
    for(it = charCommands.begin(); it != charCommands.end(); it++) {
        processBfCharCommand(*it, uMap);
    }
    for(it = rangeCommands.begin(); it != rangeCommands.end(); it++) {
        processBfRangeCommand(*it, uMap);
    }
}
void
PdfParser::processBfCharCommand(PdfArray& command, UnicodeMap& uMap) {    
    if (command.size() % 2 != 0) {
        cerr << "Wrong number of charcodes in bfchar list!"<<endl;
        return;
    }
    for (PdfArray::iterator it = command.begin(); it != command.end(); it++) {
        PdfString* srcChar = dynamic_cast<PdfString*> (*(it++));            
        PdfString* dstChar = dynamic_cast<PdfString*> (*it);
        if (!srcChar || !dstChar) return;
        uMap.addChar(*srcChar, *dstChar);            
    }
} 
void
PdfParser::processBfRangeCommand(PdfArray& command, UnicodeMap& uMap) { 
    if (command.size() % 3 != 0) {
        cerr << "Wrong number of arguments for bfrange!"<<endl;
        return;
    }
    for (PdfArray::iterator it = command.begin(); it != command.end(); it++) {
        PdfString* srcStart = dynamic_cast<PdfString*> (*(it++));
        PdfString* srcEnd = dynamic_cast<PdfString*> (*(it++));
        if (!srcStart || !srcEnd) return;
        if (typeid(**it) == typeid(PdfString)) {
            PdfString* dstStart = (PdfString*) *it;
            uMap.addRange(*srcStart, *srcEnd, *dstStart);
        } else {
            PdfArray* dstCodes = dynamic_cast<PdfArray*> (*it);
            if (!dstCodes) return;
            uMap.addRange(*srcStart, dstCodes);
        }
    }
}

/**-------------------------------------------------------------------------
 * Contents value in a Page dictionary can be either a reference or
 * a an array of references. This method returns a list of references.
 */
list<PdfReference> 
PdfParser::toRefList(PdfObject* contentRefs) {    
    list<PdfReference> list;
    if (typeid(*contentRefs) == typeid(PdfReference)) {
        list.push_back(*(PdfReference*) contentRefs);
    } else {
        PdfArray* refArr = dynamic_cast<PdfArray*> (contentRefs);
        if (!refArr) return list;        
        for (PdfArray::iterator it = refArr->begin(); it != refArr->end(); it++) {
            PdfReference* ref = dynamic_cast<PdfReference*> (*it);            
            if (ref) list.push_back(*ref);
        }
    }     
    return list;
}

/**-------------------------------------------------------------------------
 * Save a text command. If cmd is of type
 * - PdfString, this is a Tj command (text string)
 * - PdfArray, this is a TJ command (array of text strings)
 * - PdfName, this is a Tf command (change font)
 * Strings and font names are saved as strings to 
 * textCommands[bodyReader.objectIndex], but text strings are denoted with a 
 * trailing 's' char and fonts with a trailing 'f' char. This is because there
 * can be lots of text commands and saving pointers would cause some memory overhead.
 */
void PdfParser::saveTextCommand(PdfObject* cmd) {
    //printf("Save text command\n");
    if (typeid(*cmd) == typeid(PdfName)) {   
        textCommands[bodyReader.objectRef].push_back(*(PdfName*) cmd + 'f');
        return;
    }   
    string str;
    if (typeid(*cmd) == typeid(PdfString)) {
        str += *(PdfString*) cmd;
    } else if (typeid(*cmd) == typeid(PdfArray)) {
        PdfArray* arr = (PdfArray*) cmd;     
        
        for (PdfArray::iterator it = arr->begin(); it != arr->end(); it++) {
            PdfObject* obj = *it;
            if (typeid(*obj) == typeid(PdfString)) {
                str += *(PdfString*) obj;
            } else {
                // Numbers with a large absolute value in a TJ
                // array usually correspond to whitespace                
                const int32_t MIN_SPACE_VALUE = 150;
                PdfNumber* num = dynamic_cast<PdfNumber*> (obj);
                if (!num) return;
                int32_t val = num->integerPart();
                if (val < -MIN_SPACE_VALUE || val > MIN_SPACE_VALUE) {                    
                    str += UnicodeMap::WHITESPACE_CHAR;
                }
            }
        }        
    }   
    str += 's';
    textCommands[bodyReader.objectRef].push_back(str);    
}

 
/**-------------------------------------------------------------------------
 * Handle a content stream. We are interested in the following commands:
 * - Tj, ', " (print a text string)
 * - TJ (print an array of text strings)
 * - Tf (select font)
 * - EndBfChar (in the case of a ToUnicode stream, a sequence of source and
 *   destination charcodes)
 * - endBfRange (a sequence of src and dst ranges)
 * 
 * Content streams can be split to several streams and the split often occurs even
 * between the arguments of an operator and the operator itself. If we find 
 * a set of arguments without an operator at the end of the stream, we send it to
 * saveTextCommand just in case.
 */
StreamStatus
PdfParser::handleContentStream(StreamBase<char>* s) {
    //cout << "Handle content stream, ref "<<bodyReader.objectRef.toString()<<endl;
    ContentStreamReader reader(s);
    StreamStatus r;
    PdfReference ref = bodyReader.objectRef;
    PdfOperator& op = reader.lastOperator;
    while ((r = reader.parseNextCommand()) != Error) {   
        if (!reader.lastArguments.empty() &&
            (op == "Tj" || op == "'" || op == "\"" || op == "TJ" || op == "Tf"
             || (r == Eof && op.empty())) ) {            
            saveTextCommand(reader.lastArguments.front());                    
        } else if (op == "endbfchar") {            
            bfCharCommands[ref].push_back(reader.lastArguments);
            reader.lastArguments.clear();
        } else if (op == "endbfrange") {                
            bfRangeCommands[ref].push_back(reader.lastArguments);
            reader.lastArguments.clear();
        } 
        if (r == Eof) break;
    }
    return r;
}
/**-------------------------------------------------------------------------
 * Handle a object stream. In an object stream, the object indices are first and
 * the come the objects in a sequence. Save dictionaries if they are interesting.
 */
StreamStatus
PdfParser::handleObjectStream(StreamBase<char>* s, PdfDictionary* infoDict) {
    //printf("Handle object stream, ref %s\n", bodyReader.objectRef.toString().c_str());
    s->reset(0);
    ObjectStreamReader reader(s, infoDict);
    StreamStatus r;
    
    while ((r = reader.parseNextObject()) != Error) {
        if (reader.lastDictionary) {
            PdfDictionary* dict = reader.lastDictionary;
            if (saveDictIfNeeded(dict)) {
                reader.lastDictionary = 0;
            }
        }
        if (r == Eof) break;
    } 
    return r;
}

/**-------------------------------------------------------------------------
 * Handle a stream. Pass on the stream the handleContentStream or
 * handleObjectStream if needed.
 */
StreamStatus
PdfParser::handleStream(StreamBase<char>* s, PdfDictionary* dict) {       
    PdfName key;
    StreamBase<char>* stm = s;
    if (dict->count(key.assign("Filter")) != 0) {
        PdfName* filter = dynamic_cast<PdfName*> ((*dict)[key]);
        if (filter && *filter == "FlateDecode") {
            stm = new GZipInputStream(s, GZipInputStream::ZLIBFORMAT);
        } else {
            // We cannot handle this stream
            //cerr << "Discarding stream "<< dict->toString() << endl;;
            if (streamhandler) streamhandler->handle(s);
            return forwardStream(s);
        }             
    }   
    stm->reset(0);
    PdfName* type = dynamic_cast<PdfName*> (dict->get("Type"));
    if (!type && !dict->hasKey("Subtype")) {
        //cout << "Handle content stream with dict: "<<dict->toString()<<endl;
        handleContentStream(stm);    
    }
    else if (type && *type == "ObjStm") handleObjectStream(stm, dict);
            
    stm->reset(0);
    if (streamhandler) streamhandler->handle(stm);    
    if (stm != s) delete stm;
    return forwardStream(s);
    
}

/**-------------------------------------------------------------------------
 * Skip until EOF.
 */
StreamStatus
PdfParser::forwardStream(StreamBase<char>* s) {
    StreamStatus r = Ok;
    while (r == Ok) {
        s->skip(1000);
        r = s->status();
    }
    return r;
}

/**-------------------------------------------------------------------------
 * Calculate the memory taken by saved text strings and dictionaries
 */

size_t
PdfParser::getTextCommandsMemoryUsage() {
    size_t size = textCommands.size()*sizeof(pair<PdfReference, list<string> >);
    map<PdfReference, list<string> >::iterator it;
    for(it = textCommands.begin(); it != textCommands.end(); it++) {
        size += (*it).second.size()*sizeof(string);
        list<string>::iterator it2;
        for(it2 = (*it).second.begin(); it2 != (*it).second.end(); it2++) {
            size += (*it2).length();
        }
    }
    return size;
}
size_t
PdfParser::getDictionariesMemoryUsage() {
    size_t size = dictionaries.size()*sizeof(pair<PdfReference, PdfDictionary*>);
    map<PdfReference, PdfDictionary*>::iterator it;
    for (it = dictionaries.begin(); it != dictionaries.end(); it++) {
        size += (*it).second->totalSize();
    }
    return size;
}

/**-------------------------------------------------------------------------
 * Default handlers
 */
Strigi::StreamStatus
PdfParser::DefaultStreamHandler::handle(Strigi::StreamBase<char>* s) {
    static int count = 0;
    char name[32];
    const char *c;
    int32_t n = s->read(c, 1, 0);
    if (n <= 0) {
        return s->status();
    }
    sprintf(name, "out/%i", ++count);
    FILE* file = fopen(name, "wb");
    if (file == 0) {
        return Error;
    }
    do {
        fwrite(c, 1, n, file);
        n = s->read(c, 1, 0);
    } while (n > 0);
    fclose(file);
    return s->status();
}
Strigi::StreamStatus
PdfParser::DefaultTextHandler::handle(const string& s) {
    printf("%s\n", s.c_str());
    return Ok;
}

