#ifndef STRIGITYPES_H
#define STRIGITYPES_H

#include <QtCore/QMap>
#include <QtCore/QMetaType>
#include <QtDBus/QtDBus>

typedef QMap<QString, QString> StringStringMap;
Q_DECLARE_METATYPE(StringStringMap)
typedef QMultiMap<int, QString> IntegerStringMultiMap;
Q_DECLARE_METATYPE(IntegerStringMultiMap)

// sdsssxxa{sas}
struct StrigiHit {
   QString uri;
   double score;
   QString fragment;
   QString mimetype;
   QString sha1;
   qint64 size;
   qint64 mtime;
   QMap<QString, QStringList> properties;
};
Q_DECLARE_METATYPE(StrigiHit)
Q_DECLARE_METATYPE(QList<StrigiHit>)

QDBusArgument& operator<<(QDBusArgument &a, const StrigiHit &hit);
const QDBusArgument& operator>>(const QDBusArgument &a, StrigiHit& hit);

#endif