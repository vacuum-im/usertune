#ifndef USERTUNETYPES_H
#define USERTUNETYPES_H

#include <QString>
#include <QUrl>

enum MprisVer {
    mprisNone = 0,
    mprisV1 = 1,
    mprisV2
};

enum PlayingStatus
{
    PSPlaying = 0,
    PSPaused,
    PSStopped
};

struct PlayerStatus
{
    PlayerStatus() : Play(PSStopped) {}
    bool operator ==(const PlayerStatus &APlayerStatus) const;
    bool operator !=(const PlayerStatus &APlayerStatus) const;
    PlayerStatus &operator =(const PlayerStatus &APlayerStatus);

    int Play;
    int PlayRandom;
    int Repeat;
    int RepeatPlaylist;
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
