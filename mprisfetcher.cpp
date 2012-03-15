#include <QDebug>

#include "mprisfetcher.h"

MprisFetcher::MprisFetcher(QObject *parent) :
    QObject(parent)
{
    playerExist = false;
    QDBusConnection::sessionBus().connect ("org.freedesktop.DBus",
                                           "/org/freedesktop/DBus",
                                           "org.freedesktop.DBus",
                                           "NameOwnerChanged",
                                           this,
                                           SLOT(onPlayersExistenceChanged(QString, QString, QString)));
}

MprisFetcher::~MprisFetcher()
{
    if (playerExist)
    {
        QDBusConnection::sessionBus().disconnect(
                "org.mpris." + curPlayerName,
                "/Player",
                "org.freedesktop.MediaPlayer",
                "TrackChange",
                "a{sv}",
                this,
                SLOT(onTrackChanged(QVariantMap)));
        QDBusConnection::sessionBus().disconnect(
                "org.mpris." + curPlayerName,
                "/Player",
                "org.freedesktop.MediaPlayer",
                "StatusChange",
                "(iiii)",
                this,
                SLOT (onStatusChanged(PlayerStatus)));
        delete m_player;
    }
}

QStringList MprisFetcher::getPlayers()
{
    QStringList players_v1 = QDBusConnection::sessionBus().interface()->registeredServiceNames().value().filter("org.mpris.");
    players_v1.replaceInStrings(QRegExp("org.mpris.(MediaPlayer2.)?"),"");
    players_v1.removeDuplicates();

    return players_v1;
}

QString MprisFetcher::setPlayer(QString playerName)
{
    if (playerExist)
    {
        QDBusConnection::sessionBus().disconnect(
                "org.mpris." + curPlayerName,
                "/Player",
                "org.freedesktop.MediaPlayer",
                "TrackChange",
                "a{sv}",
                this,
                SLOT(onTrackChanged(QVariantMap)));
        QDBusConnection::sessionBus().disconnect(
                "org.mpris." + curPlayerName,
                "/Player",
                "org.freedesktop.MediaPlayer",
                "StatusChange",
                "(iiii)",
                this,
                SLOT (onStatusChanged(PlayerStatus)));
        delete m_player;
    }
    m_player = new QDBusInterface("org.mpris." + playerName, "/Player",
                                  "org.freedesktop.MediaPlayer", QDBusConnection::sessionBus());
    if (m_player->lastError().type() != QDBusError::NoError) {
        qDebug() << QDBusError::errorString(m_player->lastError().type());
        return QDBusError::errorString(m_player->lastError().type());
    }
    QDBusConnection::sessionBus().connect(
                "org.mpris." + playerName,
                "/Player",
                "org.freedesktop.MediaPlayer",
                "TrackChange",
                "a{sv}",
                this,
                SLOT(onTrackChanged(QVariantMap)));
    bool test = QDBusConnection::sessionBus().connect(
                "org.mpris." + playerName,
                "/Player",
                "org.freedesktop.MediaPlayer",
                "StatusChange",
                "(iiii)",
                this,
                SLOT (onStatusChanged(PlayerStatus)));
    qDebug() << "status connect: " << test;
    QDBusReply<QVariantMap> m_metadata = m_player->call("GetMetadata");
    QVariantMap trackInfo = m_metadata.value();
    QList<QString> keys = trackInfo.keys();
    QString info;
    foreach (QString key, keys) {
        info.append('%'+key+'\n');
    }

    playerExist = true;
    curPlayerName = playerName;
    return info;
}

void MprisFetcher::setFormat(const QString &format)
{
    m_format = format;
}

void MprisFetcher::onTrackChanged(QVariantMap trackInfo)
{
//    QRegExp rx("%(\\w+-?\\w+)");

//    QString outText;
//    int srPos = 0;
//    int cpPos = 0;

//    while ( (srPos = rx.indexIn(m_format,srPos)) != -1 ) {
//        outText.append(m_format.mid(cpPos,srPos - cpPos)).append(trackInfo.contains(rx.cap(1)) ? trackInfo[rx.cap(1)].toString() : " " );
//        cpPos = srPos + rx.matchedLength();
//        srPos += rx.matchedLength();
//    }

//    outText.append(m_format.mid(cpPos));

    emit trackChanged(trackInfo);

}

void MprisFetcher::onStatusChanged(PlayStatus m_pstatus)
{
    qDebug() << m_pstatus.PlayStatus_;
    if (m_pstatus.PlayStatus_ == PSStopped)
        emit playerStoped();
}

void MprisFetcher::onPlayersExistenceChanged(QString name, QString, QString newOwner)
{
    if (!name.startsWith("org.mpris")) {
        return;
    }


    // какая непонятная портянка. все перепишу когда доделаю impris fetcher!!!
    if (!newOwner.isEmpty()) {
        qDebug() << "Available new player " + name + ".";
        qDebug() << "newOwner " + newOwner;

        playerExist = curPlayerName.compare(name);
        if (playerExist) {
        // завернуть все в одну функцию как в imprisfetcher
            QDBusConnection::sessionBus().connect(
                        "org.mpris." + playerName,
                        "/Player",
                        "org.freedesktop.MediaPlayer",
                        "TrackChange",
                        "a{sv}",
                        this,
                        SLOT(onTrackChanged(QVariantMap)));
            QDBusConnection::sessionBus().connect(
                        "org.mpris." + playerName,
                        "/Player",
                        "org.freedesktop.MediaPlayer",
                        "StatusChange",
                        "(iiii)",
                        this,
                        SLOT (onStatusChanged(int, int, int, int)));
        }
    } else if (newOwner.isEmpty ()) {
        qDebug() << "Player " + name + "shutdown.";
        qDebug() << "newOwner " + newOwner;

        playerExist = curPlayerName.compare(name);
        if (playerExist)
        {
            QDBusConnection::sessionBus().disconnect(
                    "org.mpris." + curPlayerName,
                    "/Player",
                    "org.freedesktop.MediaPlayer",
                    "TrackChange",
                    "a{sv}",
                    this,
                    SLOT(onTrackChanged(QVariantMap)));
            QDBusConnection::sessionBus().disconnect(
                    "org.mpris." + curPlayerName,
                    "/Player",
                    "org.freedesktop.MediaPlayer",
                    "StatusChange",
                    "(iiii)",
                    this,
                    SLOT (onStatusChanged(int, int, int, int)));
            delete m_player;
        }
    }
}
