#ifndef MPRISFETCHER1_H
#define MPRISFETCHER1_H

#include "imetadatafetcher.h"

class MprisFetcher1 : public IMetaDataFetcher
{
	Q_OBJECT
public:
	MprisFetcher1(QObject *parent, const QString &APlayerName);
	~MprisFetcher1();

	virtual void updateStatus();

	PlayerStatus getPlayerStatus() const;
	QString getPlayerName() const;

signals:
	void statusChanged(PlayerStatus);
	void trackChanged(UserTuneData);

public slots:
	virtual void playerPlay() const;
	virtual void playerStop() const;
	virtual void playerPrev() const;
	virtual void playerNext() const;
	virtual void onPlayerNameChange(const QString &);

private slots:
	void onTrackChange(QVariantMap);
	void onPlayerStatusChange(PlayerStatus);
	virtual void onPlayersExistenceChanged(QString, QString, QString);

private:
	void connectToBus();
	void disconnectToBus();
	QDBusInterface* createPlayerInterface();

	PlayerStatus FStatus;
};

#endif // MPRISFETCHER1_H
