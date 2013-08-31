#ifndef IMPRISFETCHER_H
#define IMPRISFETCHER_H

#include <QObject>
#include <QtDBus/QtDBus>

#include "usertunetypes.h"

#define ORG_MPRIS_1 QLatin1String("org.mpris.")
#define ORG_MPRIS_2 QLatin1String("org.mpris.MediaPlayer2.")

QStringList getPlayersList(const int &ver);

class IMetaDataFetcher : public QObject
{
	Q_OBJECT
public:
	IMetaDataFetcher(QObject *parent);
	virtual ~IMetaDataFetcher();

	virtual void updateStatus() = 0;
	QString getPlayerName() const { return FPlayerName; }

signals:
	void statusChanged(PlayerStatus);
	void trackChanged(UserTuneData);

public slots:
	virtual void playerPlay() const = 0;
	virtual void playerStop() const = 0;
	virtual void playerPrev() const = 0;
	virtual void playerNext() const = 0;
	virtual void onPlayerNameChange(const QString &) = 0;

protected slots:
	virtual void onPlayersExistenceChanged(QString, QString, QString) = 0;

protected:
	QString FPlayerName;
	QDBusInterface *FPlayerInterface;
};

Q_DECLARE_METATYPE(PlayerStatus)

#endif // IMPRISFETCHER_H
