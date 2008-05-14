/* This file is part of Strigi Desktop Search
 *
 * Copyright (C) 2006 Ben van Klinken <bvklinken@gmail.com>
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

#include "indexreadertests.h"
#include <stdio.h>
#include <strigi/strigiconfig.h>
#include "indexreader.h"
using namespace Strigi;

class IndexReaderTester {
private:
    IndexReader* reader;
public:
    IndexReaderTester(IndexReader* r) :reader(r) {}
    int files(char depth) {
        VERIFY(reader);
        if (reader == 0) return 1;
        return 0;
    }
};

IndexReaderTests::IndexReaderTests(Strigi::IndexReader* w)
    :tester (new IndexReaderTester(w)) {
}
IndexReaderTests::~IndexReaderTests() {
    delete tester;
}

int
IndexReaderTests::testAll() {
    int n = 0;
    n += tester->files(0);
    return n;
}
