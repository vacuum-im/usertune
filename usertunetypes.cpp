#include "usertunetypes.h"

bool PlayerStatus::operator ==(const PlayerStatus &APlayerStatus) const
{
    return (Play == APlayerStatus.Play
            && PlayRandom == APlayerStatus.PlayRandom
            && Repeat == APlayerStatus.Repeat
            && RepeatPlaylist == APlayerStatus.RepeatPlaylist);
}

bool PlayerStatus::operator !=(const PlayerStatus &APlayerStatus) const
{
    return !operator==(APlayerStatus);
}

PlayerStatus &PlayerStatus::operator =(const PlayerStatus &APlayerStatus)
{
    Play = APlayerStatus.Play;
    PlayRandom = APlayerStatus.PlayRandom;
    Repeat = APlayerStatus.Repeat;
    RepeatPlaylist = APlayerStatus.RepeatPlaylist;

    return *this;
}

UserTuneData::UserTuneData() :
    length(0),
    rating(0)
{

}

UserTuneData::~UserTuneData()
{

}

bool UserTuneData::isEmpty() const
{
    return artist.isEmpty() && source.isEmpty() && title.isEmpty() && track.isEmpty() && uri.isEmpty();
}

bool UserTuneData::operator ==(const UserTuneData &AUserTune) const
{
    return (artist      == AUserTune.artist
            && title      == AUserTune.title
            && source  == AUserTune.source
            && track    == AUserTune.track
            && length   == AUserTune.length
            && rating   == AUserTune.rating
            && uri        == AUserTune.uri);
}

bool UserTuneData::operator !=(const UserTuneData &AUserTune) const
{
    return !operator==(AUserTune);
}

UserTuneData &UserTuneData::operator =(const UserTuneData &AUserTune)
{
    artist = AUserTune.artist;
    source = AUserTune.source;
    title = AUserTune.title;
    track = AUserTune.track;
    length = AUserTune.length;
    rating = AUserTune.rating;
    uri = AUserTune.uri;

    return *this;
}
