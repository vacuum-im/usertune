#ifndef IMPRISFETCHER_H
#define IMPRISFETCHER_H

#include <QObject>
#include <QtDBus/QtDBus>

struct PlayerStatus
{
    int Play;
    int PlayRandom;
    int Repeat;
    int RepeatPlaylist;
};

enum PlayingStatus
{
    PSPlaying = 0,
    PSPaused,
    PSStopped
};

class IMprisFetcher : public QObject
{
    Q_OBJECT
public:
    explicit IMprisFetcher(QObject *parent);
    virtual ~IMprisFetcher();
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
    inline QString secToTime(int);

private:
    QString FPlayerName;
    QDBusInterface *FPlayerInterface;
    PlayerStatus FStatus;
    QVariantMap FTrackInfo;
};

Q_DECLARE_METATYPE(PlayerStatus)

#endif // IMPRISFETCHER_H
