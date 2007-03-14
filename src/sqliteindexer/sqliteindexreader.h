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
#ifndef SQLITEINDEXREADER_H
#define SQLITEINDEXREADER_H

#include "indexreader.h"
#include <map>

struct sqlite3;
class SqliteIndexManager;

class SqliteIndexReader : public Strigi::IndexReader {
friend class SqliteIndexManager;
private:
    SqliteIndexManager* manager;
    SqliteIndexReader(SqliteIndexManager* m);
    ~SqliteIndexReader();
public:
    int32_t countHits(const Strigi::Query& query);
    std::vector<Strigi::IndexedDocument> query(const Strigi::Query&);
    std::map<std::string, time_t> getFiles(char depth);
    int32_t countDocuments();
    int64_t getDocumentId(const std::string& uri);
    time_t getMTime(int64_t docid);
    std::vector<std::pair<std::string,uint32_t> > getHistogram(
            const std::string& query, const std::string& fieldname,
            const std::string& labeltype);
    std::vector<std::string> getFieldNames();
    int32_t countKeywords(const std::string& keywordprefix,
        const std::vector<std::string>& fieldnames);
    std::vector<std::string> getKeywords(const std::string& keywordmatch,
        const std::vector<std::string>& fieldnames,
        uint32_t max, uint32_t offset);
};

#endif
