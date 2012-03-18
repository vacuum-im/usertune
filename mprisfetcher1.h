#ifndef MPRISFETCHER1_H
#define MPRISFETCHER1_H

#include "imprisfetcher.h"

class MprisFetcher1 : public IMprisFetcher
{
public:
    MprisFetcher1(QObject *parent, const QString &APlayerName);
    ~MprisFetcher1();
    virtual QStringList getPlayersList();
    QVariantMap getMetadata();
    bool isNowPlaying();

signals:
    void statusChanged(PlayingStatus);
    void trackChanged(QVariantMap);

public slots:
    virtual void playerPlay();
    virtual void playerStop();
    virtual void playerPrev();
    virtual void playerNext();
    virtual void onPlayerNameChange(const QString &);

private slots:
    virtual void onTrackChange(QVariantMap);
    virtual void onPlayerStatusChange(PlayerStatus);
    virtual void onPlayersExistenceChanged(QString, QString, QString);

private:
    void connectToBus();
    void disconnectToBus();
};

#endif // MPRISFETCHER1_H
