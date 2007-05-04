/* This file is part of Strigi Desktop Search
 *
 * Copyright (C) 2001, 2002 Rolf Magnus <ramagnus@kde.org>
 * Copyright (C) 2006 Jos van den Oever <jos@vandenoever.info>
 * Copyright (C) 2007 Vincent Ricard <magic@magicninja.org>
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
#include "strigiconfig.h"
#include <time.h>

#include "pngendanalyzer.h"
#include "analysisresult.h"
#include "textendanalyzer.h"
#include "subinputstream.h"
#include "fieldtypes.h"
#include "gzipinputstream.h"
#include "textutils.h"
using namespace std;
using namespace Strigi;

const string PngEndAnalyzerFactory::widthFieldName("image.width");
const string PngEndAnalyzerFactory::heightFieldName("image.height");
const string PngEndAnalyzerFactory::colorDepthFieldName("image.color_depth");
const string PngEndAnalyzerFactory::colorModeFieldName("image.color_space");
const string PngEndAnalyzerFactory::compressionFieldName("compressed.compression_algorithm");
const string PngEndAnalyzerFactory::interlaceModeFieldName("image.interlace");
const string PngEndAnalyzerFactory::lastModificationTimeFieldName("lastModificationTime");
const string PngEndAnalyzerFactory::titleFieldName("content.title");
const string PngEndAnalyzerFactory::authorFieldName("content.author");
const string PngEndAnalyzerFactory::descriptionFieldName("content.description");
const string PngEndAnalyzerFactory::copyrightFieldName("content.copyright");
const string PngEndAnalyzerFactory::creationTimeFieldName("content.creation_time");
const string PngEndAnalyzerFactory::softwareFieldName("content.generator");
const string PngEndAnalyzerFactory::disclaimerFieldName("content.disclaimer");
const string PngEndAnalyzerFactory::warningFieldName("content.comment");
 // PNG spec says Source is Device used to create the image
const string PngEndAnalyzerFactory::sourceFieldName("photo.camera_model");
const string PngEndAnalyzerFactory::commentFieldName("content.comment");

// and for the colors
static const char* colors[] = {
  "Grayscale",
  "Unknown",
  "RGB",
  "Palette",
  "Grayscale/Alpha",
  "Unknown",
  "RGB/Alpha"
};

static const char* interlaceModes[] = {
  "None",
  "Adam7"
};

void
PngEndAnalyzerFactory::registerFields(FieldRegister& reg) {
    widthField = reg.registerField(widthFieldName,
        FieldRegister::integerType, 1, 0);
    heightField = reg.registerField(heightFieldName,
        FieldRegister::integerType, 1, 0);
    colorDepthField = reg.registerField(colorDepthFieldName,
        FieldRegister::integerType, 1, 0);
    colorModeField = reg.registerField(colorModeFieldName,
        FieldRegister::integerType, 1, 0);
    compressionField = reg.registerField(compressionFieldName,
        FieldRegister::integerType, 1, 0);
    interlaceModeField = reg.registerField(interlaceModeFieldName,
        FieldRegister::integerType, 1, 0);
    lastModificationTimeField = reg.registerField(lastModificationTimeFieldName,
        FieldRegister::integerType, 1, 0);
    titleField = reg.registerField(titleFieldName,
        FieldRegister::stringType, 1, 0);
    authorField = reg.registerField(authorFieldName,
        FieldRegister::stringType, 1, 0);
    descriptionField = reg.registerField(descriptionFieldName,
        FieldRegister::stringType, 1, 0);
    copyrightField = reg.registerField(copyrightFieldName,
        FieldRegister::stringType, 1, 0);
    creationTimeField = reg.registerField(creationTimeFieldName,
        FieldRegister::integerType, 1, 0);
    softwareField = reg.registerField(softwareFieldName,
        FieldRegister::stringType, 1, 0);
    disclaimerField = reg.registerField(disclaimerFieldName,
        FieldRegister::stringType, 1, 0);
    warningField = reg.registerField(warningFieldName,
        FieldRegister::stringType, 1, 0);
    sourceField = reg.registerField(sourceFieldName,
        FieldRegister::stringType, 1, 0);
    commentField = reg.registerField(commentFieldName,
        FieldRegister::stringType, 1, 0);
}

PngEndAnalyzer::PngEndAnalyzer(const PngEndAnalyzerFactory* f) :factory(f) {
    // XXX hack to workaround mktime
    // which takes care of the local time zone
    struct tm timeZone;
    timeZone.tm_sec = 0;
    timeZone.tm_min = 0;
    timeZone.tm_hour = 0;
    timeZone.tm_mday = 1;
    timeZone.tm_mon = 0;
    timeZone.tm_year = 70;
    timeZone.tm_isdst = 0;
    timeZoneOffset = mktime(&timeZone);
}
bool
PngEndAnalyzer::checkHeader(const char* header, int32_t headersize) const {
    static const unsigned char pngmagic[]
        = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
    return headersize >= 29 && memcmp(header, pngmagic, 8) == 0;
}
char
PngEndAnalyzer::analyze(AnalysisResult& as, InputStream* in) {
    const char* c;
    int32_t nread = in->read(c, 12, 12);
    if (nread != 12) {
        // file is too small to be a png
        return -1;
    }

    // read chunksize and include the size of the type and crc (4 + 4)
    uint32_t chunksize = readBigEndianUInt32(c+8) + 8;
    if (chunksize > 1048576) {
        fprintf(stderr,"chunk too big: %u\n",chunksize);
        return -1;
    }
    nread = in->read(c, chunksize, chunksize);
    // the IHDR chunk should be the first
    if (nread != (int32_t)chunksize || strncmp(c, "IHDR", 4)) {
        return -1;
    }

    // read the png dimensions
    uint32_t width = readBigEndianUInt32(c+4);
    uint32_t height = readBigEndianUInt32(c+8);
    as.addValue(factory->widthField, width);
    as.addValue(factory->heightField, height);

    uint16_t type = c[13];
    uint16_t bpp = c[12];

    // the bpp are only per channel, so we need to multiply the with
    // the channel count
    switch (type) {
        case 0: break;           // Grayscale
        case 2: bpp *= 3; break; // RGB
        case 3: break;           // palette
        case 4: bpp *= 2; break; // grayscale w. alpha
        case 6: bpp *= 4; break; // RGBA

        default: // we don't get any sensible value here
            bpp = 0;
    }

    as.addValue(factory->colorDepthField, (uint32_t)bpp);
    as.addValue(factory->colorModeField,
        (type < sizeof(colors)/sizeof(colors[0]))
                   ? colors[type] : "Unknown");

    as.addValue(factory->compressionField,
        (c[14] == 0) ? "Deflate" : "Unknown");

    as.addValue(factory->interlaceModeField,
        ((uint)c[16] < sizeof(interlaceModes)/sizeof(interlaceModes[0]))
                   ? interlaceModes[(int)c[16]] : "Unknown");

    // read the rest of the chunks
    // TODO: check if we want a fast or complete analysis
    // read the type
    nread = in->read(c, 8, 8);
    while (nread == 8 && strncmp("IEND", c+4, 4)) {
        // get the size of the data block
        chunksize = readBigEndianUInt32(c);

        if (strncmp("tEXt", c+4, 4) == 0) {
            // TODO convert latin1 to utf8 and analyze the format properly
            SubInputStream sub(in, chunksize);
            analyzeText(as, &sub);
            sub.skip(chunksize);
        } else if (strncmp("zTXt", c+4, 4) == 0) {
            SubInputStream sub(in, chunksize);
            analyzeZText(as, &sub);
            sub.skip(chunksize);
        } else if (strncmp("iTXt", c+4, 4) == 0) {
            SubInputStream sub(in, chunksize);
            analyzeText(as, &sub);
            sub.skip(chunksize);
        } else if (strncmp("tIME", c+4, 4) == 0) {
            SubInputStream sub(in, chunksize);
            analyzeTime(as, &sub);
            sub.skip(chunksize);
        } else {
            nread = (int32_t)in->skip(chunksize);
            if (nread != (int32_t)chunksize) {
                fprintf(stderr, "could not skip chunk size %u\n", chunksize);
                return -1;
            }
        }
        in->skip(4); // skip crc
        nread = in->read(c, 8, 8);
    }
    if (nread != 8) {
        // invalid file or error
        fprintf(stderr, "bad end in %s\n", as.path().c_str());
        return -1;
    }

    return 0;
}
char
PngEndAnalyzer::analyzeText(Strigi::AnalysisResult& as,
        InputStream* in) {
    const char* c;
    int32_t nread = in->read(c, 80, 80);
    if (nread < 1) {
         return -1;
    }
    // find the \0
    int32_t nlen = 0;
    while (nlen < nread && c[nlen]) nlen++;
    if (nlen == nread) return -1;
    const string name(c, nlen); // do something with the name!
    in->reset(nlen+1);
    return addMetaData(name, as, in);
}
char
PngEndAnalyzer::analyzeZText(Strigi::AnalysisResult& as,
        InputStream* in) {
    const char* c;
    int32_t nread = in->read(c, 81, 81);
    if (nread < 1) {
         return -1;
    }
    // find the \0
    int32_t nlen = 0;
    while (nlen < nread && c[nlen]) nlen++;
    if (nlen == nread) return -1;
    const string name(c, nlen); // do something with the name!
    in->reset(nlen+2);
    GZipInputStream z(in, GZipInputStream::ZLIBFORMAT);
    return addMetaData(name, as, &z);
}
char
PngEndAnalyzer::analyzeTime(Strigi::AnalysisResult& as,
        Strigi::InputStream* in) {
    const char* chunck;
    int32_t nread = in->read(chunck, 7, 7);
    if (nread != 7) {
        return -1;
    }

    int16_t year = readBigEndianInt16(chunck);
    int8_t month = *(chunck+2);
    int8_t day = *(chunck+3);
    int8_t hour = *(chunck+4);
    int8_t minute = *(chunck+5);
    int8_t second = *(chunck+6);
    // check the data (the leap second is allowed)
    if (!(1 <= month && month <= 12
            && 1 <= day && day <= 31
            && 0 <= hour && hour <= 23
            && 0 <= minute && minute <= 59
            && 0 <= second && second <= 60)) {
        return -1;
    }
    // we want to store the date/time as a number of
    // seconds since Epoch (1970-01-01T00:00:00)
    struct tm dateTime;
    dateTime.tm_sec = second;
    dateTime.tm_min = minute;
    dateTime.tm_hour = hour;
    dateTime.tm_mday = day;
    dateTime.tm_mon = month-1;
    dateTime.tm_year = year-1900;
    dateTime.tm_isdst = 0;

    time_t sinceEpoch = mktime(&dateTime);
    if (sinceEpoch == (time_t)-1) {
        fprintf(stderr, "could not compute the date/time\n");
        return -1;
    }

    // FIXME the chunck is UTC but mktime use the local timezone :-(
    // so i have to add the offset of the local time zone
    // If someone has a better solution...
    uint32_t time = sinceEpoch + timeZoneOffset;
    as.addValue(factory->lastModificationTimeField, (uint32_t)time);

    return 0;
}
char
PngEndAnalyzer::addMetaData(const string& key,
        Strigi::AnalysisResult& as, InputStream* in) {
    // try to store 1KB (should we get more?)
    const char* b;
    int32_t nread = in->read(b, 1024, 0);
    if (in->status() == Error) {
        m_error = in->error();
        return -1;
    }
    if (0 < nread) {
        const string value(b, nread);
        if ("Title" == key) {
            as.addValue(factory->titleField, value);
        } else if ("Author" == key) {
            as.addValue(factory->authorField, value);
        } else if ("Description" == key) {
            as.addValue(factory->descriptionField, value);
        } else if ("Copyright" == key) {
            as.addValue(factory->copyrightField, value);
        } else if ("Creation Time" == key) {
            // TODO we need to parse the date time
            // "[...]the date format defined in section 5.2.14 of RFC 1123[...]"
        } else if ("Software" == key) {
            as.addValue(factory->softwareField, value);
        } else if ("Disclaimer" == key) {
            as.addValue(factory->disclaimerField, value);
        } else if ("Warning" == key) {
            as.addValue(factory->warningField, value);
        } else if ("Source" == key) {
            as.addValue(factory->sourceField, value);
        } else if ("Comment" == key) {
            as.addValue(factory->commentField, value);
        }
    }
    return 0;
}
