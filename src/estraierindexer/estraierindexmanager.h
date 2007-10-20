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
#ifndef ESTRAIERINDEXMANAGER_H
#define ESTRAIERINDEXMANAGER_H

#include "indexmanager.h"
#include <estraier.h>
#include <pthread.h>
#include <string>
#include <map>

/**
 * Manage the database connection between threads.
 * http://www.estraier.org/cvstrac/wiki?p=MultiThreading
 **/

class EstraierIndexReader;
class EstraierIndexWriter;
class EstraierIndexManager : public Strigi::IndexManager {
private:
    pthread_mutex_t dblock;
    static pthread_mutex_t lock;
    std::string dbdir;
    bool writing;
    ESTDB* db;
    std::map<pthread_t, EstraierIndexReader*> readers;
    std::map<pthread_t, EstraierIndexWriter*> writers;

    void closedb();
public:
    explicit EstraierIndexManager(const char* path);
    ~EstraierIndexManager();

    Strigi::IndexReader* indexReader();
    Strigi::IndexWriter* indexWriter();
    ESTDB* ref();
    void deref();

    void deleteIndex();
};

#endif
