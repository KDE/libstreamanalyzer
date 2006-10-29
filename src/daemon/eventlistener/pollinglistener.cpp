/* This file is part of Strigi Desktop Search
 *
 * Copyright (C) 2006 Flavio Castelli <flavio.castelli@gmail.com>
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
#include "jstreamsconfig.h"
#include "pollinglistener.h"

#include <errno.h>
#include <dirent.h>
#include <sys/types.h>

#include "event.h"
#include "eventlistenerqueue.h"
#include "filtermanager.h"
#include "filelister.h"
#include "indexreader.h"
#include "../strigilogging.h"

using namespace std;
using namespace jstreams;

PollingListener* PollingListener::workingPoller;

PollingListener::PollingListener()
    :EventListener("PollingListener")
{
    workingPoller = this;
    setState(Idling);
    pthread_mutex_init (&m_mutex, 0);
    m_pause = 60; //60 seconds of pause between each polling
}

PollingListener::PollingListener(set<string>& dirs)
    :EventListener("PollingListener")
{
    workingPoller = this;
    setState(Idling);
    pthread_mutex_init (&m_mutex, 0);
    m_pause = 60; //60 seconds of pause between each polling
    
    addWatches( dirs);
}

PollingListener::~PollingListener()
{
    m_watches.clear();
    pthread_mutex_destroy (&m_mutex);
}

void* PollingListener::run(void*)
{
    STRIGI_LOG_DEBUG ("strigi.PollingListener.run", "started");
    
    while (getState() != Stopping)
    {
        // wait m_pause seconds before polling again
        if (!m_watches.empty())
        {
            sleep (m_pause);
            pool();
        }

        if (getState() == Working)
            setState(Idling);
    }
    
    STRIGI_LOG_DEBUG ("strigi.PollingListener.run", string("exit state: ") + getStringState());
    return 0;
}

void PollingListener::pool ()
{
    if (!m_pIndexReader) {
        STRIGI_LOG_ERROR ("strigi.PollingListener.pool", "m_pEventQueue == NULL!")
        return;
    }
    if (m_pEventQueue == NULL)
    {
        STRIGI_LOG_ERROR ("strigi.PollingListener.pool", "m_pEventQueue == NULL!")
        return;
    }
    
    vector<Event*> events;
    map <string, time_t> indexedFiles = m_pIndexReader->getFiles(0);
    set<string> watches;
    
    FileLister lister (m_pFilterManager);
    lister.setFileCallbackFunction(&fileCallback);
    
    m_toIndex.clear();
    
    // get a shadow copy of m_watches
    pthread_mutex_lock (&m_mutex);
    watches = m_watches;
    pthread_mutex_unlock (&m_mutex);
    
    STRIGI_LOG_DEBUG ("strigi.PollingListener.pool", "going across filesystem")
    
    // walk through the watched dirs
    for (set<string>::const_iterator iter = watches.begin(); iter != watches.end(); iter++)
    {
        // check if dir still exists
        DIR* dir = opendir(iter->c_str());
        if (dir == NULL)
        {
            STRIGI_LOG_DEBUG ("strigi.PollingListener.pool", "error opening dir " + *iter + ": " + strerror (errno))
            
            // dir doesn't exists anymore, remove it for the watches list
            if ((errno == ENOENT) || (errno == ENOTDIR))
                rmWatch (*iter);
        }
        else
        {
            closedir( dir);
            lister.listFiles(iter->c_str());
        }
    }
    
    STRIGI_LOG_DEBUG ("strigi.PollingListener.pool", "filesystem access finished")
    
    // de-index files deleted since last polling
    map<string,time_t>::iterator mi = indexedFiles.begin();
    while (mi != indexedFiles.end())
    {
        map<string,time_t>::iterator it = m_toIndex.find(mi->first);
        
        if (it == m_toIndex.end())
        {
            // file has been deleted since last run
            events.push_back (new Event (Event::DELETED, mi->first));
            
            // no more useful, speedup later 
            map<string,time_t>::iterator itrm = mi;
            mi++;
            indexedFiles.erase(itrm);
        }
        else if (mi->second < it->second)
        {
            // file has been updated since last polling
            events.push_back (new Event (Event::UPDATED, mi->first));
            
            // no more useful, speedup later
            m_toIndex.erase(it);
            mi++;
        }
        else
        {
            // file has NOT been changed since last polling, we keep our indexed information
            m_toIndex.erase (it);
            mi++;
        }
    }

    // now m_toIndex contains only files created since the last polling
    for (mi = m_toIndex.begin(); mi != m_toIndex.end(); mi++)
        events.push_back (new Event (Event::CREATED, mi->first));
    
    if (events.size() > 0)
        m_pEventQueue->addEvents (events);

    m_toIndex.clear();
}

bool PollingListener::addWatch (const string& path)
{
    pthread_mutex_lock (&m_mutex);
    
    m_watches.insert (path);
    
    pthread_mutex_unlock (&m_mutex);
    
    STRIGI_LOG_DEBUG ("strigi.PollingListener.addWatch", "successfully added polling watch for " + path)
    
    return true;
}

void PollingListener::rmWatch (const string& path)
{
    pthread_mutex_lock (&m_mutex);
    
    set<string>::iterator iter = m_watches.find (path);
    
    if (iter != m_watches.end())
        m_watches.erase (iter);
    
    pthread_mutex_unlock (&m_mutex);
}

void PollingListener::addWatches (const set<string> &watches)
{
    for (set<string>::iterator iter = watches.begin(); iter != watches.end(); iter++)
    {
        string temp = fixPath (*iter);
        bool match = false;
        
        // try to reduce watches number
        for (set<string>::iterator it = m_watches.begin(); it != m_watches.end(); it++)
        {
            if ((temp.length() >= it->length()) && (temp.find(*it) == 0))
            {
                // temp starts with *it, it means temp is a subdir of *it
                // we don't add it to watches
                match = true;
                break;
            }
            else if ((temp.length() < it->length()) && (it->find(temp) == 0))
            {
                // *it starts with temp, it means *it is a subdir of temp
                // we have to replace *it with temp, begin deleting it
                pthread_mutex_lock (&m_mutex);
                m_watches.erase(it);
                pthread_mutex_unlock (&m_mutex);
                break;
            }
        }
        
        // there's nothing related with temp, add it to watches
        if (!match)
            addWatch (*iter);
    }
}

void PollingListener::setIndexedDirectories (const set<string>& dirs)
{
    pthread_mutex_lock (&m_mutex);
    m_watches.clear();
    pthread_mutex_unlock (&m_mutex);
    
    addWatches (dirs);
}

bool PollingListener::fileCallback(const char* path, uint dirlen, uint len, time_t mtime)
{
    if (strstr(path, "/.")) return true;
    
    string file (path,len);

    (workingPoller->m_toIndex).insert (make_pair(file, mtime));

    return true;
}