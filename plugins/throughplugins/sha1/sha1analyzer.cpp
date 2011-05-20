/* This file is part of Strigi Desktop Search
 *
 * Copyright (C) 2011 Vishesh Handa <handa.vish@gmail.com>
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
#include "sha1analyzer.h"
#include <strigi/streambase.h>
#include <strigi/analysisresult.h>
#include <strigi/fieldtypes.h>

#include <iostream>

using namespace std;
using namespace Strigi;

Sha1Analyzer::Sha1Analyzer(const Sha1AnalyzerFactory* f)
        :factory(f) {
    analysisresult = 0;
}

Sha1Analyzer::~Sha1Analyzer() {
}

InputStream* Sha1Analyzer::connectInputStream(InputStream* in)
{
    if( !in )
        return in;

    //cout << "Starting.. " << endl;
    
    const int64_t pos = in->position();
    in->reset( pos );
    
    const char * data = 0;
    
    //Keep feeding the data in 512 byte increments
    while( true ) {
        int length = in->read( data, 512, 512 );
        
        if( length == -1 || length == 0 ) {
            in->reset( pos );
            break;
        }
        else if( length == -2 ) {
            in->reset( pos );
            return in;
        }
        
        sha1.Input( data, length );
        data = 0;
    }
      
    char d[41];
    unsigned message_digest[5];
    if (!sha1.Result(message_digest))
    {
        cerr << "sha: could not compute message digest\n";
        return in;
    }
    else
    {
        sprintf( d, "%08x%08x%08x%08x%08x",
                message_digest[0],
                message_digest[1],
                message_digest[2],
                message_digest[3],
                message_digest[4] );
    }
    
    string hashUri = analysisresult->newAnonymousUri();
    analysisresult->addValue(factory->shafield, hashUri);
    analysisresult->addTriplet(hashUri,
                               "http://www.w3.org/1999/02/22-rdf-syntax-ns#type",
                               "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#FileHash");
    analysisresult->addTriplet(hashUri,
                               "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#hashAlgorithm",
                               "SHA1");
    analysisresult->addTriplet(hashUri,
                               "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#hashValue",
                               string(d,40));
    analysisresult = 0;
    
    return in;
}

bool Sha1Analyzer::isReadyWithStream() {
    return true;
}

void Sha1AnalyzerFactory::registerFields(Strigi::FieldRegister& reg) {
    shafield = reg.registerField(
        "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#hasHash");
    addField(shafield);
}

void Sha1Analyzer::setIndexable(AnalysisResult* ar)
{
    analysisresult = ar;
    sha1.Reset();
}

//Factory
class Factory : public AnalyzerFactoryFactory {
public:
    list<StreamThroughAnalyzerFactory*>
    streamThroughAnalyzerFactories() const {
        list<StreamThroughAnalyzerFactory*> af;
        af.push_back(new Sha1AnalyzerFactory());
        return af;
    }
};

STRIGI_ANALYZER_FACTORY(Factory)