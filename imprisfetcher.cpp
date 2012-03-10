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

QString IMprisFetcher::formatMetadata(QVariantMap &trackInfo) {
    QRegExp rx("%(\\w+-?\\w+)");

    QString outText;
    int srPos = 0;
    int cpPos = 0;

    if (FLineFormat.contains("time")) {
        trackInfo["time"] = secToTime(trackInfo["time"].toInt());
    }

    while ( (srPos = rx.indexIn(FLineFormat,srPos)) != -1 ) {
        outText.append(FLineFormat.mid(cpPos,srPos - cpPos)).append(trackInfo.contains(rx.cap(1)) ? trackInfo[rx.cap(1)].toString() : " " );
        cpPos = srPos + rx.matchedLength();
        srPos += rx.matchedLength();
    }

    outText.append(FLineFormat.mid(cpPos));

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
