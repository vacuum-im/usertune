#include <QDebug>

#include "mprisfetcher.h"

MprisFetcher::MprisFetcher(QObject *parent) :
    QObject(parent)
{
    playerExist = false;
}

MprisFetcher::~MprisFetcher()
{
    if (playerExist)
    {
        bool test = QDBusConnection::sessionBus().disconnect("org.mpris." + curPlayerName,
                                            "/Player",
                                            "org.freedesktop.MediaPlayer",
                                            "TrackChange",
                                            "a{sv}",
                                            this,
                                            SLOT(onTrackChange(QVariantMap)));
        qDebug() << test;
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
        bool test = QDBusConnection::sessionBus().disconnect("org.mpris." + curPlayerName,
                                            "/Player",
                                            "org.freedesktop.MediaPlayer",
                                            "TrackChange",
                                            "a{sv}",
                                            this,
                                            SLOT(onTrackChange(QVariantMap)));
//        qDebug() << test;
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
                SLOT(onTrackChange(QVariantMap)));
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

void MprisFetcher::onTrackChange(QVariantMap trackInfo)
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
