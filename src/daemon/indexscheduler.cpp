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
#include "indexscheduler.h"
#include "indexmanager.h"
#include "indexreader.h"
#include "indexwriter.h"

#include "strigiconfig.h"
#include "event.h"
#include "eventlistenerqueue.h"
#include "analyzerconfiguration.h"

#include "filelister.h"
#include "streamanalyzer.h"
#include <cerrno>
#include <sys/resource.h>

#include "strigilogging.h"

using namespace std;

IndexScheduler* sched;

IndexScheduler::IndexScheduler() :StrigiThread("IndexScheduler") {
    sched = this;
    m_listenerEventQueue = NULL;
}
IndexScheduler::~IndexScheduler() {
}
std::string
IndexScheduler::getStateString() {
    State state = getState();
    if (state == Idling) return "idling";
    if (state == Working) return "indexing";
    return "stopping";
}
int
IndexScheduler::getQueueSize() {
    return toindex.size();
}
void
shortsleep(long nanoseconds) {
    // set sleep time
    struct timespec sleeptime;
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = nanoseconds;
    nanosleep(&sleeptime, 0);
}
void *
IndexScheduler::run(void*) {
    while (getState() != Stopping) {
        shortsleep(100000000);
        if (getState() == Working) {
            index();
            if (getState() == Working) {
                setState(Idling);
            }
        }
        else if (getState() == Idling) {
            if (m_listenerEventQueue == NULL)
                return 0;

            vector <Event*> events = m_listenerEventQueue->getEvents();
            if (events.size() > 0) {
                setState(Working);
                processListenerEvents(events);
                setState(Idling);
            }
        }
    }

    STRIGI_LOG_DEBUG ("strigi.IndexScheduler.run", string("exit state: ") + getStringState());
    return 0;
}
void
IndexScheduler::index() {
    Strigi::IndexReader* reader = indexmanager->indexReader();
    Strigi::IndexWriter* writer = indexmanager->indexWriter();
    Strigi::StreamAnalyzer* streamindexer
        = new Strigi::StreamAnalyzer(*m_indexerconfiguration);
    streamindexer->setIndexWriter(*writer);

    if (dbfiles.size() == 0 && toindex.size() == 0) {
        // retrieve the list of real files currently in the database
        dbfiles = reader->files(0);

        char buff [20];
        snprintf(buff, 20* sizeof (char), "%i", dbfiles.size());
        STRIGI_LOG_DEBUG ("strigi.IndexScheduler", string(buff) + " real files in the database")

        // first loop through all files
        Strigi::FileLister lister(m_indexerconfiguration);
        STRIGI_LOG_DEBUG ("strigi.IndexScheduler", "going to index")
        set<string>::const_iterator i;
        for (i = dirstoindex.begin(); i != dirstoindex.end(); ++i) {
            lister.startListing(i->c_str());
            string path;
            time_t mtime;
            int r = lister.nextFile(path, mtime);
            while (r >= 0) {
                fprintf(stderr, "r %i\n", r);
                if (r > 0) {
                    map<string, time_t>::iterator i
                        = sched->dbfiles.find(path);
                    // if the file has not yet been indexed or if the mtime has
                    // changed, put it in the list to index
                    if (i == sched->dbfiles.end() || i->second != mtime) {
                        sched->toindex[path] = mtime;
                    } else {
                        sched->dbfiles.erase(i);
                    }
                }
                r = lister.nextFile(path, mtime);
            }
        }

        snprintf(buff, 20* sizeof (char), "%i", dbfiles.size());
        STRIGI_LOG_DEBUG ("strigi.IndexScheduler", string(buff) + " files to remove")

        snprintf(buff, 20* sizeof (char), "%i", toindex.size());
        STRIGI_LOG_DEBUG ("strigi.IndexScheduler", string(buff) + " files to add or update")
    }

    vector<string> todelete;
    map<string,time_t>::iterator it = dbfiles.begin();
    while (getState() == Working && it != dbfiles.end()) {
        todelete.push_back(it->first);
//        writer->deleteEntry(it->first);
        dbfiles.erase(it++);
    }
    writer->deleteEntries(todelete);

    it = toindex.begin();
    while (getState() == Working && it != toindex.end()) {
        streamindexer->indexFile(it->first);
        if (writer->itemsInCache() > 10000) {
            writer->commit();
        }
        toindex.erase(it++);
    }
    if (getState() == Working) {
        writer->commit();
        writer->optimize();
    }

    delete streamindexer;
}

void
IndexScheduler::processListenerEvents(vector<Event*>& events) {
    Strigi::IndexReader* reader = indexmanager->indexReader();
    Strigi::IndexWriter* writer = indexmanager->indexWriter();
    Strigi::AnalyzerConfiguration ic;
    Strigi::StreamAnalyzer* streamindexer = new Strigi::StreamAnalyzer(ic);
    streamindexer->setIndexWriter(*writer);

    vector<string> toDelete;

    STRIGI_LOG_DEBUG ("strigi.IndexScheduler", "processing listener's events")

    for(vector<Event*>::iterator iter = events.begin(); iter != events.end(); iter++)
    {
        Event* event = *iter;

        STRIGI_LOG_DEBUG ("strigi.IndexScheduler", "event infos: " + event->toString())

        switch (event->getType())
        {
            case Event::CREATED:
                toindex.insert (make_pair (event->getPath(), event->getTime()));
                break;
            case Event::UPDATED:
            {
                time_t indexTime = reader->mTime(reader->documentId(event->getPath()));
                if (indexTime < event->getTime())
                {
                    toindex.insert (make_pair (event->getPath(), event->getTime()));
                    toDelete.push_back (event->getPath());
                }
                else
                    STRIGI_LOG_DEBUG ("strigi.IndexScheduler", "ignoring last event")
                break;
            }
            case Event::DELETED:
                toDelete.push_back (event->getPath());
                break;
        }

        delete event;
    }
    writer->deleteEntries(toDelete);

    map<string, time_t>::iterator it = toindex.begin();
    while (it != toindex.end())
    {
        streamindexer->indexFile(it->first);
        if (writer->itemsInCache() > 10000) {
            writer->commit();
        }
        toindex.erase(it++);
    }

    writer->commit();
    writer->optimize();

    delete streamindexer;
}

void
IndexScheduler::setIndexedDirectories(const std::set<std::string> &d) {
    dirstoindex.clear();
    std::set<std::string>::const_iterator i;
    for (i = d.begin(); i!=d.end(); ++i) {
        bool ok = true;
        std::set<std::string>::iterator j;
        for (j = dirstoindex.begin(); ok && j != dirstoindex.end(); ++j) {
            if (j->length() >= i->length()
                && j->substr(0, i->length()) == *i) {
                dirstoindex.erase(j);
                j = dirstoindex.begin();
            } else if (i->length() >= j->length()
                && i->substr(0, j->length()) == *j) {
// for now, allow subdirs, because they might not be included otherwise
//                ok = false;
            }
        }
        if (ok) {
            string dir = *i;
            if (dir[dir.length()-1] == '/') {
                dir = dir.substr(0, dir.length()-1);
            }
            dirstoindex.insert(dir);
        }
    }
    if (dirstoindex.size() == 0) {
        indexmanager->indexWriter()->deleteAllEntries();
    }
}
