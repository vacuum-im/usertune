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
	delete FPlayerInterface;
}

void MprisFetcher2::connectToBus()
{
    QDBusConnection::sessionBus().connect(
                ORG_MPRIS_2 + FPlayerName,
                QLatin1String("/org/mpris/MediaPlayer2"),
                QLatin1String("org.freedesktop.DBus.Properties"),
                QLatin1String("PropertiesChanged"),
                this,
                SLOT(onPropertyChange(QDBusMessage)));
}

void MprisFetcher2::disconnectToBus()
{
    QDBusConnection::sessionBus().disconnect(ORG_MPRIS_2 + FPlayerName,
                                             QLatin1String("/org/mpris/MediaPlayer2"),
                                             QLatin1String("org.freedesktop.DBus.Properties"),
                                             QLatin1String("PropertiesChanged"),
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

    FPlayerInterface = new QDBusInterface(ORG_MPRIS_2 + FPlayerName, "/Player",
                                          "org.freedesktop.MediaPlayer", QDBusConnection::sessionBus());
    Q_ASSERT(FPlayerInterface && FPlayerInterface->isValid());
    if (FPlayerInterface->isValid()) {
        updateStatus();
        connectToBus();
    }
}

void MprisFetcher2::onPropertyChange(QDBusMessage msg)
{
	qDebug() << msg;
	QDBusArgument arg = msg.arguments().at(0).value<QDBusArgument>();
    const QVariantMap& map = qdbus_cast<QVariantMap>(arg);

    QVariant v = map.value("Metadata");
	//Q_ASSERT(v.isValid());
    if (v.isValid())
    {
        arg = v.value<QDBusArgument>();
        QVariantMap trackInfo = qdbus_cast<QVariantMap>(arg);
        UserTuneData data;

        if (trackInfo.contains("xesam:artist")) {
            data.artist =  trackInfo["xesam:artist"].toString();
        } else if (trackInfo.contains("xesam:composer")) {
            data.artist =  trackInfo["xesam:composer"].toString();
        }

        if (trackInfo.contains("mpris:length")) {
            data.length = trackInfo["mpris:length"].toULongLong() / 1000000;
        }

        if (trackInfo.contains("xesam:userRating")) {
            // use rating from 1 to 10
            data.rating = trackInfo["xesam:userRating"].toUInt() * 2;
        } else if (trackInfo.contains("rating")) {
            data.rating = trackInfo["rating"].toUInt() * 2;
        }

        if (trackInfo.contains("xesam:album")) {
            data.source =  trackInfo["xesam:album"].toString();
        }

        if (trackInfo.contains("xesam:title")) {
            data.title =  trackInfo["xesam:title"].toString();
        }

        if (trackInfo.contains("xesam:trackNumber")) {
            data.track = trackInfo["xesam:trackNumber"].toString();
        }

        if (trackInfo.contains("xesam:url")) {
            data.uri = trackInfo["xesam:url"].toUrl();
        }

        if (data != FUserTuneData) {
            FUserTuneData = data;
            emit trackChanged(data);
        }
    }

    v = map.value("PlaybackStatus");
    if (v.isValid())
    {
        QString sStatus = v.toString();
        PlayerStatus pStatus;
        if (sStatus == "Playing") {
            pStatus.Play = PlayingStatus::Playing;
        } else if (sStatus == "Paused") {
            pStatus.Play = PlayingStatus::Paused;
        } else if (sStatus == "Stopped") {
            pStatus.Play = PlayingStatus::Stopped;
        }

        if (FStatus != pStatus) {
            FStatus = pStatus;
            emit statusChanged(FStatus);
        }
    }
}

void MprisFetcher2::onPlayersExistenceChanged(QString name, QString /*empty*/, QString newOwner)
{
    if (!name.startsWith(ORG_MPRIS_2)) {
        return;
    }
#if QT_VERSION >= 0x040800
    QStringRef newPlayer = name.midRef(QString(ORG_MPRIS_2).length());
#else
    QString newPlayer = name.mid(QString(ORG_MPRIS_2).length());
#endif

    if (FPlayerName == newPlayer) {
        // player not closed
        if (!newOwner.isEmpty()) {
#if QT_VERSION >= 0x040800
            onPlayerNameChange(newPlayer.toString());
#else
            onPlayerNameChange(newPlayer);
#endif
        } else {
            disconnectToBus();
            delete FPlayerInterface;
            FPlayerInterface = NULL;

            FStatus.Play = PlayingStatus::Stopped;
            emit statusChanged(FStatus);
        }
    }
}
