#include <QMetaType>

#include "imprisfetcher.h"

QDBusArgument &operator<< (QDBusArgument &arg, const PlayerStatus &ps)
{
    arg.beginStructure ();
    arg << ps.Play
        << ps.Random
        << ps.Repeat
        << ps.RepeatPlaylist;
    arg.endStructure ();
    return arg;
}

const QDBusArgument &operator>> (const QDBusArgument &arg, PlayerStatus &ps)
{
    arg.beginStructure();
    arg >> ps.Play
        >> ps.Random
        >> ps.Repeat
        >> ps.RepeatPlaylist;
    arg.endStructure();
    return arg;
}

IMprisFetcher::IMprisFetcher(QObject *parent) :
    QObject(parent)
{
    qDBusRegisterMetaType<PlayerStatus>();
}

IMprisFetcher::~IMprisFetcher()
{

}

QString IMprisFetcher::secToTime(int secs) {
    int min = 0;
    int sec = secs;
    while (sec > 60) {
        ++min;
        sec -= 60;
    }

    return QString("%1:%2").arg(min).arg(sec,2,10,QChar('0'));
}

QString IMprisFetcher::formatMetadata(QVariantMap &ATrackInfo,QString  const &AFormat) {
    QRegExp rx("%(\\w+-?\\w+)");

    QString outText;
    int srPos = 0;
    int cpPos = 0;

    if (AFormat.contains("time")) {
        ATrackInfo["time"] = secToTime(trackInfo["time"].toInt());
    }

    while ( (srPos = rx.indexIn(AFormat,srPos)) != -1 ) {
        outText.append(AFormat.mid(cpPos,srPos - cpPos)).append(ATrackInfo.contains(rx.cap(1)) ? ATrackInfo[rx.cap(1)].toString() : " " );
        cpPos = srPos + rx.matchedLength();
        srPos += rx.matchedLength();
    }

    outText.append(AFormat.mid(cpPos));

    return outText;
}

QString IMprisFetcher::getMetadata() {
    if (!FPlayerInterface->isValid())
        {
            return QString::Null;
        }

    QDBusReply<QVariantMap> AMetadata = FPlayerInterface->call("GetMetadata");

    if (AMetadata.isValid())
    {
        return formatMetadata(AMetadata);
    } else {
        return QString::Null;
    }
}

void IMprisFetcher::onSetFormat(const QString &AFormat) {
    FLineFormat = AFormat;
}
