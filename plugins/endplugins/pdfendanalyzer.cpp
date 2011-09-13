/*
   Copyright (C) 2011  Vishesh Handa <handa.vish@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <strigi/streambase.h>
#include <strigi/streamendanalyzer.h>
#include <strigi/streamanalyzerfactory.h>
#include <strigi/analyzerplugin.h>
#include <strigi/analysisresult.h>
#include <strigi/fieldtypes.h>
#include <strigi/textutils.h>

#include <string.h>
#include <list>
#include <iostream>

#include "poppler/Page.h"
#include "poppler/PDFDoc.h"
#include <poppler/TextOutputDev.h>
#include <poppler/Stream.h>
#include <poppler/CachedFile.h>
#include <poppler/Dict.h>
#include <poppler/GlobalParams.h>
#include <poppler/UnicodeMap.h>

class PdfEndAnalyzerFactory;

class STRIGI_EXPORT PdfEndAnalyzer : public Strigi::StreamEndAnalyzer {
public:
    PdfEndAnalyzer( const PdfEndAnalyzerFactory *f );
    virtual ~PdfEndAnalyzer();

    const char* name() const {
        return "PdfEndAnalyzer";
    }

    bool checkHeader(const char* header, int32_t headersize) const;
    signed char analyze(Strigi::AnalysisResult& idx, Strigi::InputStream* in);

private:
    const PdfEndAnalyzerFactory *m_factory;
    UnicodeMap *m_unicodeMap;
};

class STRIGI_EXPORT PdfEndAnalyzerFactory : public Strigi::StreamEndAnalyzerFactory {
public:
    Strigi::StreamEndAnalyzer* newInstance() const {
        return new PdfEndAnalyzer(this);
    }

    const char* name() const {
        return "PdfEndAnalyzer";
    }

    void registerFields(Strigi::FieldRegister& );

    const Strigi::RegisteredField *typeProperty;
};

void PdfEndAnalyzerFactory::registerFields(Strigi::FieldRegister& fieldRegister)
{
    typeProperty = fieldRegister.typeField;
    //fieldRegister.registerField(  );
}

class StrigiInputStreamLoader : public CachedFileLoader {
public:
    StrigiInputStreamLoader( Strigi::InputStream *inputStream );

    virtual size_t init(GooString* uri, CachedFile* cachedFile);
    virtual int load(const std::vector< ByteRange >& ranges, CachedFileWriter* writer) {
        return 0;
    }

private:
    Strigi::InputStream *m_inputStream;
};

StrigiInputStreamLoader::StrigiInputStreamLoader(Strigi::InputStream* inputStream)
    : m_inputStream( inputStream )
{
}

size_t StrigiInputStreamLoader::init(GooString* uri, CachedFile* cachedFile)
{
    size_t size = 0;

    CachedFileWriter writer = CachedFileWriter(cachedFile, NULL);
    while( 1 ) {
        const char *data=0;
        int bytesRead = m_inputStream->read( data, 1, CachedFileChunkSize );
        if( bytesRead == 0 ) {
            // We're done
            break;
        }
        else if( bytesRead == -2 ) {
            // Some error
            return -1;
        }
        writer.write(data, bytesRead);
        size += bytesRead;
    }

    return size;
}

/// Used by TextOutputDev
void ouptputFunc( void *stream, char *text, int len ) {
    if( len < 0 )
        return;

    Strigi::AnalysisResult *result = static_cast<Strigi::AnalysisResult*>(stream);
    result->addText( text, len );
}

PdfEndAnalyzer::PdfEndAnalyzer(const PdfEndAnalyzerFactory* f)
    : m_factory(f)
{
    // Required by poppler
    // globalParams is a global variable declared in <poppler/GlobalParams.h>
    // which must be initialized on startup
    globalParams = new GlobalParams();
    globalParams->setTextPageBreaks(gFalse);
    globalParams->setTextEncoding("UTF-8");
}

PdfEndAnalyzer::~PdfEndAnalyzer()
{
    delete globalParams;
}


//
// Check if we have a PDF file
//
bool PdfEndAnalyzer::checkHeader(const char* header, int32_t headersize) const
{
    // PDF files start with a "%PDF-1.x", where x can be anywhere from 0-7
    if( headersize < 8 )
        return false;

    if( strncmp(header, "%PDF-1.", 7) != 0 )
        return false;

    return true;
}

signed char PdfEndAnalyzer::analyze(Strigi::AnalysisResult& idx, Strigi::InputStream* in)
{
    Object obj;
    obj.initNull();

    // Create a file stream
    CachedFile *cachedFile = new CachedFile(new StrigiInputStreamLoader(in), NULL);
    BaseStream *stream = new CachedFileStream(cachedFile, 0, gFalse,
                                              cachedFile->getLength(), &obj);
    PDFDoc *doc = new PDFDoc( stream );

    if( !doc->isOk() )
        return doc->getErrorCode();

    m_unicodeMap = globalParams->getTextEncoding();
    std::cout << "Is Unicode: " << m_unicodeMap->isUnicode() << std::endl;

    //
    // Headers
    //
    Object info;
    doc->getDocInfo(&info);
    if( info.isDict() ) {
        Dict* dict = info.getDict();

        Object obj;
        //FIXME: utf8 support?
        if( dict->lookup("Author", &obj)->isString() ) {
            char *author = obj.getString()->getCString();
            //TODO: Parse email
            std::cout << "Author : " << author << std::endl;
        }

        if( dict->lookup("Title", &obj)->isString() ) {
            char *title = obj.getString()->getCString();
            std::cout << "Title : " << title << std::endl;
        }

        if( dict->lookup("Subject", &obj)->isString() ) {
            char *subject = obj.getString()->getCString();
            std::cout << "Subject : " << subject << std::endl;
        }

        if( dict->lookup("Keywords", &obj)->isString() ) {
            char *keywords = obj.getString()->getCString();
            //TODO: Parse comma separated tags
            std::cout << "Keywords : " << keywords << std::endl;
        }
    }
    info.free();

    //
    // Raw text
    //
    // If the physical layout is maintained then there are large amounts of spaces
    const bool maintainPhysicalLayout = false;
    // If this is true, then there is a new line after every word
    const bool rawOrder = false;

    TextOutputDev output( ouptputFunc, static_cast<void*>(&idx), maintainPhysicalLayout, rawOrder );

    const double resolution = 72.0;

    // For some insane reason poppler starts the page count from 1
    const int firstPage = 1;
    const int lastPage = doc->getNumPages();

    doc->displayPages( &output, firstPage, lastPage, resolution, resolution, 0, true, false, false );

    // The types
    idx.addValue( m_factory->typeProperty, "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#PaginatedTextDocument" );

    // Yup, that's about it
    return 0;
}


/*
 For plugins, we need to have a way to find out which plugins are defined in a
 plugin. One instance of AnalyzerFactoryFactory per plugin profides this
 information.
*/
class Factory : public Strigi::AnalyzerFactoryFactory {
public:
    std::list<Strigi::StreamEndAnalyzerFactory*> streamEndAnalyzerFactories() const {
        std::list<Strigi::StreamEndAnalyzerFactory*> af;
        af.push_back(new PdfEndAnalyzerFactory());
        return af;
    }
};

/*
 * Register the AnalyzerFactoryFactory
 */
STRIGI_ANALYZER_FACTORY(Factory)
