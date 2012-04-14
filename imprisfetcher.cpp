#include <QMetaType>

#include "imprisfetcher.h"

t_playersList getPlayersList()
{
    QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames().value().filter("org.mpris.");
    t_playersList ret_list;

    QRegExp rx("org.mpris(.MediaPlayer2)?.",Qt::CaseSensitive);

    foreach (QString service, services) {
        if (service.startsWith("org.mpris.MediaPlayer2.")) {
            ret_list.insert(service.replace(rx,"").append("/MPRISv2"), mprisV2);
        }
        else
        {
            ret_list.insert(service.replace(rx,"").append("/MPRISv1"),mprisV1);
        }
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
