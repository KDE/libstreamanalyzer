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

#include "SHA1.h"
#include <strigi/streameventanalyzer.h>
#include <strigi/analyzerplugin.h>
#include <strigi/analysisresult.h>
#include <strigi/fieldtypes.h>
#include <list>
using namespace std;
using namespace Strigi;

class DigestEventAnalyzerFactory;
class DigestEventAnalyzer : public Strigi::StreamEventAnalyzer {
private:
    CSHA1 sha1;
    string hash;
    Strigi::AnalysisResult* analysisresult;
    const DigestEventAnalyzerFactory* const factory;
public:
    DigestEventAnalyzer(const DigestEventAnalyzerFactory*);
    ~DigestEventAnalyzer();
    const char* name() const { return "DigestEventAnalyzer"; }
    void startAnalysis(Strigi::AnalysisResult*);
    void endAnalysis(bool complete);
    void handleData(const char* data, uint32_t length);
    bool isReadyWithStream();
};

class DigestEventAnalyzerFactory
        : public Strigi::StreamEventAnalyzerFactory {
public:
    const Strigi::RegisteredField* shafield;
private:
    const char* name() const {
        return "DigestEventAnalyzer";
    }
    void registerFields(Strigi::FieldRegister&);
    Strigi::StreamEventAnalyzer* newInstance() const {
        return new DigestEventAnalyzer(this);
    }
};

DigestEventAnalyzer::DigestEventAnalyzer(const DigestEventAnalyzerFactory* f)
        :factory(f) {
    analysisresult = 0;
    hash.resize(40);
}
DigestEventAnalyzer::~DigestEventAnalyzer() {
}
void
DigestEventAnalyzer::startAnalysis(AnalysisResult* ar) {
    analysisresult = ar;
    sha1.Reset();
}
void
DigestEventAnalyzer::handleData(const char* data, uint32_t length) {
    sha1.Update((unsigned char*)data, length);
}
namespace {
    const string type("http://www.w3.org/1999/02/22-rdf-syntax-ns#type");
    const string nfoFileHash(
        "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#FileHash");
    const string nfohashAlgorithm(
        "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#hashAlgorithm");
    const string SHA1("SHA1");
    const string hashValue(
        "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#hashValue");
}
void
DigestEventAnalyzer::endAnalysis(bool complete) {
    if (!complete) {
        return;
    }
    unsigned char digest[20];
    char d[41];
    sha1.Final();
    sha1.GetHash(digest);
    for (int i = 0; i < 20; ++i) {
        sprintf(d + 2 * i, "%02x", digest[i]);
    }
    hash.assign(d);
    const string hashUri = analysisresult->newAnonymousUri();
    analysisresult->addValue(factory->shafield, hashUri);
    analysisresult->addTriplet(hashUri, type, nfoFileHash);
    analysisresult->addTriplet(hashUri, nfohashAlgorithm, SHA1);
    analysisresult->addTriplet(hashUri, hashValue, hash);
    analysisresult = 0;
}
bool
DigestEventAnalyzer::isReadyWithStream() {
    return false;
}
void
DigestEventAnalyzerFactory::registerFields(FieldRegister& reg) {
    shafield = reg.registerField(
        "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#hasHash");
    addField(shafield);
}
// Analyzer

//Factory
class Factory : public AnalyzerFactoryFactory {
public:
    list<StreamEventAnalyzerFactory*>
    streamEventAnalyzerFactories() const {
        list<StreamEventAnalyzerFactory*> af;
        af.push_back(new DigestEventAnalyzerFactory());
        return af;
    }
};

STRIGI_ANALYZER_FACTORY(Factory)

#include "SHA1.cpp"
