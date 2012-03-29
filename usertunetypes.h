#ifndef USERTUNETYPES_H
#define USERTUNETYPES_H

#include <QString>
#include <QUrl>

struct PlayerStatus
{
    bool operator ==(const PlayerStatus &APlayerStatus) const;
    bool operator !=(const PlayerStatus &APlayerStatus) const;
    PlayerStatus &operator =(const PlayerStatus &APlayerStatus);

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

enum MprisVer {
    mprisV1 = 0,
    mprisV2
};

struct UserTuneData
{
    bool isEmpty() const;
    bool operator ==(const UserTuneData &AUserTune) const;
    bool operator !=(const UserTuneData &AUserTune) const;
    UserTuneData &operator =(const UserTuneData &AUserTune);

    QString artist;
    QString source;
    QString title;
    QString track;
    int length;
    int rating;
    QUrl uri;
};

#endif // USERTUNETYPES_H
