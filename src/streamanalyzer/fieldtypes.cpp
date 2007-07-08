/* This file is part of Strigi Desktop Search
 *
 * Copyright (C) 2007 Jos van den Oever <jos@vandenoever.info>
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
#include "fieldtypes.h"
#include "fieldpropertiesdb.h"
#include <iostream>
using namespace Strigi;
using namespace std;

RegisteredField::RegisteredField(const string& k, const string& t, int m,
        const RegisteredField* p)
        : m_key(k), m_type(t), m_maxoccurs(m), m_parent(p), m_writerdata(0),
	  m_properties(FieldPropertiesDb::db().properties(k)) {
}

const string FieldRegister::floatType = "float";
const string FieldRegister::integerType = "integer";
const string FieldRegister::binaryType = "binary";
const string FieldRegister::stringType = "string";
const string FieldRegister::datetimeType = "datetime";

const string FieldRegister::pathFieldName = "system.location";
const string FieldRegister::encodingFieldName = "content.charset";
const string FieldRegister::mimetypeFieldName = "content.mime_type";
const string FieldRegister::filenameFieldName = "system.file_name";
const string FieldRegister::extensionFieldName = "system.file_extension";
const string FieldRegister::embeddepthFieldName = "system.depth";
const string FieldRegister::mtimeFieldName = "system.last_modified_time";
const string FieldRegister::sizeFieldName = "system.size";

const string FieldRegister::defaultNamespace = "strigi.";

FieldRegister::FieldRegister() {
    pathField = registerField(pathFieldName, stringType, 1, 0);
    encodingField = registerField(encodingFieldName, stringType, 1, 0);
    mimetypeField = registerField(mimetypeFieldName, stringType, -1, 0);
    filenameField = registerField(filenameFieldName, stringType, 1, 0);
    extensionField = registerField(extensionFieldName, stringType, 1, 0);
    embeddepthField = registerField(embeddepthFieldName, integerType, 1, 0);
    mtimeField = registerField(mtimeFieldName, integerType, 1, 0);
    sizeField = registerField(sizeFieldName, integerType, 1, 0);
}

FieldRegister::~FieldRegister() {
    map<string, RegisteredField*>::const_iterator i;
    for (i = m_fields.begin(); i != m_fields.end(); ++i) {
        delete i->second;
    }
}
const RegisteredField*
FieldRegister::registerField(const string& fieldname,
        const string& type, int maxoccurs, const RegisteredField* parent) {
    map<string, RegisteredField*>::iterator i = m_fields.find(fieldname);
    if (i == m_fields.end()) {
        // check with the fieldpropertiesdb
        const FieldProperties& props = FieldPropertiesDb::db().properties(fieldname);
        if (!props.valid()) {
            fprintf(stderr, "WARNING: field \"%s\" is not defined in .fieldproperties files.\n",
		fieldname.c_str());
            // register this property with the propertiesdatabase
            string parentname;
            if (parent) {
                parentname.assign(parent->key());
            }
            FieldPropertiesDb::db().addField(fieldname, type, parentname);
        }
        RegisteredField* f = new RegisteredField(fieldname, type, maxoccurs, parent);
        m_fields[fieldname] = f;
        return f;
    } else {
	return i->second;
    }    
}
