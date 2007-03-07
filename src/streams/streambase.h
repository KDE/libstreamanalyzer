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
#ifndef STREAMBASE_H
#define STREAMBASE_H

#include <stdio.h>
#include <string>
#include "streams_export.h"

#define INT32MAX 0x7FFFFFFFL

// todo: moveme to config check
#if _WIN32
typedef __int64 int64_t;
typedef int int32_t;
typedef unsigned int uint;
#endif

namespace jstreams {

enum StreamStatus { Ok, Eof, Error };

// java mapping: long=int64, int=int32, byte=uint8_t
/**
 * developer comment: This is needed because win32 compilation.
 * When we want to access a function outside a lib, we have to export them
 * but we can't export the template class because this would be somewhat
 * stupid / does not work by design :)
 * Because of this I've introduced this StreamBaseBase class
 */
class STREAMS_EXPORT StreamBaseBase {
protected:
    int64_t size;
    int64_t position;
    std::string error;
    StreamStatus status;
public:
    StreamBaseBase() :size(-1), position(0), status(Ok){ }
    virtual ~StreamBaseBase() {}
    /**
     * @brief  Return a string representation of the last error.
     * If no error has occurred, an empty string is returned.
     **/
    const char* getError() const { return error.c_str(); }
    /**
     * @brief  Return the status of the stream.
     **/
    StreamStatus getStatus() const { return status; }
    /**
     * @brief Get the current position in the stream.
     * The value obtained from this function can be used to reset the stream.
     **/
    int64_t getPosition() const { return position; }
    /**
     * @brief Return the size of the stream.
     * If the size of the stream is unknown, -1
     * is returned. If the end of the stream has been reached the size is
     * always known.
     **/
    int64_t getSize() const { return size; }
};

/**
 * @short Base class for stream read access to many different file types.
 *
 * This class is based on the interface java.io.InputStream. It allows
 * for uniform access to streamed resources.
 * The main difference with the java equivalent is a performance improvement.
 * When reading data, data is not copied into a buffer provided by the caller,
 * but a pointer to the read data is provided. This makes this interface
 * especially useful for deriving from it and implementing filterers or
 * transformers.
 */
template <class T>
class StreamBase : public StreamBaseBase {
public:
    StreamBase() { }
    virtual ~StreamBase(){}
    /**
     * @brief Reads characters from the stream and sets \a start to
     * the first character that was read.
     *
     * If @p ntoread is @c 0, then at least one character will be read.
     *
     * @param start Pointer passed by reference that will be set to point to
     *              the retrieved array of characters. If the end of the stream
     *              is encountered or an error occurs, the value of @p start
     *              is undefined.
     * @param ntoread The number of characters to read from the stream. If
     *                @p is @c 0 the stream reads at least 1 character.
     * @return the number of characters that were read. If -1 is returned, the
     *         end of the stream has been reached. If -2 is returned, an error
     *         has occurred.
     **/
    virtual int32_t read(const T*& start, int32_t min, int32_t max) = 0;
    /**
     * Skip @param ntoskip bytes. Unless an error occurs or the end of file is
     * encountered, this amount of bytes is skipped.
     * This function returns new position in the stream.
     **/
    virtual int64_t skip(int64_t ntoskip);
      /**
       * @brief Repositions this stream to given requested position.
       * Reset is guaranteed to work after a successful call to read(),
       * when the new position is in the range of the data returned by read().
       * This means that @p pos must lie between the the position
       * corresponding to the @p start parameter (x) of the @r read function
       * and the position corresponding to the last position in the returned
       * buffer (x + @p nread).
       **/
    virtual int64_t reset(int64_t pos) = 0;
    /**
     * deprecated function
     **/
    int64_t mark(int32_t readlimit) {
        int64_t p = getPosition();
        const T* ptr;
        read(ptr, readlimit, -1);
        return reset(p);
    }
};
#define SKIPSTEP 1024
template <class T>
int64_t
StreamBase<T>::skip(int64_t ntoskip) {
    const T *begin;
    int32_t nread;
    int64_t skipped = 0;
    while (ntoskip) {
        int32_t step = (int32_t)((ntoskip > SKIPSTEP) ?SKIPSTEP :ntoskip);
        nread = read(begin, 1, step);
        if (nread < -1 ) {
            // an error occurred
            return nread;
        } else if (nread < 1) {
            ntoskip = 0;
        } else {
            skipped += nread;
            ntoskip -= nread;
        }
    }
    return skipped;
}

} // end namespace jstreams

#endif
