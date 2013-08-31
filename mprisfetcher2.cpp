#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif

#include "mprisfetcher2.h"

MprisFetcher2::MprisFetcher2(QObject *parent, const QString &APlayerName = QString::Null()) :
	IMetaDataFetcher(parent)
{
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

MprisFetcher2::~MprisFetcher2()
{
	disconnectToBus();
	delete FPlayerInterface;
}

void MprisFetcher2::connectToBus()
{
	QDBusConnection::sessionBus().connect(ORG_MPRIS_2 + FPlayerName,
										  QLatin1String("/org/mpris/MediaPlayer2"),
										  QLatin1String("org.freedesktop.DBus.Properties"),
										  QLatin1String("PropertiesChanged"),
										  QLatin1String("sa{sv}as"),
										  this,
										  SLOT(onPropertyChange(QDBusMessage)));
}

void MprisFetcher2::disconnectToBus()
{
	QDBusConnection::sessionBus().disconnect(ORG_MPRIS_2 + FPlayerName,
											 QLatin1String("/org/mpris/MediaPlayer2"),
											 QLatin1String("org.freedesktop.DBus.Properties"),
											 QLatin1String("PropertiesChanged"),
											 QLatin1String("sa{sv}as"),
											 this,
											 SLOT(onPropertyChange(QDBusMessage)));
}

QDBusInterface* MprisFetcher2::createPlayerInterface()
{
	return new QDBusInterface(ORG_MPRIS_2 + FPlayerName,
							  QLatin1String("/org/mpris/MediaPlayer2"),
							  QLatin1String("org.mpris.MediaPlayer2.Player"),
							  QDBusConnection::sessionBus(),
							  this);
}

void MprisFetcher2::playerPlay() const
{
	if (!FPlayerInterface || !FPlayerInterface->isValid()) {
		return;
	}

	FPlayerInterface->call(QLatin1String("PlayPause"));
	Q_ASSERT(FPlayerInterface->lastError().type() == QDBusError::NoError);
}

void MprisFetcher2::playerStop() const
{
	if (!FPlayerInterface || !FPlayerInterface->isValid()) {
		return;
	}

	FPlayerInterface->call(QLatin1String("Stop"));
	Q_ASSERT(FPlayerInterface->lastError().type() == QDBusError::NoError);
}

void MprisFetcher2::playerPrev() const
{
	if (!FPlayerInterface || !FPlayerInterface->isValid()) {
		return;
	}

	FPlayerInterface->call(QLatin1String("Previous"));
	Q_ASSERT(FPlayerInterface->lastError().type() == QDBusError::NoError);
}

void MprisFetcher2::playerNext() const
{
	if (!FPlayerInterface || !FPlayerInterface->isValid()) {
		return;
	}

	FPlayerInterface->call(QLatin1String("Next"));
	Q_ASSERT(FPlayerInterface->lastError().type() == QDBusError::NoError);
}

void MprisFetcher2::updateStatus()
{
	if (FPlayerInterface && FPlayerInterface->isValid()) {
		QDBusInterface interface(ORG_MPRIS_2 + FPlayerName,
								 QLatin1String("/org/mpris/MediaPlayer2"),
								 QLatin1String("org.freedesktop.DBus.Properties"),
								 QDBusConnection::sessionBus(),
								 this);

		QDBusReply<QVariant> trackInfo = interface.call(QLatin1String("Get"),
														QLatin1String("org.mpris.MediaPlayer2.Player"),
														QLatin1String("Metadata"));
		if (trackInfo.isValid()) {
			QDBusArgument argument = static_cast<QVariant>(trackInfo.value()).value<QDBusArgument>();
			parseTrackInfo(qdbus_cast<QVariantMap>(argument));
		}

		QDBusReply<QVariant> playbackStatus = interface.call(QLatin1String("Get"),
															QLatin1String("org.mpris.MediaPlayer2.Player"),
															QLatin1String("PlaybackStatus"));
		if (playbackStatus.isValid()) {
			parsePlaybackStatus(playbackStatus.value().toString());
		}
	}
	Q_ASSERT(FPlayerInterface->lastError().type() == QDBusError::NoError);
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
		FPlayerInterface = nullptr;
	}

	FPlayerInterface = createPlayerInterface();
	Q_ASSERT(FPlayerInterface && FPlayerInterface->isValid());
	if (FPlayerInterface->isValid()) {
		updateStatus();
		connectToBus();
	}
}

void MprisFetcher2::parseTrackInfo(const QVariantMap &ATrackInfo)
{
	UserTuneData data;

	if (ATrackInfo.contains(QLatin1String("xesam:artist"))) {
		data.artist = ATrackInfo[QLatin1String("xesam:artist")].toString();
	} else if (ATrackInfo.contains(QLatin1String("xesam:composer"))) {
		data.artist = ATrackInfo[QLatin1String("xesam:composer")].toString();
	}

	if (ATrackInfo.contains(QLatin1String("mpris:length"))) {
		data.length = ATrackInfo[QLatin1String("mpris:length")].toULongLong() / 1000000;
	}

	if (ATrackInfo.contains(QLatin1String("xesam:userRating"))) {
		// use rating from 1 to 10
		data.rating = ATrackInfo[QLatin1String("xesam:userRating")].toUInt() * 2;
	} else if (ATrackInfo.contains(QLatin1String("rating"))) {
		data.rating = ATrackInfo[QLatin1String("rating")].toUInt() * 2;
	}

	if (ATrackInfo.contains(QLatin1String("xesam:album"))) {
		data.source = ATrackInfo[QLatin1String("xesam:album")].toString();
	}

	if (ATrackInfo.contains(QLatin1String("xesam:title"))) {
		data.title = ATrackInfo[QLatin1String("xesam:title")].toString();
	}

	if (ATrackInfo.contains(QLatin1String("xesam:trackNumber"))) {
		data.track = ATrackInfo[QLatin1String("xesam:trackNumber")].toString();
	}

	if (ATrackInfo.contains(QLatin1String("xesam:url"))) {
		data.uri = ATrackInfo[QLatin1String("xesam:url")].toUrl();
	}

	emit trackChanged(data);
}

void MprisFetcher2::parsePlaybackStatus(const QString &AStatus)
{
	PlayerStatus pStatus;

	if (AStatus == QLatin1String("Playing")) {
		pStatus.Play = PlaybackStatus::Playing;
	} else if (AStatus == QLatin1String("Paused")) {
		pStatus.Play = PlaybackStatus::Paused;
	} else if (AStatus == QLatin1String("Stopped")) {
		pStatus.Play = PlaybackStatus::Stopped;
	}

	emit statusChanged(pStatus);
}

void MprisFetcher2::onPropertyChange(QDBusMessage AMsg)
{
	QVariantMap map;

	const QList<QVariant> arguments = AMsg.arguments();
	// find argument in received message
	for (QList<QVariant>::const_iterator iter = arguments.constBegin(); iter != arguments.constEnd(); ++iter) {
		if (iter->canConvert<QDBusArgument>()) {
			//QVariant -> QDBusArgument -> QVariantMap
			map.unite(qdbus_cast<QVariantMap>(iter->value<QDBusArgument>()));
		}
	}

	if (map.contains(QLatin1String("Metadata"))) {
		// QVariantMap -> QVariant -> QDBusArgument -> QVariantMap
		const QVariantMap trackInfo = qdbus_cast<QVariantMap>(map[QLatin1String("Metadata")]);
		parseTrackInfo(trackInfo);
	}

	if (map.contains(QLatin1String("PlaybackStatus"))) {
		const QString sStatus = map.value(QLatin1String("PlaybackStatus")).toString();
		parsePlaybackStatus(sStatus);
	}
}

void MprisFetcher2::onPlayersExistenceChanged(QString AName, QString /*empty*/, QString ANewOwner)
{
	if (!AName.startsWith(ORG_MPRIS_2)) {
		return;
	}
#if QT_VERSION >= 0x040800
	const QStringRef thisPlayer = AName.midRef(QString(ORG_MPRIS_2).length());
#else
	const QString thisPlayer = AName.mid(QString(ORG_MPRIS_2).length());
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

			PlayerStatus status;
			status.Play = PlaybackStatus::Stopped;
			emit statusChanged(status);
		}
	}
}
