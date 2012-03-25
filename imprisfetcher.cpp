#include <QMetaType>

#include "imprisfetcher.h"

IMprisFetcher::IMprisFetcher(QObject *parent) :
    QObject(parent)
{
    QDBusConnection::sessionBus().connect("org.freedesktop.DBus",
                                          "/org/freedesktop/DBus",
                                          "org.freedesktop.DBus",
                                          "NameOwnerChanged",
                                          this,
                                          SLOT(onPlayersExistenceChanged(QString, QString, QString)));
}

IMprisFetcher::~IMprisFetcher()
{
    QDBusConnection::sessionBus().disconnect("org.freedesktop.DBus",
                                           "/org/freedesktop/DBus",
                                           "org.freedesktop.DBus",
                                           "NameOwnerChanged",
                                           this,
                                           SLOT(onPlayersExistenceChanged(QString, QString, QString)));
}

PlayerStatus IMprisFetcher::getPlayerStatus()
{
    if (!FPlayerInterface || !FPlayerInterface->isValid())
    {
        FStatus.Play = PSStopped;
    }

    return FStatus;
}

QVariantMap IMprisFetcher::getMetadata() {
    if (FPlayerInterface && FPlayerInterface->isValid())
    {
        return FTrackInfo;
    } else {
        return QVariantMap();
    }
}

QString IMprisFetcher::secToTime(int secs) {
    int min = 0;
    int sec = secs;

    while (sec > 60) {
        ++min;
        sec -= 60;
    }

    return QString("%1:%2").arg(min).arg(sec,2,10,QChar('0'));
}
