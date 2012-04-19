#include <QMetaType>
#include "imprisfetcher.h"

QStringList getPlayersList(const int &ver)
{
    QStringList ret_list;
    QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames().value().filter("org.mpris.");

    switch (ver)
    {
    case mprisV1:
        foreach (QString service, services)
        {
            if (!service.startsWith("org.mpris.MediaPlayer2."))
                ret_list << service.replace("org.mpris.","");
        }
        break;
     case mprisV2:
        foreach (QString service, services)
        {
            if (service.startsWith("org.mpris.MediaPlayer2."))
                ret_list << service.replace("org.mpris.MediaPlayer2.","");
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
