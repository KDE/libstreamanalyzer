#include "fsfileinputstream.h"
#include <Qt/QFSFileEngine>

const size_t FSFileInputStream::defaultBufferSize = 64;
FSFileInputStream::FSFileInputStream(const QString &filename, size_t buffersize) {
    // initialize values that signal state
    status = Ok;
    fse = new QFSFileEngine(filename);

    // try to open the file for reading
    open = fse->open(QIODevice::ReadOnly);
    if (!open) {
        // handle error
        error = (const char*)fse->errorString().toUtf8();
	printf("error: %s %s\n",  (const char*)filename.toUtf8(), error.c_str());
        status = Error;
        return;
    }

    // allocate memory in the buffer
    buffer.setSize(buffersize);
}
FSFileInputStream::FSFileInputStream(QFSFileEngine *fse, size_t buffersize) {
    status = Ok;
    open = true; // fse must be have been opened
    this->fse = fse;
    // allocate memory in the buffer
    buffer.setSize(buffersize);
}
FSFileInputStream::~FSFileInputStream() {
    delete fse;
}
InputStream::Status
FSFileInputStream::reopen() {
    if (open) {
        if (!fse->seek(0)) {
            error = (const char*)fse->errorString().toUtf8();
            status = Error;
        }
    } else if (fse->open(QIODevice::ReadOnly)) {
        open = true;
    } else {
        error = (const char*)fse->errorString().toUtf8();
        status = Error;
    }
    buffer.markPos = buffer.curPos = 0;
    buffer.avail = 0;
    return status;
}
InputStream::Status
FSFileInputStream::read(const char*& start, size_t& nread, size_t max) {
    // if an error occured earlier, signal this
    if (status) return status;

    // if we cannot read and there's nothing in the buffer
    // (this can maybe be fixed by calling reset)
    if (!open && buffer.avail == 0) return Eof;

    // if buffer is empty, read from buffer
    if (buffer.avail == 0) {
        readFromFile();
        if (status) return status;
    }
    // set the pointers to the available data
    buffer.read(start, nread, max);
    return Ok;
}
void
FSFileInputStream::readFromFile() {
    // prepare the buffer for writing
    size_t bytesRead = buffer.getWriteSpace();
    // read into the buffer
    bytesRead = fse->read(buffer.curPos, bytesRead);
    buffer.avail = bytesRead;
    // check the file stream status
    if (bytesRead == (size_t)-1) {
        error = (const char*)fse->errorString().toUtf8();
        fse->close();
        open = false;
        status = Error;
    } else if (bytesRead == 0) {
        fse->close();
        open = false;
    }
}
InputStream::Status
FSFileInputStream::mark(size_t readlimit) {
    buffer.mark(readlimit);
    return Ok;
}
InputStream::Status
FSFileInputStream::reset() {
    buffer.reset();
    return Ok;
}

