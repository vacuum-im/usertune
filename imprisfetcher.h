#ifndef IMPRISFETCHER_H
#define IMPRISFETCHER_H

#include <QObject>
#include <QtDBus/QtDBus>

struct PlayerStatus
{
    int Play;
    int Random;
    int Repeat;
    int RepeatPlaylist;
};

enum PlayStatus
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
    static QString getMetadata();
    virtual QStringList getPlayersList();

signals:
    void changeStatus(PlayStatus);

public slots:
    virtual void playerPlay();
    virtual void playerStop();
    virtual void playerPrev();
    virtual void playerNext();
    virtual void onPlayerNameChange(const QString &);

protected slots:
    void onSetFormat(const QString &);

private:
    inline QString secToTime(int secs);
    static QString formatMetadata(QVariantMap &ATrackInfo, const QString &AFormat);
    virtual void startListening();
    virtual void stopListening();
    virtual void onTrackChange(QVariantMap);
    virtual void onPlayerStatusChange(PlayerStatus);

private:
    QObject FParent;
    QString FPlayerName;
    QDBusInterface *FPlayerInterface;
    PlayerStatus FStatus;
    QString FLineFormat;
    QMap<QString,QVariant> FTrackInfo;
};

Q_DECLARE_METATYPE(PlayerStatus)

#endif // IMPRISFETCHER_H
