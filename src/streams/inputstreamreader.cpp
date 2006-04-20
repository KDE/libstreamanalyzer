#include "inputstreamreader.h"
#include <cerrno>
using namespace jstreams;

InputStreamReader::InputStreamReader(StreamBase<char>* i, const char* enc) {
    status = Ok;
    finishedDecoding = false;
    input = i;
    if (sizeof(wchar_t) > 1) {
        converter = iconv_open("WCHAR_T", enc);
    } else {
        converter = iconv_open("ASCII", enc);
    }
    // check if the converter is valid
    if (converter == (iconv_t) -1) {
        error = "conversion from '";
        error += enc;
        error += "' not available.";
        status = Error;
        return;
    }
    charbuf.setSize(262);
    buffer.setSize(262);
    charsLeft = 0;
}
InputStreamReader::~InputStreamReader() {
    if (converter != (iconv_t) -1) {
        iconv_close(converter);
    }
}
/*int32_t
InputStreamReader::read(const wchar_t*& start, int32_t ntoread) {
}
int32_t
InputStreamReader::read(const wchar_t*& start) {
    // if an error occured earlier, signal this
    if (status == Error) return -1;
    if (status == Eof) return 0;

    // if we cannot read and there's nothing in the buffer
    // (this can maybe be fixed by calling reset)
    if (finishedDecoding && buffer.avail == 0) return 0;

    // check if there is still data in the buffer
    if (buffer.avail == 0) {
        decodeFromStream();
        if (status == Error) return -1;
        if (status == Eof) return 0;
    }

    // set the pointers to the available data
    int32_t nread;
    nread = buffer.read(start, 0);
    return nread;
}*/
void
InputStreamReader::readFromStream() {
    // read data from the input stream
    inSize = input->read(inStart);
    if (inSize == 0) {
        status = Eof;
    } else if (inSize == -1) {
        status = Error;
    }
}
void
InputStreamReader::decode() {
    // decode from charbuf
    char *inbuf = charbuf.curPos;
    size_t inbytesleft = charbuf.avail;
    int32_t space = buffer.getWriteSpace();
//    printf("decode %p %i %i\n", buffer.curPos, space, buffer.size);
    size_t outbytesleft = sizeof(wchar_t)*space;
    char *outbuf = (char*)buffer.curPos;
    size_t r = iconv(converter, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    if (r == (size_t)-1) {
        switch (errno) {
        case EILSEQ: //invalid multibyte sequence
//            printf("invalid multibyte\n");
            error = "Invalid multibyte sequence.";
            status = Error;
            return;
        case EINVAL: // last character is incomplete
            // move from inbuf to the end to the start of
            // the buffer
//            printf("last char incomplete %i %i\n", left, inbytesleft);
            memmove(charbuf.start, inbuf, inbytesleft);
            charbuf.curPos = charbuf.start;
            charbuf.avail = inbytesleft;
            buffer.avail = ((wchar_t*)outbuf) - buffer.curPos;
            break;
        case E2BIG: // output buffer is full
//            printf("output full read %i %i\n", read, charbuf.avail );
            charbuf.curPos += charbuf.avail - inbytesleft;
            charbuf.avail = inbytesleft;
            buffer.avail = space;
            break;
        default:
            exit(-1);
        }
    } else { //input sequence was completely converted
//        printf("complete\n" );
        charbuf.curPos = charbuf.start;
        charbuf.avail = 0;
        buffer.avail = ((wchar_t*)outbuf) - buffer.curPos;
        if (input == 0) {
            finishedDecoding = true;
        }
    }
}
bool
InputStreamReader::fillBuffer() {
//    printf("decodefromstream\n");
    // fill up charbuf
    if (charbuf.curPos == charbuf.start) {
 //       printf("fill up charbuf\n");
        const char *begin;
        int32_t numRead;
        numRead = input->read(begin, charbuf.size);
        switch (numRead) {
        case 0:
            // signal end of input buffer
            input = 0;
            if (charbuf.avail) {
                error = "stream ends on incomplete character";
                status = Error;
            } else {
                status = Eof;
            }
            return false;
        case -1:
            error = input->getError();
            status = Error;
            return false;
        default:
            // copy data into other buffer
            memmove(charbuf.start + charbuf.avail, begin, numRead);
            charbuf.avail = numRead;
            break;
        }
    }
    // decode
    decode();
    return true;
}
StreamStatus
InputStreamReader::mark(int32_t readlimit) {
    buffer.mark(readlimit);
    return Ok;
}
StreamStatus
InputStreamReader::reset() {
    if (buffer.markPos) {
        buffer.reset();
        return Ok;
    } else {
        error = "No valid mark for reset.";
        return Error;
    }
}
FileReader::FileReader(const char* fname, const char* /*encoding_scheme*/,
        int32_t cachelen, int32_t /*cachebuff*/) {
    input = new FileInputStream(fname, cachelen);
    reader = new InputStreamReader(input);
}
FileReader::~FileReader() {
    if (reader) delete reader;
    if (input) delete input;
}
int32_t
FileReader::read(const wchar_t*& start) {
    int32_t nread = reader->read(start);
    if (nread == -1) {
        error = reader->getError();
        status = Error;
        return -1;
    } else if (nread == 0) {
        status = Eof;
    }
    return nread;
}
int32_t
FileReader::read(const wchar_t*& start, int32_t ntoread) {
    int32_t nread = reader->read(start, ntoread);
    if (nread == -1) {
        error = reader->getError();
        status = Error;
        return -1;
    } else if (nread != ntoread) {
        status = Eof;
    }
    return nread;
}
StreamStatus
FileReader::mark(int32_t readlimit) {
    status = reader->mark(readlimit);
    if (status != Ok) error = reader->getError();
    return status;
}
StreamStatus
FileReader::reset() {
    status = reader->reset();
    if (status != Ok) error = reader->getError();
    return status;
}
StringReader::StringReader(const wchar_t* value, const int32_t length )
        : len(length) {
    this->data = new wchar_t[len+1];
    bcopy(value, data, len*sizeof(wchar_t));
    this->pt = 0;
}
StringReader::StringReader( const wchar_t* value ) {
    this->len = wcslen(value);
    bcopy(value, data, len*sizeof(wchar_t));
    this->pt = 0;
}
StringReader::~StringReader(){
    close();
}
StreamStatus
StringReader::read(const wchar_t*& start, int32_t& nread, int32_t max) {
    if ( pt >= len )
        return Eof;
    nread = max;
    if (nread > len) nread = len;
    start = data + pt;
    pt += nread;
    return Ok;
}
StreamStatus
StringReader::read(wchar_t&c) {
    if (pt == len) {
        return Eof;
    }
    c = data[pt++];
    return Ok;
}
void
StringReader::close(){
    if (data) {
        delete data;
    }
}
StreamStatus
StringReader::mark(int32_t /*readlimit*/) {
    markpt = pt;
    return Ok;
}
StreamStatus
StringReader::reset() {
    pt = markpt;
    return Ok;
}

#include "inputstreambuffer.cpp"


