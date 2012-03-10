#ifndef MPRISFETCHER_H
#define MPRISFETCHER_H

#include <QObject>
#include <QStringList>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>

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

public slots:

protected slots:
    void onTrackChange(QVariantMap m_metadata);

};

#endif // MPRISFETCHER_H
