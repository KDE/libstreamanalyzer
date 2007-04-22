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
#include "strigiconfig.h"
#include "diranalyzer.h"
#include "analyzerconfiguration.h"

int
main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s [indexdir] [dir-to-index]\n", argv[0]);
        return -1;
    }
    std::string file = argv[1];
    file += "/sqlite.db";

    Strigi::AnalyzerConfiguration ic;
    Indexer indexer(file.c_str(), ic);
    indexer.index(argv[2]);
    return 0;
}
