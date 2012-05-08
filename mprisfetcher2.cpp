#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif

#include "mprisfetcher2.h"

MprisFetcher2::MprisFetcher2(QObject *parent, const QString &APlayerName = QString::Null()) :
    IMetaDataFetcher(parent)
{
    FPlayerInterface = NULL;

    if (APlayerName.isNull() || APlayerName.isEmpty()) {
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

MprisFetcher2::~MprisFetcher2()
{
    disconnectToBus();
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

void MprisFetcher2::playerPlay()
{
    if (!FPlayerInterface || !FPlayerInterface->isValid())
    {
        return;
    }

    FPlayerInterface->call("PlayPause");
    Q_ASSERT(FPlayerInterface->lastError().type() == QDBusError::NoError);
}

void MprisFetcher2::playerStop()
{
    if (!FPlayerInterface || !FPlayerInterface->isValid())
    {
        return;
    }

    FPlayerInterface->call("Stop");
    Q_ASSERT(FPlayerInterface->lastError().type() == QDBusError::NoError);
}

void MprisFetcher2::playerPrev()
{
    if (!FPlayerInterface || !FPlayerInterface->isValid())
    {
        return;
    }

    FPlayerInterface->call("Previous");
    Q_ASSERT(FPlayerInterface->lastError().type() == QDBusError::NoError);
}

void MprisFetcher2::playerNext()
{
    if (!FPlayerInterface || !FPlayerInterface->isValid())
    {
        return;
    }

    FPlayerInterface->call(QString("Next"));
    Q_ASSERT(FPlayerInterface->lastError().type() == QDBusError::NoError);
}

void MprisFetcher2::updateStatus()
{
    QDBusMessage msg = FPlayerInterface->call("GetMetadata");
    onPropertyChange(msg);
}

void MprisFetcher2::onPlayerNameChange(const QString &AName)
{
    if (AName.isNull() || AName.isEmpty()) {
        return;
    }

    FPlayerName = AName;

    if (FPlayerInterface) {
        disconnectToBus();
        delete FPlayerInterface;
        FPlayerInterface = NULL;
    }

    FPlayerInterface = new QDBusInterface("org.mpris.MediaPlayer2." + FPlayerName, "/Player",
                                  "org.freedesktop.MediaPlayer", QDBusConnection::sessionBus());

    if (FPlayerInterface->isValid()) {
        updateStatus();
        connectToBus();
    }
}

void MprisFetcher2::onPropertyChange(QDBusMessage msg)
{
    QDBusArgument arg = msg.arguments().at(1).value<QDBusArgument>();
    const QVariantMap& map = qdbus_cast<QVariantMap>(arg);

    QVariant v = map.value("Metadata");
    if (v.isValid())
    {
        arg = v.value<QDBusArgument>();
        FTrackInfo = qdbus_cast<QVariantMap>(arg);
        UserTuneData data;

        if (FTrackInfo.contains("xesam:artist")) {
            data.artist =  FTrackInfo["xesam:artist"].toString();
        } else if (FTrackInfo.contains("xesam:composer")) {
            data.artist =  FTrackInfo["xesam:composer"].toString();
        }

        if (FTrackInfo.contains("mpris:length")) {
            data.length = FTrackInfo["mpris:length"].toULongLong() / 1000000;
        }

        if (FTrackInfo.contains("xesam:userRating")) {
        // use rating from 1 to 10
            data.rating = FTrackInfo["xesam:userRating"].toUInt() * 2;
        } else if (FTrackInfo.contains("rating")) {
            data.rating = FTrackInfo["rating"].toUInt() * 2;
        }

        if (FTrackInfo.contains("xesam:album")) {
            data.source =  FTrackInfo["xesam:album"].toString();
        }

        if (FTrackInfo.contains("xesam:title")) {
            data.title =  FTrackInfo["xesam:title"].toString();
        }

        if (FTrackInfo.contains("xesam:trackNumber")) {
            data.track = FTrackInfo["xesam:trackNumber"].toString();
        }

        if (FTrackInfo.contains("xesam:url")) {
            data.uri = FTrackInfo["xesam:url"].toUrl();
        }

        if (data != FUserTuneData) {
            FUserTuneData = data;
            emit trackChanged(data);
        }
#ifndef QT_NO_DEBUG
    } else {
        qWarning() << "invalid Metadata onPropertyChange(QDBusMessage)";
#endif
    }

    v = map.value("PlaybackStatus");
    if (v.isValid())
    {
        QString sStatus = v.toString();
        PlayerStatus pStatus;
        if (sStatus == "Playing") {
            pStatus.Play = PSPlaying;
        } else if (sStatus == "Paused") {
            pStatus.Play = PSPaused;
        } else if (sStatus == "Stopped") {
            pStatus.Play = PSStopped;
        }

        if (FStatus != pStatus) {
            FStatus = pStatus;
            emit statusChanged(FStatus);
        }
#ifndef QT_NO_DEBUG
    } else {
        qWarning() << "invalid PlaybackStatus onPropertyChange(QDBusMessage)";
#endif
    }
}

void MprisFetcher2::onPlayersExistenceChanged(QString name, QString /*empty*/, QString newOwner)
{
    if (!name.startsWith("org.mpris.MediaPlayer2.")) {
        return;
    }
    QString newPlayer = name.replace("org.mpris.MediaPlayer2.","");

    bool player_closed = newOwner.isEmpty();
    if (!player_closed) {
        if (FPlayerName == newPlayer) {
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
