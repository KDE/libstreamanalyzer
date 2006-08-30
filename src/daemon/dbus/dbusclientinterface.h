#ifndef DBUSCLIENTINTERFACE_H
#define DBUSCLIENTINTERFACE_H
#include "dbusobjectinterface.h"
#include <map>
class ClientInterface;
class DBusClientInterface : public DBusObjectInterface {
private:
    typedef void (DBusClientInterface::*handlerFunction)
        (DBusMessage* msg, DBusConnection* conn);
    ClientInterface* impl;
    std::map<std::string, handlerFunction> handlers;
    DBusHandlerResult handleCall(DBusConnection* connection, DBusMessage* msg);
    void getStatus(DBusMessage* msg, DBusConnection* conn);
    void isActive(DBusMessage* msg, DBusConnection* conn);
    void setIndexedDirectories(DBusMessage* msg, DBusConnection* conn);
    void getIndexedDirectories(DBusMessage* msg, DBusConnection* conn);
    void stopIndexing(DBusMessage* msg, DBusConnection* conn);
    void getHits(DBusMessage* msg, DBusConnection* conn);
    void startIndexing(DBusMessage* msg, DBusConnection* conn);
    void countHits(DBusMessage* msg, DBusConnection* conn);
    void stopDaemon(DBusMessage* msg, DBusConnection* conn);
public:
    DBusClientInterface(ClientInterface* i);
    ~DBusClientInterface() {}
    std::string getIntrospectionXML();
};
#endif
