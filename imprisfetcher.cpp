#include <QMetaType>
#include "imprisfetcher.h"

QStringList getPlayersList(const int &ver)
{
    QStringList ret_list;
    QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames().value().filter("org.mpris.");
    QStringList::const_iterator it = services.constBegin();
    switch (ver)
    {
    case mprisV1:
        for ( ; it != services.constEnd(); ++it)
        {
            if (!it->startsWith("org.mpris.MediaPlayer2."))
            ret_list << *it;
        }
        break;
     case mprisV2:
        for ( ; it != services.constEnd(); ++it)
        {
            if (it->startsWith("org.mpris.MediaPlayer2."))
            ret_list << *it;
        }
        break;
    default:
        break;
    }

    return ret_list;
}

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

QVariantMap IMprisFetcher::getMetadata()
{
    if (FPlayerInterface && FPlayerInterface->isValid())
    {
        return FTrackInfo;
    } else {
        return QVariantMap();
    }
}
