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
#ifndef PINDEXER_H
#define PINDEXER_H

#include <string>
#include "streamanalyzer.h"
#include "xmlindexwriter.h"

class FileLister;

class PIndexer {
private:
    std::ostream& out;
    FileLister* m_lister;
    XmlIndexWriter writer;
    Strigi::StreamAnalyzer m_indexer;
    const TagMapping mapping;

    static void addFileCallback(const char* fullpath, unsigned dirlen,
        unsigned len, time_t mtime);
    static PIndexer *workingIndexer;
    void doFile(const char* filepath);
public:
    PIndexer(std::ostream& out, Strigi::AnalyzerConfiguration&,
        const char* mappingfile, int nthreads);
    ~PIndexer();
    void index(const char *dir);
};

#endif
