#ifndef USERTUNETYPES_H
#define USERTUNETYPES_H

#include <QString>
#include <QUrl>

struct PlayerStatus
{
    bool operator ==(const PlayerStatus &APlayerStatus) const {
        return (Play == APlayerStatus.Play
                && PlayRandom == APlayerStatus.PlayRandom
                && Repeat == APlayerStatus.Repeat
                && RepeatPlaylist == APlayerStatus.RepeatPlaylist);
    }

    bool operator !=(const PlayerStatus &APlayerStatus) const { return !operator==(APlayerStatus); }

    PlayerStatus &operator =(const PlayerStatus &APlayerStatus) {
        Play = APlayerStatus.Play;
        PlayRandom = APlayerStatus.PlayRandom;
        Repeat = APlayerStatus.Repeat;
        RepeatPlaylist = APlayerStatus.RepeatPlaylist;

        return *this;
    }

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
    bool isEmpty() const { return artist.isEmpty() && source.isEmpty() && title.isEmpty() && track.isEmpty() && uri.isEmpty(); }

    bool operator ==(const UserTuneData &AUserTune) const
    {
        return (artist      == AUserTune.artist
                && title      == AUserTune.title
                && source  == AUserTune.source
                && track    == AUserTune.track
                && length   == AUserTune.length
                && rating   == AUserTune.rating
                && uri        == AUserTune.uri);
    }

    bool operator !=(const UserTuneData &AUserTune) const { return !operator==(AUserTune); }

    UserTuneData &operator =(const UserTuneData &AUserTune) {
        artist = AUserTune.artist;
        source = AUserTune.source;
        title = AUserTune.title;
        track = AUserTune.track;
        length = AUserTune.length;
        rating = AUserTune.rating;
        uri = AUserTune.uri;

        return *this;
    }

    QString artist;
    QString source;
    QString title;
    QString track;
    int length;
    int rating;
    QUrl uri;
};

#endif // USERTUNETYPES_H
