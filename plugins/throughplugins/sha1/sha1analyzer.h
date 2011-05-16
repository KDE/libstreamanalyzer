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

#ifndef STRIGI_SHA1ANALYZER_H
#define STRIGI_SHA1ANALYZER_H

#include "sha1.h"

#include <strigi/streamthroughanalyzer.h>
#include <strigi/analyzerplugin.h>

namespace Strigi {
    class RegisteredField;
    class FieldRegister;
}

class Sha1AnalyzerFactory;

class Sha1Analyzer : public Strigi::StreamThroughAnalyzer {
private:
    SHA1 sha1;
    Strigi::AnalysisResult* analysisresult;
    const Sha1AnalyzerFactory* const factory;
    
    void handleData(const char* data, uint32_t length);
public:
    Sha1Analyzer(const Sha1AnalyzerFactory*);
    ~Sha1Analyzer();
    
    void setIndexable(Strigi::AnalysisResult* );
    const char* name() const { return "Sha1Analyzer"; }
    
    Strigi::InputStream* connectInputStream(Strigi::InputStream* in);    
    bool isReadyWithStream();
};

class Sha1AnalyzerFactory
        : public Strigi::StreamThroughAnalyzerFactory {
public:
    const Strigi::RegisteredField* shafield;
private:
    const char* name() const {
        return "Sha1Analyzer";
    }
    
    void registerFields(Strigi::FieldRegister&);
    
    Strigi::StreamThroughAnalyzer* newInstance() const {
        return new Sha1Analyzer(this);
    }
};

#endif
