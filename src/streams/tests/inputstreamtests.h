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
#ifndef INPUTSTREAMTESTS
#define INPUTSTREAMTESTS
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

namespace jstreams {
    template <class T> class StreamBase;
    class SubStreamProvider;
}

template <class T>
void inputStreamTest1(jstreams::StreamBase<T>* stream);

template <class T>
void inputStreamTest2(jstreams::StreamBase<T>* stream);

void subStreamProviderTest1(jstreams::SubStreamProvider* stream);

extern int ninputstreamtests;
extern void (*charinputstreamtests[])(jstreams::StreamBase<char>*);
extern void (*wcharinputstreamtests[])(jstreams::StreamBase<wchar_t>*);

extern int nstreamprovidertests;
extern void (*streamprovidertests[])(jstreams::SubStreamProvider*);

extern int founderrors;
#define VERIFY(TESTBOOL) if (!(TESTBOOL)) {\
	fprintf(stderr, "test '%s' failed at\n\t%s:%i\n", \
		#TESTBOOL, __FILE__, __LINE__); \
	founderrors++; \
}


#endif
