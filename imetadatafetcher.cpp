#include <QMetaType>
#include "imetadatafetcher.h"

QStringList getPlayersList(const int &ver)
{
    QStringList ret_list;
#ifdef Q_WS_X11
    QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames().value().filter("org.mpris.");
#elif Q_WS_WIN

#endif

    switch (ver)
    {
#ifdef  Q_WS_X11
    case FetcherVer::mprisV1:
        foreach (QString service, services)
        {
            if (!service.startsWith("org.mpris.MediaPlayer2."))
                ret_list << service.replace("org.mpris.","");
        }
        break;
     case FetcherVer::mprisV2:
        foreach (QString service, services)
        {
            if (service.startsWith("org.mpris.MediaPlayer2."))
                ret_list << service.replace("org.mpris.MediaPlayer2.","");
        }
        break;
#elif Q_WS_WIN

#endif
    default:
        break;
    }

    return ret_list;
}

IMetaDataFetcher::IMetaDataFetcher(QObject *parent) :
    QObject(parent)
{
#ifdef  Q_WS_X11
    QDBusConnection::sessionBus().connect("org.freedesktop.DBus",
                                          "/org/freedesktop/DBus",
                                          "org.freedesktop.DBus",
                                          "NameOwnerChanged",
                                          this,
                                          SLOT(onPlayersExistenceChanged(QString, QString, QString)));
#endif
}

IMetaDataFetcher::~IMetaDataFetcher()
{
#ifdef  Q_WS_X11
    QDBusConnection::sessionBus().disconnect("org.freedesktop.DBus",
                                           "/org/freedesktop/DBus",
                                           "org.freedesktop.DBus",
                                           "NameOwnerChanged",
                                           this,
                                           SLOT(onPlayersExistenceChanged(QString, QString, QString)));
#endif
}

PlayerStatus IMetaDataFetcher::getPlayerStatus()
{
    if (!FPlayerInterface || !FPlayerInterface->isValid())
    {
        FStatus.Play = PSStopped;
    }

    return FStatus;
}

QVariantMap IMetaDataFetcher::getMetadata()
{
    if (FPlayerInterface && FPlayerInterface->isValid())
    {
        return FTrackInfo;
    } else {
        return QVariantMap();
    }
}
