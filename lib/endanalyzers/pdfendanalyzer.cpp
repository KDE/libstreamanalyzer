/* This file is part of Strigi Desktop Search
 *
 * Copyright (C) 2007 Jos van den Oever <jos@vandenoever.info>
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

#include "pdfendanalyzer.h"
#include <strigi/strigiconfig.h>
#include <strigi/analysisresult.h>
#include <strigi/fieldtypes.h>
#include <strigi/textutils.h>
#include <sstream>
#include <iostream>
#include <cstring>

using namespace std;
using namespace Strigi;

const string titleFieldName("http://www.semanticdesktop.org/ontologies/2007/01/19/nie/#title");
const string subjectFieldName("http://www.semanticdesktop.org/ontologies/2007/01/19/nie/#subject");
const string creatorFieldName("http://www.semanticdesktop.org/ontologies/2007/03/22/nco/#creator");
const string keywordFieldName("http://www.semanticdesktop.org/ontologies/2007/01/19/nie/#keyword");
const string contentCreatedFieldName("http://www.semanticdesktop.org/ontologies/2007/01/19/nie/#contentCreated");
const string contentLastModifiedFieldName(
    "http://www.semanticdesktop.org/ontologies/2007/01/19/nie/#contentLastModified");

void
PdfEndAnalyzerFactory::registerFields(FieldRegister& reg) {
    typeField = reg.typeField;
    titleField = reg.registerField(titleFieldName);
    subjectField = reg.registerField(subjectFieldName);
    creatorField = reg.registerField(creatorFieldName);
    keywordField = reg.registerField(keywordFieldName);
    contentCreatedField = reg.registerField(contentCreatedFieldName);
    contentLastModifiedField = reg.registerField(contentLastModifiedFieldName);
    
    addField(typeField);
    addField(titleField);
    addField(subjectField);
    addField(creatorField);
    addField(contentCreatedField);
    addField(contentLastModifiedField);
}

PdfEndAnalyzer::PdfEndAnalyzer(const PdfEndAnalyzerFactory* f) :factory(f) {
    parser.setStreamHandler(this);
    parser.setTextHandler(this);
}
StreamStatus
PdfEndAnalyzer::handle(InputStream* s) {
    ostringstream str;
    str << n++;
    char r = analysisresult->indexChild(str.str(), analysisresult->mTime(), s);
    analysisresult->finishIndexChild();
    // how do we set the error message in this case?
    return (r) ?Error :Ok;
}
StreamStatus
PdfEndAnalyzer::handle(const std::string& s) {
    analysisresult->addText(s.c_str(), (uint32_t)s.length());
    return Ok;
}
bool
PdfEndAnalyzer::checkHeader(const char* header, int32_t headersize) const {
    return headersize > 7 && strncmp(header, "%PDF-1.", 7) == 0;
}
string
PdfEndAnalyzer::toXsdDate(const string& pdfDate) {
    if (pdfDate.empty()) return string();
    
    string::const_iterator pos = pdfDate.begin();
    string::const_iterator end = pdfDate.end();
    if (*pos == 'D') {
        pos += 2;        
    }   
    if (pos + 4 > end) return string();
    
    string year(pos, pos+4);
    string month("01");
    string day("01");
    string hour("00");
    string minute("00");
    string second("00");
    string zoneSign;
    string zoneHour("00");
    string zoneMinute("00");
        
    pos += 4;
    while(true) {
        if (pos+2 > end) break;
        month.assign(pos, pos+2);
        pos += 2;
        if (pos+2 > end) break;
        day.assign(pos, pos+2);
        pos += 2;
        if (pos+2 > end) break;
        hour.assign(pos, pos+2);
        pos += 2;
        if (pos+2 > end) break;
        minute.assign(pos, pos+2);
        pos += 2;
        if (pos+2 > end) break;
        second.assign(pos, pos+2);
        pos += 2;
        if (pos+1 > end) break;
        zoneSign.assign(pos, ++pos);
        if (pos+3 > end) break;
        zoneHour.assign(pos, pos+2);
        pos += 2;
        if (*(pos++) != '\'') break;        
        if (pos+3 > end) break;
        zoneMinute.assign(pos, pos+2);
        break;
    }
    string result(year+'-'+month+'-'+day+'T'+hour+':'+minute+':'+second);
    if (!zoneSign.empty()) {
        result += zoneSign;
        if (zoneSign != "Z") result += zoneHour+':'+zoneMinute;
    }
    return result;
}   
signed char
PdfEndAnalyzer::analyze(AnalysisResult& as, InputStream* in) {
    analysisresult = &as;
    n = 0;
    StreamStatus r = parser.parse(in);
    if (r != Eof) m_error.assign(parser.error());
    analysisresult->addValue(factory->typeField,
        "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#PaginatedTextDocument");
    
    // Get PDF metadata.
    // FIXME: the Author field is not saved because I don't know
    // which ontology should be used...
    map<string, string> metadata = parser.getMetadata();
    
    map<string, string>::const_iterator it;
    for (it = metadata.begin(); it != metadata.end(); it++) {
        const string& key = (*it).first;
        const string& val = (*it).second;
        RegisteredField* field = 0;
        if (key == "Title") {
            as.addValue(factory->titleField, val);
        } else if (key == "Subject") {
            as.addValue(factory->subjectField, val);
        } else if (key == "Creator") {
            // FIXME: The range of 
            //http://www.semanticdesktop.org/ontologies/2007/03/22/nco/#creatorField
            // is "Contact"... Should that be respected somehow?
            as.addValue(factory->creatorField, val);
        } else if (key == "CreationDate") {
            as.addValue(factory->contentCreatedField, toXsdDate(val));
        } else if (key == "ModDate") {
            as.addValue(factory->contentLastModifiedField, toXsdDate(val));
        } else if (key == "Keywords") {
            cout << val << endl;
            istringstream iss(val);
            iss >> ws;
            while (iss) {
                string keyword;
                iss >> keyword;
                as.addValue(factory->keywordField, keyword);
            }
        }
    }    
    return (r == Eof) ?(signed char)0 :(signed char)-1;
}
