#ifndef QT_NO_DEBUG
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

MprisFetcher1::MprisFetcher1(QObject *parent, const QString &APlayerName = QString::Null()) :
    IMprisFetcher(parent)
{
    qDBusRegisterMetaType<PlayerStatus>();

    FPlayerInterface = NULL;

    if (APlayerName.isNull() || APlayerName.length() == 0) {
#ifndef QT_NO_DEBUG
        qDebug() << "Player name not set.";
#endif
        return;
    }

    FPlayerName = APlayerName;
    FPlayerInterface = new QDBusInterface("org.mpris." + FPlayerName, "/Player",
                                          "org.freedesktop.MediaPlayer", QDBusConnection::sessionBus());

    if (FPlayerInterface->lastError().type() != QDBusError::NoError) {
#ifndef QT_NO_DEBUG
        qWarning() << QDBusError::errorString(FPlayerInterface->lastError().type());
#endif
        return;
    }

    updateStatus();
    connectToBus();
}

MprisFetcher1::~MprisFetcher1()
{
    disconnectToBus();
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

void MprisFetcher1::updateStatus()
{
    QDBusReply<PlayerStatus> status = FPlayerInterface->call("GetStatus");
    onPlayerStatusChange(status);

    if (FStatus.Play != PSStopped) {
        QDBusReply<QVariantMap> metadata = FPlayerInterface->call("GetMetadata");
        if (metadata.isValid()) {
            FTrackInfo = metadata.value();
            onTrackChange(FTrackInfo);
#ifndef QT_NO_DEBUG
        } else {
            qWarning() << "Invalid metadata updateStatus()";
#endif
        }
    }
}

void MprisFetcher1::onPlayerNameChange(const QString &AName)
{
    FPlayerName = AName;

    if (FPlayerInterface) {
        disconnectToBus();
        delete FPlayerInterface;
        FPlayerInterface = NULL;
    }

    FPlayerInterface = new QDBusInterface("org.mpris." + FPlayerName, "/Player",
                                  "org.freedesktop.MediaPlayer", QDBusConnection::sessionBus());
    if (FPlayerInterface->isValid()) {
        updateStatus();
        connectToBus();
    }
}

void MprisFetcher1::onTrackChange(QVariantMap trackInfo)
{
    qDebug() << "Track changed";
    UserTuneData data;
    if (trackInfo.contains("artist")) {
        data.artist =  trackInfo["artist"].toString();
    }
    if (trackInfo.contains("time")) {
        data.length = trackInfo["time"].toInt();
    }
    if (trackInfo.contains("rating")) {
        data.rating = trackInfo["artist"].toInt();
    }
    if (trackInfo.contains("album")) {
        data.source =  trackInfo["album"].toString();
    }
    if (trackInfo.contains("title")) {
        data.title =  trackInfo["title"].toString();
    }
    if (trackInfo.contains("tracknumber")) {
        data.track = trackInfo["tracknumber"].toString();
    }
    if (trackInfo.contains("location")) {
        data.uri = trackInfo["location"].toUrl();
    }

    trackInfo["time"] = secToTime(trackInfo["time"].toInt());
    FTrackInfo = trackInfo;

    emit trackChanged(data);
}

void MprisFetcher1::onPlayerStatusChange(PlayerStatus status)
{
    if (FStatus != status) {
        FStatus = status;

        emit statusChanged(FStatus);
    }
}

void MprisFetcher1::onPlayersExistenceChanged(QString name, QString /*empty*/, QString newOwner)
{
    if (!name.startsWith("org.mpris.") || name.startsWith("org.mpris.MediaPlayer2.")) {
        return;
    }
    QString newPlayer = name.replace("org.mpris.","");

    bool player_closed = newOwner.isEmpty();
    if (!player_closed) {
        if (FPlayerName == newPlayer) {
            qDebug() << newPlayer;
            onPlayerNameChange(newPlayer);
        }
    } else {
        if (FPlayerName == newPlayer)
        {
            disconnectToBus();
            delete FPlayerInterface;
            FPlayerInterface = NULL;

            FStatus.Play = PSStopped;
            emit statusChanged(FStatus);
        }
    }
}
