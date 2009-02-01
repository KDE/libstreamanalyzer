/* This file is part of Strigi Desktop Search
 *
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

#include "xesam_ul_string_scanner.h"

using namespace std;

XesamUlStringScanner::XesamUlStringScanner(string xesam_query)
{
  m_xesam_query = xesam_query;
}

bool XesamUlStringScanner::is_open()
{
  return !m_xesam_query.empty();
}

char XesamUlStringScanner::getCh()
{
  char ret;
  if (m_xesam_query.empty())
    return 0;
  
  ret = m_xesam_query[0];

  if (m_xesam_query.length() == 1)
    m_xesam_query.clear();
  else
    m_xesam_query = m_xesam_query.substr(1);

  return ret;
}

char XesamUlStringScanner::peekCh()
{
  if (!m_xesam_query.empty())
    return m_xesam_query[0];

  return 0;
}

bool XesamUlStringScanner::eof()
{
  return m_xesam_query.empty();
}
