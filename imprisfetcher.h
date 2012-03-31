#ifndef IMPRISFETCHER_H
#define IMPRISFETCHER_H

#include <QObject>
#include <QtDBus/QtDBus>

#include "usertunetypes.h"

class IMprisFetcher : public QObject
{
    Q_OBJECT
public:
    explicit IMprisFetcher(QObject *parent);
    virtual ~IMprisFetcher();
    virtual QStringList getPlayersList() = 0;
    QVariantMap getMetadata();
    PlayerStatus getPlayerStatus();
    QString getPlayerName() const { return FPlayerName; }

signals:
    void statusChanged(PlayerStatus);
    void trackChanged(UserTuneData);

public slots:
    virtual void playerPlay() = 0;
    virtual void playerStop() = 0;
    virtual void playerPrev() = 0;
    virtual void playerNext() = 0;
    virtual void onPlayerNameChange(const QString &) = 0;

protected slots:
    virtual void onPlayersExistenceChanged(QString, QString, QString) = 0;

protected:
    inline QString secToTime(int sec)
    {
        int min = 0;

        while (sec > 60) {
            ++min;
            sec -= 60;
        }

        return QString("%1:%2").arg(min).arg(sec,2,10,QChar('0'));
    }

protected:
    QString FPlayerName;
    QDBusInterface *FPlayerInterface;
    PlayerStatus FStatus;
    QVariantMap FTrackInfo;
};

Q_DECLARE_METATYPE(PlayerStatus)

#endif // IMPRISFETCHER_H
