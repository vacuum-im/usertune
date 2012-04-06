#ifndef MPRISFETCHER1_H
#define MPRISFETCHER1_H

#include "imprisfetcher.h"

class MprisFetcher1 : public IMprisFetcher
{
    Q_OBJECT
public:
    MprisFetcher1(QObject *parent, const QString &APlayerName);
    ~MprisFetcher1();
    QVariantMap getMetadata();
    PlayerStatus getPlayerStatus();
    QString getPlayerName() const;
    virtual QStringList getPlayersList();

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
    void onTrackChange(QVariantMap);
    void onPlayerStatusChange(PlayerStatus);
    virtual void onPlayersExistenceChanged(QString, QString, QString);

private:
    void connectToBus();
    void disconnectToBus();
};

#endif // MPRISFETCHER1_H
