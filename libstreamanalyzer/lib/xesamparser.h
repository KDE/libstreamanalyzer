/* This file is part of Strigi Desktop Search
 *
 * Copyright (C) 2010 Klarälvdalens Datakonsult AB,
 *     a KDAB Group company, info@kdab.net,
 *     author Tobias Koenig <tokoe@kde.org>
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

#ifndef STRIGI_XESAMPARSER_H
#define STRIGI_XESAMPARSER_H

#include <string>

#include "strigi/query.h"

class SimpleNode;
class XMLStream;

namespace Strigi {

class XesamParser {
public:
    XesamParser();
    ~XesamParser();

    bool buildQuery(const std::string& queryString, Query &query);
    std::string errorMessage() const;

private:
    bool parseQuery(Query &query);

    bool parseSelectorClause(Query &query, Query::Type type);
    bool parseEquals(Query &query);
    bool parseContains(Query &query);
    bool parseLessThan(Query &query);
    bool parseLessThanEquals(Query &query);
    bool parseGreaterThan(Query &query);
    bool parseGreaterThanEquals(Query &query);
    bool parseStartsWith(Query &query);
    bool parseInSet(Query &query);
    bool parseFullText(Query &query);

    bool parseCollectorClause(Query &query, Query::Type type);
    bool parseAnd(Query &query);
    bool parseOr(Query &query);

    bool parseString(Query &query);
    bool parseInteger(Query &query);
    bool parseDate(Query &query);
    bool parseFloat(Query &query);
    bool parseBoolean(Query &query);

    XMLStream *m_xmlStream;
    std::string m_errorMessage;
};

}

#endif
