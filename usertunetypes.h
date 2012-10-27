#ifndef USERTUNETYPES_H
#define USERTUNETYPES_H

#include <QString>
#include <QUrl>

namespace FetcherVer {
enum FetchersVer {
	fetcherNone = 0,
#ifdef Q_WS_X11
	mprisV1 = 1,
	mprisV2
#elif Q_WS_WIN

#endif
};
}

enum PlayingStatus
{
	PSPlaying = 0,
	PSPaused,
	PSStopped
};

struct PlayerStatus
{
	PlayerStatus() : Play(PSStopped), PlayRandom(0), Repeat(0), RepeatPlaylist(0) {}
	PlayerStatus(const PlayerStatus &);
	bool operator ==(const PlayerStatus &) const;
	bool operator !=(const PlayerStatus &) const;
	PlayerStatus &operator =(const PlayerStatus &);

	unsigned short Play;
	unsigned short PlayRandom;
	unsigned short Repeat;
	unsigned short RepeatPlaylist;
};

struct UserTuneData
{
	UserTuneData() : length(0), rating(0) {}
	UserTuneData(const UserTuneData &);
	bool isEmpty() const;
	bool operator ==(const UserTuneData &) const;
	bool operator !=(const UserTuneData &) const;
	UserTuneData &operator =(const UserTuneData &);

	QString artist;
	QString source;
	QString title;
	QString track;
	unsigned short length;
	unsigned short rating;
	QUrl uri;
};

#endif // USERTUNETYPES_H
