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
    virtual QStringList getPlayersList() = 0;
    QVariantMap getMetadata();
    bool isNowPlaying();

signals:
    void statusChanged(PlayingStatus);
    void trackChanged(QVariantMap);

public slots:
    virtual void playerPlay() = 0;
    virtual void playerStop() = 0;
    virtual void playerPrev() = 0;
    virtual void playerNext() = 0;
    virtual void onPlayerNameChange(const QString &) = 0;

private slots:
    /* MPRIS v.1 */
    virtual void onTrackChange(QVariantMap) = 0;
    virtual void onPlayerStatusChange(PlayerStatus) = 0;
    /* MPRIS v.2 */
    virtual void onPropertyChange(QDBusMessage) = 0;
    /* All */
    virtual void onPlayersExistenceChanged(QString, QString, QString) = 0;

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
