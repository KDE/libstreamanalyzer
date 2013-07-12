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
#include <strigi/strigiconfig.h>
#include "compat.h"
#include "indexpluginloader.h"
#include "indexmanager.h"
#include "indexmanagertests.h"
#include "indexwritertests.h"
#include "indexreadertests.h"
#include <strigi/analyzerconfiguration.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#ifdef _WIN32
#include <direct.h>
#endif

using namespace std;

StrigiMutex errorlock;
int founderrors = 0;
int
CLuceneTest(int argc, char*argv[]) {
    setenv("STRIGI_PLUGIN_PATH", BINARYDIR"/src/luceneindexer/", 1);
    setenv("XDG_DATA_HOME",
        SOURCEDIR"/src/streamanalyzer/fieldproperties", 1);
    setenv("XDG_DATA_DIRS",
        SOURCEDIR"/src/streamanalyzer/fieldproperties", 1);
    const char* path = ":memory:";

    // initialize a directory for writing and an indexmanager
    Strigi::IndexManager* manager
        = Strigi::IndexPluginLoader::createIndexManager("clucene", path);
    if (manager == 0) {
        cerr << "could not create indexmanager" << endl;
        founderrors++;
        return founderrors;
    }

    Strigi::AnalyzerConfiguration ic;
    IndexManagerTests tests(manager, ic);
    tests.testAll();
    tests.testAllInThreads(20);

    Strigi::IndexWriter* writer = manager->indexWriter();
    IndexWriterTests wtests(*writer, ic);
    wtests.testAll();

    Strigi::IndexReader* reader = manager->indexReader();
    IndexReaderTests rtests(reader);
    rtests.testAll();

    // close and clean up the manager
    Strigi::IndexPluginLoader::deleteIndexManager(manager);

    return founderrors;
}
