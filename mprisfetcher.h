#ifndef MPRISFETCHER_H
#define MPRISFETCHER_H

#include <QObject>
#include <QStringList>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>

enum PlayStatus
{
    PSPlaying = 0,
    PSPaused,
    PSStopped
};

struct PlayerStatus
{
    int PlayStatus_;
    int PlayOrder_;
    int PlayRepeat_;
    int StopOnce_;
};

class MprisFetcher : public QObject
{
Q_OBJECT
public:
    explicit MprisFetcher(QObject *parent = 0);
    ~MprisFetcher();

    QStringList getPlayers();
    QString setPlayer(QString playerName);
    void setFormat(const QString &format);

private:
    QDBusInterface *m_player;
    QString m_format;
    bool playerExist;
    QString curPlayerName;

signals:
    void trackChanged(QVariantMap trackInfo);
    void playerStoped();

public slots:

protected slots:
    void onTrackChanged(QVariantMap m_metadata);
    void onStatusChanged(int PlayStatus_, int PlayOrder_,  int PlayRepeat_, int StopOnce_);
    void onPlayersExistenceChanged(QString, QString, QString);

};

#endif // MPRISFETCHER_H
