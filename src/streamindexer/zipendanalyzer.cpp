#include "zipendanalyzer.h"
#include "zipinputstream.h"
#include "streamindexer.h"
#include "subinputstream.h"
using namespace jstreams;

bool
ZipEndAnalyzer::checkHeader(const char* header, int32_t headersize) const {
    return ZipInputStream::checkHeader(header, headersize);
}
char
ZipEndAnalyzer::analyze(std::string filename, InputStream *in,
        int depth, StreamIndexer *indexer, jstreams::Indexable*) {
    ZipInputStream zip(in);
    InputStream *s = zip.nextEntry();
    if (zip.getStatus()) {
        printf("error: %s\n", zip.getError());
//        exit(1);
    }
    while (s) {
        std::string file = filename+"/";
        file += zip.getEntryInfo().filename;
        indexer->analyze(file, zip.getEntryInfo().mtime, s, depth);
        s = zip.nextEntry();
    }
    if (zip.getStatus() == jstreams::Error) {
        error = zip.getError();
        return -1;
    } else {
        error.resize(0);
    }
    return 0;
}

