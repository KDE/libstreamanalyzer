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
#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include "strigiconfig.h"
#include "fileinputstream.h"
#include "bz2inputstream.h"
#include "indexer.h"
#include "analyzerconfiguration.h"
#include "streamendanalyzer.h"
#include "streamthroughanalyzer.h"
#include "streamlineanalyzer.h"
#include "streamsaxanalyzer.h"
#include "streameventanalyzer.h"

#include <cstdio>
#include <cstring>
#ifdef HAVE_UNISTD_H
 #include <unistd.h>
#endif
#ifdef HAVE_DIRECT_H
 #include <direct.h>
#endif

#include <sstream>
#include <fstream>
using namespace Strigi;
using namespace std;

class SingleAnalyzerConfiguration : public Strigi::AnalyzerConfiguration {
private:
    const char* const analyzerName;
    mutable int count;
public:
    mutable vector<string> names;
    explicit SingleAnalyzerConfiguration(const char* an)
        : analyzerName(an), count(0) {}

    bool valid() const { return count == 1; }
    bool useFactory(StreamEndAnalyzerFactory* f) const {
        bool use = strcmp(f->name(), analyzerName) == 0;
        count += (use) ?1 :0;
        names.push_back(f->name());
        return use;
    }
    bool useFactory(StreamThroughAnalyzerFactory* f) const {
        bool use = strcmp(f->name(), analyzerName) == 0;
        count += (use) ?1 :0;
        names.push_back(f->name());
        return use;
    }
    bool useFactory(StreamSaxAnalyzerFactory* f) const {
        bool use = strcmp(f->name(), analyzerName) == 0;
        count += (use) ?1 :0;
        names.push_back(f->name());
        return use;
    }
    bool useFactory(StreamEventAnalyzerFactory* f) const {
        bool use = strcmp(f->name(), analyzerName) == 0;
        count += (use) ?1 :0;
        names.push_back(f->name());
        return use;
    }
    bool useFactory(StreamLineAnalyzerFactory* f) const {
        bool use = strcmp(f->name(), analyzerName) == 0;
        count += (use) ?1 :0;
        names.push_back(f->name());
        return use;
    }
};

void
printUsage(char** argv) {
    fprintf(stderr, "Usage: %s analyzer file-to-analyze referenceoutputfile\n",
        argv[0]);
}
bool
containsHelp(int argc, char **argv) {
    for (int i=1; i<argc; ++i) {
         if (strcmp(argv[i], "--help") == 0
             || strcmp(argv[i], "-h") == 0) return true;
    }
    return false;
}
int
main(int argc, char** argv) {
    if (containsHelp(argc, argv) || argc < 3 || argc > 4) {
        printUsage(argv);
        return -1;
    }

    const char* analyzerName = argv[1];
    const char* targetFile = argv[2];
    const char* referenceFile = 0;
    if (argc == 4) {
        referenceFile = argv[3];
    }
    const char* mappingFile = 0;

    // check that the target file exists
    {
        ifstream filetest(targetFile);
        if (!filetest.good()) {
            cerr << "The file '" << targetFile << "' cannot be read." << endl;
            return 1;
        }
    }

    ostringstream s;
    SingleAnalyzerConfiguration ic(analyzerName);
    Indexer indexer(s, ic, mappingFile);
    if (!ic.valid()) {
        fprintf(stderr, "No analyzer with name %s was found.\n", analyzerName);
        fprintf(stderr, "Choose one from:\n");
        for (uint i=0; i<ic.names.size(); i++) {
            cerr << " " << ic.names[i] << endl;
        }
        return 1;
    }
    chdir(argv[1]);
    indexer.index(targetFile);
    string str = s.str();
    int32_t n = 2*str.length();

    // if no reference file was specified, we output the analysis
    if (referenceFile == 0) {
        cout << str;
        return 0;
    }

    // load the file to compare with
    FileInputStream f(referenceFile);
    BZ2InputStream bz2(&f);
    const char* c;
    n = bz2.read(c, n, n);
    if (n < 0) {
        fprintf(stderr, "Error: %s\n", bz2.error());
        return -1;
    }
    if (n != (int32_t)s.str().length()) {
        printf("output length differs %i instead of %i\n", n, s.str().length());
    }

    const char* p1 = c;
    const char* p2 = str.c_str();
    int32_t n1 = n;
    int32_t n2 = str.length();
    while (n1-- && n2-- && *p1 == *p2) {
        p1++;
        p2++;
    }
    if (n1 ==0 && (*p1 || *p2)) {
         printf("difference at position %i\n", p1-c);

         int32_t m = (80 > str.length())?str.length():80;
         printf("%i %.*s\n", m, m, str.c_str());

         m = (80 > n)?n:80;
         printf("%i %.*s\n", m, m, c);

         return -1;
    }

    return 0;
}
