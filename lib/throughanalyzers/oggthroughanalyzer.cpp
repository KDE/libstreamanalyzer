/* This file is part of Strigi Desktop Search
 *
 * Copyright (C) 2006 Jos van den Oever <jos@vandenoever.info>
 *               2007 Tobias Pfeiffer <tgpfeiffer@web.de>
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

#include "oggthroughanalyzer.h"
#include <strigi/strigiconfig.h>
#include <strigi/analysisresult.h>
#include <strigi/textutils.h>
#include "../rdfnamespaces.h"
#include <iostream>
#include <cctype>
#include <cstring>
using namespace Strigi;
using namespace std;

const string
    typePropertyName(
	"http://www.w3.org/1999/02/22-rdf-syntax-ns#type"),
    fullnamePropertyName(
	"http://www.semanticdesktop.org/ontologies/2007/03/22/nco#fullname"),
    titlePropertyName(
	"http://www.semanticdesktop.org/ontologies/2007/01/19/nie#title"),
    albumTrackCountName(
        NMM_DRAFT "albumTrackCount"),

    musicClassName(
	NMM_DRAFT "MusicPiece"),
    albumClassName(
	NMM_DRAFT "MusicAlbum"),
    contactClassName(
	"http://www.semanticdesktop.org/ontologies/2007/03/22/nco#Contact");

void
OggThroughAnalyzerFactory::registerFields(FieldRegister& r) {
    fields["title"] = r.registerField(titlePropertyName);
    albumField = r.registerField(NMM_DRAFT "musicAlbum");
    fields["genre"] = r.registerField(NMM_DRAFT "genre");
    fields["codec"] = r.registerField("http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#codec");
    composerField = r.registerField(NMM_DRAFT "composer");
    performerField = r.registerField(NMM_DRAFT "performer");
    fields["date"] = r.registerField("http://www.semanticdesktop.org/ontologies/2007/01/19/nie#contentCreated");
    fields["description"] = r.registerField("http://www.semanticdesktop.org/ontologies/2007/01/19/nie#description");
    fields["tracknumber"] = r.registerField(NMM_DRAFT "trackNumber");


    fields["version"] = r.registerField("http://www.semanticdesktop.org/ontologies/2007/01/19/nie#version");
    fields["isrc"] = r.registerField(NMM_DRAFT "internationalStandardRecordingCode");
    fields["copyright"] = r.registerField("http://www.semanticdesktop.org/ontologies/2007/01/19/nie#copyright");
    fields["license"] = r.registerField("http://www.semanticdesktop.org/ontologies/2007/01/19/nie#license");

// ogg spec fields left unimplemented: ORGANIZATION, LOCATION, CONTACT

    fields["type"] = r.typeField;
}

inline
void
addStatement(AnalysisResult* indexable, string& subject, const string& predicate, const string& object) {
  if (subject.empty())
    subject = indexable->newAnonymousUri();
  indexable->addTriplet(subject, predicate, object);
}

void
OggThroughAnalyzer::setIndexable(AnalysisResult* i) {
    indexable = i;
}
InputStream*
OggThroughAnalyzer::connectInputStream(InputStream* in) {
    if(!in) {
        return in;
    }

    const char* buf;
    // read 1024 initially
    int32_t nreq = 1024;
    int32_t nread = in->read(buf, nreq, nreq);
    in->reset(0);
    // check the header for signatures
    // the first ogg page starts at position 0, the second at position 58
    if (nread < nreq || strcmp("OggS", buf) || strcmp("vorbis", buf+29)
            || strcmp("OggS", buf+58)) {
        return in;
    }
    // read the number of page segments at 58 + 26
    unsigned char segments = (unsigned char)buf[84];
    if (85 + segments >= nread) {
        // this cannot be a good vorbis file: the initial page is too large
        return in;
    }

    // read the sum of page segment sizes 
    int psize = 0;
    for (int i=0; i<segments; ++i) {
        psize += (unsigned char)buf[85+i];
    }
    // read the entire first two pages
    nreq = 85 + segments + psize;
    nread = in->read(buf, nreq, nreq);
    in->reset(0);
    if (nread < nreq) {
        return in;
    }
    // we have now read the second Ogg Vorbis header containing the comments
    // get a pointer to the first page segment in the second page
    const char* p2 = buf + 85 + segments;
    // check the header of the 'vorbis' page
    if (psize < 15 || strncmp(p2 + 1, "vorbis", 6)) {
        return in;
    }
    // get a pointer to the end of the second page
    const char* end = p2 + psize;
    uint32_t size = readLittleEndianUInt32(p2+7);
    // advance to the position of the number of fields and read it
    p2 += size + 11;
    if (p2 + 4 > end) {
        return in;
    }
    uint32_t nfields = readLittleEndianUInt32(p2);

    // in Vorbis comments the "artist" field is used for the performer in modern music
    // but for the composer in calssical music. Thus, we cache both and make the decision
    // at the end
    string artist, performer;
    string albumUri;

    // read all the comments
    p2 += 4;
    for (uint32_t i = 0; p2 < end && i < nfields; ++i) {
        // read the comment length
        size = readLittleEndianUInt32(p2);
        p2 += 4;
        if (size <= (uint32_t)(end - p2)) {
            uint32_t eq = 1;
            while (eq < size && p2[eq] != '=') eq++;
            if (size > eq) {
                string name(p2, eq);
                // convert field name to lower case
                const string::size_type length = name.length();
                for(string::size_type k=0; k!=length; ++k) {
                    name[k] = (char)std::tolower(name[k]);
                }
                // check if we can handle this field and if so handle it
                map<string, const RegisteredField*>::const_iterator iter
                    = factory->fields.find(name);
                string value(p2+eq+1, size-eq-1);
                if (iter != factory->fields.end()) {
                    // Hack: the tracknumber sometimes contains the track count, too
                    int pos = 0;
                    if(name=="tracknumber" && (pos = value.find_first_of('/')) > 0 ) {
                        // the track number
                        indexable->addValue(iter->second, value.substr(0, pos));
                        // the track count
                        addStatement(indexable, albumUri, albumTrackCountName, value.substr(pos+1));
                    }
                    else {
                        indexable->addValue(iter->second, value);
                    }
                } else if(name=="artist") {
                    artist = value;
                } else if(name=="album") {
                    addStatement(indexable, albumUri, titlePropertyName, value);
		} else if(name=="composer") {
		    string composerUri = indexable->newAnonymousUri();

		    indexable->addValue(factory->composerField, composerUri);
		    indexable->addTriplet(composerUri, typePropertyName, contactClassName);
		    indexable->addTriplet(composerUri, fullnamePropertyName, value);
		} else if(name=="performer") {
                    performer = value;
		}
            }
        } else {
            cerr << "problem with tag size of " << size << endl;
            return in;
        }
        p2 += size;
    }

    // we now decide how to store the artist and performer as suggested by the Vorbis comments spec
    const Strigi::RegisteredField* artistField = 0;
    const Strigi::RegisteredField* performerField = 0;
    if (!artist.empty()) {
        if (!performer.empty()) {
            artistField = factory->composerField;
            performerField = factory->performerField;
        }
        else {
            artistField = factory->performerField;
        }
    }
    else if (!performer.empty()) {
        performerField = factory->performerField;
    }
    if (artistField) {
        const string artistUri( indexable->newAnonymousUri() );

        indexable->addValue(artistField, artistUri);
        indexable->addTriplet(artistUri, typePropertyName, contactClassName);
        indexable->addTriplet(artistUri, fullnamePropertyName, artist);
    }
    if (performerField) {
        const string performerUri( indexable->newAnonymousUri() );

        indexable->addValue(performerField, performerUri);
        indexable->addTriplet(performerUri, typePropertyName, contactClassName);
        indexable->addTriplet(performerUri, fullnamePropertyName, performer);
    }
    if(!albumUri.empty()) {
      indexable->addValue(factory->albumField, albumUri);
      indexable->addTriplet(albumUri, typePropertyName, albumClassName);
    }

    // set the "codec" value
    indexable->addValue(factory->fields.find("codec")->second, "Ogg/Vorbis");
    indexable->addValue(factory->fields.find("type")->second, musicClassName);
    return in;
}
bool
OggThroughAnalyzer::isReadyWithStream() {
    return true;
}
