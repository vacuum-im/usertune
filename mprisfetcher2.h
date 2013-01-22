#ifndef MPRISFETCHER2_H
#define MPRISFETCHER2_H

#include "imetadatafetcher.h"

class MprisFetcher2 : public IMetaDataFetcher
{
	Q_OBJECT
public:
	MprisFetcher2(QObject *parent, const QString &APlayerName);
	~MprisFetcher2();

	virtual void updateStatus();

	PlayerStatus getPlayerStatus() const;
	QString getPlayerName() const;

signals:
	void statusChanged(PlayerStatus);
	void trackChanged(UserTuneData);

public slots:
	virtual void playerPlay();
	virtual void playerStop();
	virtual void playerPrev();
	virtual void playerNext();
	virtual void onPlayerNameChange(const QString &);

private slots:
	void onPropertyChange(QDBusMessage AMsg);
	virtual void onPlayersExistenceChanged(QString, QString, QString);

private:
	void connectToBus();
	void disconnectToBus();

private:
	UserTuneData FUserTuneData;
};

#endif // MPRISFETCHER2_H
