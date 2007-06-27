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
#include "../fileinputstream.h"
#include "../stringstream.h"
#include "../stringterminatedsubstream.h"
#include "inputstreamtests.h"
#include <iostream>

using namespace std;
using namespace Strigi;

int
StringTerminatedSubStreamTest(int, char*[]) {
    founderrors = 0;
    StringInputStream sr("abc");
    StringTerminatedSubStream sub(&sr, "b");
    const char* start;
    int64_t nread = sub.read(start, 10, 10);
    cout << "read " << nread << endl;
/*
    for (int i=0; i<ninputstreamtests; ++i) {
        FileInputStream file("a.zip");
        StringTerminatedSubStream sub(&file, "THEEND");
        charinputstreamtests[i](&sub);
    }*/
    return founderrors;
}

