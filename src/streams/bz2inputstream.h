#ifndef BZ2INPUTSTREAM_H
#define BZ2INPUTSTREAM_H

#include "bufferedstream.h"

#include <bzlib.h>

namespace jstreams {

class BZ2InputStream : public BufferedInputStream<char> {
private:
    bool allocatedBz;
    bz_stream bzstream;
    StreamBase<char> *input;

    void dealloc();
    void readFromStream();
    bool checkMagic();
protected:
    int32_t fillBuffer(char* start, int32_t space);
public:
    BZ2InputStream(StreamBase<char>* input);
    ~BZ2InputStream();
    static bool checkHeader(const char* data, int32_t datasize);
};

} // end namespace jstreams

#endif
