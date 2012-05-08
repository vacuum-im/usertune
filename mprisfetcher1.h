#ifndef MPRISFETCHER1_H
#define MPRISFETCHER1_H

#include "imetadatafetcher.h"

class MprisFetcher1 : public IMetaDataFetcher
{
    Q_OBJECT
public:
    MprisFetcher1(QObject *parent, const QString &APlayerName);
    ~MprisFetcher1();
    QVariantMap getMetadata();
    PlayerStatus getPlayerStatus();
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
    void onTrackChange(QVariantMap);
    void onPlayerStatusChange(PlayerStatus);
    virtual void onPlayersExistenceChanged(QString, QString, QString);

private:
    void connectToBus();
    void disconnectToBus();
    void updateStatus();
};

#endif // MPRISFETCHER1_H
