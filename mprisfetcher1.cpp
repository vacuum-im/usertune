#ifndef NO_QT_DEBUG
#  include <QDebug>
#endif

#include "mprisfetcher1.h"

QDBusArgument &operator<< (QDBusArgument &arg, const PlayerStatus &ps)
{
    arg.beginStructure ();
    arg << ps.Play
        << ps.PlayRandom
        << ps.Repeat
        << ps.RepeatPlaylist;
    arg.endStructure ();
    return arg;
}

const QDBusArgument &operator>> (const QDBusArgument &arg, PlayerStatus &ps)
{
    arg.beginStructure();
    arg >> ps.Play
        >> ps.PlayRandom
        >> ps.Repeat
        >> ps.RepeatPlaylist;
    arg.endStructure();
    return arg;
}

MprisFetcher1::MprisFetcher1(QObject *parent, const QString &APlayerName) :
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

    QDBusReply<QVariantMap> metadata = FPlayerInterface->call("GetMetadata");
    FTrackInfo = metadata.value();

   QDBusReply<PlayerStatus> status = FPlayerInterface->call("GetStatus");
    onPlayerStatusChange(status);

    connectToBus();
}

void MprisFetcher1::connectToBus()
{
    QDBusConnection::sessionBus().connect(
                "org.mpris." + FPlayerName,
                "/Player",
                "org.freedesktop.MediaPlayer",
                "TrackChange",
                "a{sv}",
                this,
                SLOT(onTrackChange(QVariantMap)));

    QDBusConnection::sessionBus().connect(
                "org.mpris." + FPlayerName,
                "/Player",
                "org.freedesktop.MediaPlayer",
                "StatusChange",
                "(iiii)",
                this,
                SLOT(onPlayerStatusChange(PlayerStatus)));
    Q_ASSERT(FPlayerInterface->lastError().type() != QDBusError::NoError);
}

void MprisFetcher1::disconnectToBus()
{
    QDBusConnection::sessionBus().disconnect("org.mpris." + FPlayerName,
                                        "/Player",
                                        "org.freedesktop.MediaPlayer",
                                        "StatusChange",
                                        "(iiii)",
                                        this,
                                        SLOT(onPlayerStatusChange(PlayerStatus)));
    QDBusConnection::sessionBus().disconnect("org.mpris." + FPlayerName,
                                        "/Player",
                                        "org.freedesktop.MediaPlayer",
                                        "TrackChange",
                                        "a{sv}",
                                        this,
                                        SLOT(onTrackChange(QVariantMap)));
    Q_ASSERT(FPlayerInterface->lastError().type() != QDBusError::NoError);
}

QStringList MprisFetcher1::getPlayersList()
{
    QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames().value().filter("org.mpris.");
    QStringList ret_list;

    foreach (QString service, services) {
        if (service.startsWith("org.mpris.") && !service.startsWith("org.mpris.MediaPlayer2.")) {
            ret_list << service.replace("org.mpris.","");
        }
    }

    return ret_list;
}

void MprisFetcher1::playerPlay()
{
    if (!FPlayerInterface || !FPlayerInterface->isValid())
    {
        return;
    }

    if (FStatus.Play == PSPaused) {
        FPlayerInterface->call("Play");
    } else {
        FPlayerInterface->call("Pause");
    }
    Q_ASSERT(FPlayerInterface->lastError().type() != QDBusError::NoError);
}

void MprisFetcher1::playerStop()
{
    if (!FPlayerInterface || !FPlayerInterface->isValid())
    {
        return;
    }

    FPlayerInterface->call("Stop");
    Q_ASSERT(FPlayerInterface->lastError().type() != QDBusError::NoError);
}

void MprisFetcher1::playerPrev()
{
    if (!FPlayerInterface || !FPlayerInterface->isValid())
    {
        return;
    }

    FPlayerInterface->call("Prev");
    Q_ASSERT(FPlayerInterface->lastError().type() != QDBusError::NoError);
}

void MprisFetcher1::playerNext()
{
    if (!FPlayerInterface || !FPlayerInterface->isValid())
    {
        return;
    }

    FPlayerInterface->call(QString("Next"));
    Q_ASSERT(FPlayerInterface->lastError().type() != QDBusError::NoError);
}

void MprisFetcher1::onTrackChange(QVariantMap trackInfo)
{
    trackInfo["time"] = secToTime(trackInfo["time"].toInt());
    FTrackInfo = trackInfo;

    emit trackChanged(FTrackInfo);
}

void MprisFetcher1::onPlayerStatusChange(PlayerStatus status)
{
    FStatus.Play = status.Play;
    FStatus.PlayRandom = status.PlayRandom;
    FStatus.Repeat = status.Repeat;
    FStatus.RepeatPlaylist = status.RepeatPlaylist;

    emit statusChanged(FStatus);
}

void MprisFetcher1::onPlayersExistenceChanged(QString name, QString /*empty*/, QString /*newOwner*/)
{
    if (!name.startsWith("org.mpris.") || name.startsWith("org.mpris.MediaPlayer2.")) {
        return;
    }
    QString newPlayer = name.replace("org.mpris.","");

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

    Q_ASSERT(false);
    // пускать сигнал
}
