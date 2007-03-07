// generated from dbusclientinterface.h by makecode.pl
#include "dbusclientinterface.h"
#include "dbusmessagereader.h"
#include "dbusmessagewriter.h"
#include <clientinterface.h>
#include <sstream>
DBusClientInterface::DBusClientInterface(ClientInterface* i)
        :DBusObjectInterface("vandenoever.strigi.ClientInterface"), impl(i) {
    handlers["indexFile"] = &DBusClientInterface::indexFile;
    handlers["getStatus"] = &DBusClientInterface::getStatus;
    handlers["getFilters"] = &DBusClientInterface::getFilters;
    handlers["isActive"] = &DBusClientInterface::isActive;
    handlers["getIndexedFiles"] = &DBusClientInterface::getIndexedFiles;
    handlers["setIndexedDirectories"] = &DBusClientInterface::setIndexedDirectories;
    handlers["getFieldNames"] = &DBusClientInterface::getFieldNames;
    handlers["getBackEnds"] = &DBusClientInterface::getBackEnds;
    handlers["setFilters"] = &DBusClientInterface::setFilters;
    handlers["countKeywords"] = &DBusClientInterface::countKeywords;
    handlers["getIndexedDirectories"] = &DBusClientInterface::getIndexedDirectories;
    handlers["getHistogram"] = &DBusClientInterface::getHistogram;
    handlers["stopIndexing"] = &DBusClientInterface::stopIndexing;
    handlers["getKeywords"] = &DBusClientInterface::getKeywords;
    handlers["getHits"] = &DBusClientInterface::getHits;
    handlers["startIndexing"] = &DBusClientInterface::startIndexing;
    handlers["countHits"] = &DBusClientInterface::countHits;
    handlers["stopDaemon"] = &DBusClientInterface::stopDaemon;
}
DBusHandlerResult
DBusClientInterface::handleCall(DBusConnection* connection, DBusMessage* msg){
    std::map<std::string, handlerFunction>::const_iterator h;
    const char* i = getInterfaceName().c_str();
    for (h = handlers.begin(); h != handlers.end(); ++h) {
        if (dbus_message_is_method_call(msg, i, h->first.c_str())) {
            (this->*h->second)(msg, connection);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
std::string
DBusClientInterface::getIntrospectionXML() {
    std::ostringstream xml;
    xml << "  <interface name='"+getInterfaceName()+"'>\n"
    << "    <method name='indexFile'>\n"
    << "      <arg name='path' type='s' direction='in'/>\n"
    << "      <arg name='mtime' type='t' direction='in'/>\n"
    << "      <arg name='content' type='ay' direction='in'/>\n"
    << "    </method>\n"
    << "    <method name='getStatus'>\n"
    << "      <arg name='out' type='a{ss}' direction='out'/>\n"
    << "    </method>\n"
    << "    <method name='getFilters'>\n"
    << "      <arg name='out' type='a(bs)' direction='out'/>\n"
    << "    </method>\n"
    << "    <method name='isActive'>\n"
    << "      <arg name='out' type='b' direction='out'/>\n"
    << "    </method>\n"
    << "    <method name='getIndexedFiles'>\n"
    << "      <arg name='out' type='as' direction='out'/>\n"
    << "    </method>\n"
    << "    <method name='setIndexedDirectories'>\n"
    << "      <arg name='d' type='as' direction='in'/>\n"
    << "      <arg name='out' type='s' direction='out'/>\n"
    << "    </method>\n"
    << "    <method name='getFieldNames'>\n"
    << "      <arg name='out' type='as' direction='out'/>\n"
    << "    </method>\n"
    << "    <method name='getBackEnds'>\n"
    << "      <arg name='out' type='as' direction='out'/>\n"
    << "    </method>\n"
    << "    <method name='setFilters'>\n"
    << "      <arg name='rules' type='a(bs)' direction='in'/>\n"
    << "    </method>\n"
    << "    <method name='countKeywords'>\n"
    << "      <arg name='keywordprefix' type='s' direction='in'/>\n"
    << "      <arg name='fieldnames' type='as' direction='in'/>\n"
    << "      <arg name='out' type='i' direction='out'/>\n"
    << "    </method>\n"
    << "    <method name='getIndexedDirectories'>\n"
    << "      <arg name='out' type='as' direction='out'/>\n"
    << "    </method>\n"
    << "    <method name='getHistogram'>\n"
    << "      <arg name='query' type='s' direction='in'/>\n"
    << "      <arg name='field' type='s' direction='in'/>\n"
    << "      <arg name='labeltype' type='s' direction='in'/>\n"
    << "      <arg name='out' type='a(su)' direction='out'/>\n"
    << "    </method>\n"
    << "    <method name='stopIndexing'>\n"
    << "      <arg name='out' type='s' direction='out'/>\n"
    << "    </method>\n"
    << "    <method name='getKeywords'>\n"
    << "      <arg name='keywordmatch' type='s' direction='in'/>\n"
    << "      <arg name='fieldnames' type='as' direction='in'/>\n"
    << "      <arg name='max' type='u' direction='in'/>\n"
    << "      <arg name='offset' type='u' direction='in'/>\n"
    << "      <arg name='out' type='as' direction='out'/>\n"
    << "    </method>\n"
    << "    <method name='getHits'>\n"
    << "      <arg name='query' type='s' direction='in'/>\n"
    << "      <arg name='max' type='u' direction='in'/>\n"
    << "      <arg name='offset' type='u' direction='in'/>\n"
    << "      <arg name='out' type='a(sdsssxxa{sas})' direction='out'/>\n"
    << "    </method>\n"
    << "    <method name='startIndexing'>\n"
    << "      <arg name='out' type='s' direction='out'/>\n"
    << "    </method>\n"
    << "    <method name='countHits'>\n"
    << "      <arg name='query' type='s' direction='in'/>\n"
    << "      <arg name='out' type='i' direction='out'/>\n"
    << "    </method>\n"
    << "    <method name='stopDaemon'>\n"
    << "      <arg name='out' type='s' direction='out'/>\n"
    << "    </method>\n"
    << "  </interface>\n";
    return xml.str();
}
void
DBusClientInterface::indexFile(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    std::string path;
    uint64_t mtime;
    std::vector<char> content;
    reader >> path >> mtime >> content;
    if (reader.isOk()) {
        impl->indexFile(path,mtime,content);
    }
}
void
DBusClientInterface::getStatus(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    if (reader.isOk()) {
        writer << impl->getStatus();
    }
}
void
DBusClientInterface::getFilters(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    if (reader.isOk()) {
        writer << impl->getFilters();
    }
}
void
DBusClientInterface::isActive(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    if (reader.isOk()) {
        writer << impl->isActive();
    }
}
void
DBusClientInterface::getIndexedFiles(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    if (reader.isOk()) {
        writer << impl->getIndexedFiles();
    }
}
void
DBusClientInterface::setIndexedDirectories(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    std::set<std::string> d;
    reader >> d;
    if (reader.isOk()) {
        writer << impl->setIndexedDirectories(d);
    }
}
void
DBusClientInterface::getFieldNames(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    if (reader.isOk()) {
        writer << impl->getFieldNames();
    }
}
void
DBusClientInterface::getBackEnds(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    if (reader.isOk()) {
        writer << impl->getBackEnds();
    }
}
void
DBusClientInterface::setFilters(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    std::vector<std::pair<bool,std::string> > rules;
    reader >> rules;
    if (reader.isOk()) {
        impl->setFilters(rules);
    }
}
void
DBusClientInterface::countKeywords(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    std::string keywordprefix;
    std::vector<std::string> fieldnames;
    reader >> keywordprefix >> fieldnames;
    if (reader.isOk()) {
        writer << impl->countKeywords(keywordprefix,fieldnames);
    }
}
void
DBusClientInterface::getIndexedDirectories(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    if (reader.isOk()) {
        writer << impl->getIndexedDirectories();
    }
}
void
DBusClientInterface::getHistogram(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    std::string query;
    std::string field;
    std::string labeltype;
    reader >> query >> field >> labeltype;
    if (reader.isOk()) {
        writer << impl->getHistogram(query,field,labeltype);
    }
}
void
DBusClientInterface::stopIndexing(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    if (reader.isOk()) {
        writer << impl->stopIndexing();
    }
}
void
DBusClientInterface::getKeywords(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    std::string keywordmatch;
    std::vector<std::string> fieldnames;
    uint32_t max;
    uint32_t offset;
    reader >> keywordmatch >> fieldnames >> max >> offset;
    if (reader.isOk()) {
        writer << impl->getKeywords(keywordmatch,fieldnames,max,offset);
    }
}
void
DBusClientInterface::getHits(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    std::string query;
    uint32_t max;
    uint32_t offset;
    reader >> query >> max >> offset;
    if (reader.isOk()) {
        writer << impl->getHits(query,max,offset);
    }
}
void
DBusClientInterface::startIndexing(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    if (reader.isOk()) {
        writer << impl->startIndexing();
    }
}
void
DBusClientInterface::countHits(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    std::string query;
    reader >> query;
    if (reader.isOk()) {
        writer << impl->countHits(query);
    }
}
void
DBusClientInterface::stopDaemon(DBusMessage* msg, DBusConnection* conn) {
    DBusMessageReader reader(msg);
    DBusMessageWriter writer(conn, msg);
    if (reader.isOk()) {
        writer << impl->stopDaemon();
    }
}