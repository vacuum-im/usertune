#ifndef NO_QT_DEBUG
#  include <QDebug>
#endif

#include "mprisfetcher2.h"

MprisFetcher2::MprisFetcher2(QObject *parent, const QString &APlayerName) :
    IMprisFetcher(parent),
    FPlayerName(APlayerName)
{
    qDBusRegisterMetaType<PlayerStatus>();

    FPlayerInterface = new QDBusInterface("org.mpris." + FPlayerName, "/Player",
                                          "org.freedesktop.MediaPlayer", QDBusConnection::sessionBus());

    if (FPlayerInterface->lastError().type() != QDBusError::NoError) {
#ifndef NO_QT_DEBUG
        qWarning() << QDBusError::errorString(FPlayerInterface->lastError().type());
#endif
        return;
    }

    QDBusMessage msg = FPlayerInterface->call("GetMetadata");
    onPropertyChange(msg);

    connectToBus();
}

void MprisFetcher2::connectToBus()
{
    QDBusConnection::sessionBus().connect(
                "org.mpris.MediaPlayer2." + FPlayerName,
                "/org/mpris/MediaPlayer2",
                "org.freedesktop.DBus.Properties",
                "PropertiesChanged",
                this,
                SLOT(onPropertyChange(QDBusMessage)));
}

void MprisFetcher2::disconnectToBus()
{
    QDBusConnection::sessionBus().disconnect("org.mpris.MediaPlayer2." + FPlayerName,
                                             "/org/mpris/MediaPlayer2",
                                             "org.freedesktop.DBus.Properties",
                                             "PropertiesChanged",
                                             this,
                                             SLOT(onTrackChange(QVariantMap)));
}

QStringList MprisFetcher2::getPlayersList()
{
    QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames().value().filter("org.mpris.MediaPlayer2.");
    QStringList ret_list;

    foreach (QString service, services) {
        if (service.startsWith("org.mpris.MediaPlayer2.")) {
            ret_list << service.replace("org.mpris.MediaPlayer2.","");
        }
    }

    return ret_list;
}

void MprisFetcher2::playerPlay()
{
    if (!FPlayerInterface || !FPlayerInterface->isValid())
    {
        return;
    }

    FPlayerInterface->call("PlayPause");
    Q_ASSERT(FPlayerInterface->lastError().type() != QDBusError::NoError);
}

void MprisFetcher2::playerStop()
{
    if (!FPlayerInterface || !FPlayerInterface->isValid())
    {
        return;
    }

    FPlayerInterface->call("Stop");
    Q_ASSERT(FPlayerInterface->lastError().type() != QDBusError::NoError);
}

void MprisFetcher2::playerPrev()
{
    if (!FPlayerInterface || !FPlayerInterface->isValid())
    {
        return;
    }

    FPlayerInterface->call("Previous");
    Q_ASSERT(FPlayerInterface->lastError().type() != QDBusError::NoError);
}

void MprisFetcher2::playerNext()
{
    if (!FPlayerInterface || !FPlayerInterface->isValid())
    {
        return;
    }

    FPlayerInterface->call(QString("Next"));
    Q_ASSERT(FPlayerInterface->lastError().type() != QDBusError::NoError);
}

void MprisFetcher2::onPropertyChange(QDBusMessage msg)
{

}

void MprisFetcher2::onPlayersExistenceChanged(QString name, QString /*empty*/, QString /*newOwner*/)
{
    if (!name.startsWith("org.mpris.MediaPlayer2.")) {
        return;
    }
    QString newPlayer = name.replace("org.mpris.MediaPlayer2.","");

    if (!newOwner.isEmpty()) {
        if (playerName.compare(newPlayer)) {
            playerChange(newPlayer);
        }
    } else if (newOwner.isEmpty ()) {
        if (playerName.compare(newPlayer))
        {
            disconnectToBus();
            delete FPlayerInterface;
        }
    }

    Q_ASSERT_X(false, "need to send signal");
}
