/* This file is part of Strigi Desktop Search
 *
 * Copyright (C) 2007 Jos van den Oever <jos@vandenoever.info>
 * Copyright (C) 2007 Flavio Castelli <flavio.castelli@gmail.com>
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

#include "filelister.h"
#include "strigiconfig.h"
#include "strigi_thread.h"
#include "analyzerconfiguration.h"
#include <set>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include "stgdirent.h" //dirent replacement (includes native if available)

#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif
#include <errno.h>

using namespace std;
using namespace Strigi;

string fixPath (string path)
{
    if ( path.c_str() == NULL || path.length() == 0 )
        return "";

    string temp(path);

#ifdef HAVE_WINDOWS_H
    size_t l= temp.length();
    char* t = (char*)temp.c_str();
    for (size_t i=0;i<l;i++){
        if ( t[i] == '\\' )
            t[i] = '/';
    }
    temp[0] = tolower(temp.at(0));
#endif

    char separator = '/';

    if (temp[temp.length() - 1 ] != separator)
        temp += separator;

    return temp;
}

class FileLister::Private {
public:
    char path[10000];
    STRIGI_MUTEX_DEFINE(mutex);
    DIR** dirs;
    DIR** dirsEnd;
    DIR** curDir;
    int* len;
    int* lenEnd;
    int* curLen;
    time_t mtime;
    struct dirent* subdir;
    struct stat dirstat;
    set<string> listedDirs;
    const AnalyzerConfiguration* const config;

    Private(const AnalyzerConfiguration* ic);
    ~Private();
    int nextFile(string& p, time_t& time) {
        int r;
        STRIGI_MUTEX_LOCK(&mutex);
        r = nextFile();
        if (r > 0) {
            p.assign(path, r);
            time = mtime;
        }
        STRIGI_MUTEX_UNLOCK(&mutex);
        return r;
    }
    void startListing(const std::string&);
    int nextFile();
};
FileLister::Private::Private(
            const AnalyzerConfiguration* ic) :
        config(ic) {
    STRIGI_MUTEX_INIT(&mutex);
    int nOpenDirs = 100;
    dirs = (DIR**)malloc(sizeof(DIR*)*nOpenDirs);
    dirsEnd = dirs + nOpenDirs;
    len = (int*)malloc(sizeof(int)*nOpenDirs);
    lenEnd = len + nOpenDirs;
    curDir = dirs - 1;
}
void
FileLister::Private::startListing(const string& dir){
    listedDirs.clear();
    curDir = dirs;
    curLen = len;
    int len = dir.length();
    *curLen = len;
    strcpy(path, dir.c_str());
    if (len) {
        if (path[len-1] != '/') {
            path[len++] = '/';
            path[len] = 0;
            *curLen = len;
        }
        DIR* d = opendir(path);
        if (d) {
            *curDir = d;
            listedDirs.insert (path);
        } else {
            curDir--;
        }
    } else {
        curDir--;
    }
}
FileLister::Private::~Private() {
    while (curDir >= dirs) {
        if (*curDir) {
            closedir(*curDir);
        }
        curDir--;
    }
    free(dirs);
    free(len);
    STRIGI_MUTEX_DESTROY(&mutex);
}
int
FileLister::Private::nextFile() {

    while (curDir >= dirs) {
        DIR* dir = *curDir;
        int l = *curLen;
        subdir = readdir(dir);
        while (subdir) {
            // skip the directories '.' and '..'
            char c1 = subdir->d_name[0];
            if (c1 == '.') {
                char c2 = subdir->d_name[1];
                if (c2 == '.' || c2 == '\0') {
                    subdir = readdir(dir);
                    continue;
                }
            }
            strcpy(path + l, subdir->d_name);
            int sl = l + strlen(subdir->d_name);
#ifdef _WIN32
            // windows does not have symbolic links, so stat() is fine
            if (stat(path, &dirstat) == 0) {
#else
            if (lstat(path, &dirstat) == 0) {
#endif
                if (S_ISREG(dirstat.st_mode)) {
                    if (config == 0 || config->indexFile(path, path+l)) {
                        mtime = dirstat.st_mtime;
                        return sl;
                    }
                } else if (dirstat.st_mode & S_IFDIR && (config == 0
                        || config->indexDir(path, path+l))) {
                    mtime = dirstat.st_mtime;
                    strcpy(this->path+sl, "/");
                    DIR* d = opendir(path);
                    if (d) {
                        curDir++;
                        curLen++;
                        dir = *curDir = d;
                        l = *curLen = sl+1;
                        listedDirs.insert ( path);
                    }
                }
            }
            subdir = readdir(dir);
        }
        closedir(dir);
        curDir--;
        curLen--;
    }
    return -1;
}
FileLister::FileLister(const AnalyzerConfiguration* ic)
    : p(new Private(ic)) {
}
FileLister::~FileLister() {
    delete p;
}
void
FileLister::startListing(const string& dir) {
    p->startListing(dir);
}
int
FileLister::nextFile(std::string& path, time_t& time) {
    return p->nextFile(path, time);
}
int
FileLister::nextFile(const char*& path, time_t& time) {
    int r = p->nextFile();
    if (r >= 0) {
        time = p->mtime;
        path = p->path;
    }
    return r;
}
void
FileLister::skipTillAfter(const std::string& lastToSkip) {
    int r = p->nextFile();
    while (r >= 0 && p->path != lastToSkip) {
        r = p->nextFile();
    }
}
set<string>&
FileLister::getListedDirs()
{
    return p->listedDirs;
}
