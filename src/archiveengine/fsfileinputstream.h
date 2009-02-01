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
#ifndef STRIGI_FSFILEINPUTSTREAM_H
#define STRIGI_FSFILEINPUTSTREAM_H

#include "bufferedstream.h"

class QFSFileEngine;
class QString;

namespace Strigi {

class FsFileInputStream : public Strigi::BufferedInputStream {
private:
    bool open;
    QFSFileEngine *fse;

    void readFromFile();
protected:
    int32_t fillBuffer(char* start, int32_t space);
public:
    static const int32_t defaultBufferSize;
    explicit FsFileInputStream(const QString &filename,
        int32_t buffersize=defaultBufferSize);
    explicit FsFileInputStream(QFSFileEngine *,
        int32_t buffersize=defaultBufferSize);
    ~FsFileInputStream();
    Strigi::StreamStatus reopen();
};

} // end namespace Strigi

#endif
