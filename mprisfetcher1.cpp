#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif

#include "mprisfetcher1.h"

QDBusArgument &operator<< (QDBusArgument &arg, const PlayerStatus &ps)
{
	arg.beginStructure();
	arg << ps.Play
		<< ps.PlayRandom
		<< ps.Repeat
		<< ps.RepeatPlaylist;
	arg.endStructure();
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
	IMetaDataFetcher(parent)
{
	qDBusRegisterMetaType<PlayerStatus>();

	FPlayerInterface = nullptr;

	if (APlayerName.isNull() || APlayerName.isEmpty()) {
#ifndef QT_NO_DEBUG
		qDebug() << "Player name not set.";
#endif
		return;
	}

	FPlayerName = APlayerName;
	FPlayerInterface = createPlayerInterface();

	if (FPlayerInterface->lastError().type() != QDBusError::NoError) {
#ifndef QT_NO_DEBUG
		qWarning() << QDBusError::errorString(FPlayerInterface->lastError().type());
#endif
		return;
	}

	connectToBus();
}

MprisFetcher1::~MprisFetcher1()
{
	disconnectToBus();
	delete FPlayerInterface;
}

void MprisFetcher1::connectToBus()
{
	QDBusConnection::sessionBus().connect(ORG_MPRIS_1 + FPlayerName,
										  "/Player",
										  "org.freedesktop.MediaPlayer",
										  "TrackChange",
										  "a{sv}",
										  this,
										  SLOT(onTrackChange(QVariantMap)));

	QDBusConnection::sessionBus().connect(ORG_MPRIS_1 + FPlayerName,
										  "/Player",
										  "org.freedesktop.MediaPlayer",
										  "StatusChange",
										  "(iiii)",
										  this,
										  SLOT(onPlayerStatusChange(PlayerStatus)));

	Q_ASSERT(FPlayerInterface->lastError().type() == QDBusError::NoError);
}

void MprisFetcher1::disconnectToBus()
{
	QDBusConnection::sessionBus().disconnect(ORG_MPRIS_1 + FPlayerName,
											 "/Player",
											 "org.freedesktop.MediaPlayer",
											 "StatusChange",
											 "(iiii)",
											 this,
											 SLOT(onPlayerStatusChange(PlayerStatus)));

	QDBusConnection::sessionBus().disconnect(ORG_MPRIS_1 + FPlayerName,
											 "/Player",
											 "org.freedesktop.MediaPlayer",
											 "TrackChange",
											 "a{sv}",
											 this,
											 SLOT(onTrackChange(QVariantMap)));

	Q_ASSERT(FPlayerInterface->lastError().type() == QDBusError::NoError);
}

QDBusInterface* MprisFetcher1::createPlayerInterface()
{
	return new QDBusInterface(ORG_MPRIS_1 + FPlayerName,
							  QLatin1String("/Player"),
							  QLatin1String("org.freedesktop.MediaPlayer"),
							  QDBusConnection::sessionBus());
}

void MprisFetcher1::playerPlay()
{
	if (!FPlayerInterface || !FPlayerInterface->isValid()) {
		return;
	}

	if (FStatus.Play == PlaybackStatus::Paused) {
		FPlayerInterface->call("Play");
	} else {
		FPlayerInterface->call("Pause");
	}
	Q_ASSERT(FPlayerInterface->lastError().type() == QDBusError::NoError);
}

void MprisFetcher1::playerStop()
{
	if (!FPlayerInterface || !FPlayerInterface->isValid()) {
		return;
	}

	FPlayerInterface->call("Stop");
	Q_ASSERT(FPlayerInterface->lastError().type() == QDBusError::NoError);
}

void MprisFetcher1::playerPrev()
{
	if (!FPlayerInterface || !FPlayerInterface->isValid()) {
		return;
	}

	FPlayerInterface->call("Prev");
	Q_ASSERT(FPlayerInterface->lastError().type() == QDBusError::NoError);
}

void MprisFetcher1::playerNext()
{
	if (!FPlayerInterface || !FPlayerInterface->isValid()) {
		return;
	}

	FPlayerInterface->call(QString("Next"));
	Q_ASSERT(FPlayerInterface->lastError().type() == QDBusError::NoError);
}

void MprisFetcher1::updateStatus()
{
	if (!FPlayerInterface || !FPlayerInterface->isValid()) {
		return;
	}

	QDBusReply<PlayerStatus> status = FPlayerInterface->call("GetStatus");
	Q_ASSERT(status.isValid());
	if (status.isValid()) {
		onPlayerStatusChange(status.value());

		if (FStatus.Play != PlaybackStatus::Stopped) {
			QDBusReply<QVariantMap> metadata = FPlayerInterface->call("GetMetadata");
			Q_ASSERT(metadata.isValid());
			if (metadata.isValid()) {
				onTrackChange(metadata.value());
			}
		}
	}
}

void MprisFetcher1::onPlayerNameChange(const QString &AName)
{
	if (AName.isNull() || AName.isEmpty()) {
		return;
	}

	FPlayerName = AName;

	if (FPlayerInterface) {
		disconnectToBus();
		delete FPlayerInterface;
		FPlayerInterface = nullptr;
	}

	FPlayerInterface = createPlayerInterface();
	Q_ASSERT(FPlayerInterface && FPlayerInterface->isValid());
	if (FPlayerInterface && FPlayerInterface->isValid()) {
		updateStatus();
		connectToBus();
	}
}

void MprisFetcher1::onTrackChange(QVariantMap trackInfo)
{
	UserTuneData data;
	if (trackInfo.contains("artist")) {
		data.artist =  trackInfo["artist"].toString();
	}
	if (trackInfo.contains("time")) {
		data.length = trackInfo["time"].toUInt();
	}
	if (trackInfo.contains("rating")) {
		// use rating from 1 to 10
		data.rating = trackInfo["rating"].toUInt() * 2;
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

	emit trackChanged(data);
}

void MprisFetcher1::onPlayerStatusChange(PlayerStatus status)
{
	FStatus = status;

	emit statusChanged(status);
}

void MprisFetcher1::onPlayersExistenceChanged(QString AName, QString /*empty*/, QString ANewOwner)
{
	// TODO: mprisfetcher1 not needed think of ORG_MPRIS_2
	if (!AName.startsWith(ORG_MPRIS_1) || AName.startsWith(ORG_MPRIS_2)) {
		return;
	}

#if QT_VERSION >= 0x040800
	const QStringRef thisPlayer = AName.midRef(QString(ORG_MPRIS_1).length());
#else
	const QString thisPlayer = AName.mid(QString(ORG_MPRIS_1).length());
#endif

	if (thisPlayer == FPlayerName) {
		// player did not closed and did not opened before
		if (!ANewOwner.isEmpty() && !FPlayerInterface) {
#if QT_VERSION >= 0x040800
			onPlayerNameChange(thisPlayer.toString());
#else
			onPlayerNameChange(thisPlayer);
#endif
		} else {
			disconnectToBus();
			delete FPlayerInterface;
			FPlayerInterface = nullptr;

			FStatus.Play = PlaybackStatus::Stopped;
			emit statusChanged(FStatus);
		}
	}
}
