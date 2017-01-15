#include <QMetaType>
#include "imetadatafetcher.h"

QStringList getPlayersList(const int &ver)
{
	QStringList ret_list;
	QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames().value().filter(ORG_MPRIS_1);

	switch (ver)
	{
	case FetcherVer::mprisV1:
		foreach (QString service, services)
		{
			if (!service.startsWith(ORG_MPRIS_2))
				ret_list << service.replace(ORG_MPRIS_1,"");
		}
		break;
	 case FetcherVer::mprisV2:
		foreach (QString service, services)
		{
			if (service.startsWith(ORG_MPRIS_2))
				ret_list << service.replace(ORG_MPRIS_2,"");
		}
		break;
	default:
		break;
	}

	return ret_list;
}

IMetaDataFetcher::IMetaDataFetcher(QObject *parent) :
	QObject(parent)
{
	QDBusConnection::sessionBus().connect("org.freedesktop.DBus",
										  "/org/freedesktop/DBus",
										  "org.freedesktop.DBus",
										  "NameOwnerChanged",
										  this,
										  SLOT(onPlayersExistenceChanged(QString, QString, QString)));
}

IMetaDataFetcher::~IMetaDataFetcher()
{
	QDBusConnection::sessionBus().disconnect("org.freedesktop.DBus",
											 "/org/freedesktop/DBus",
											 "org.freedesktop.DBus",
											 "NameOwnerChanged",
											 this,
											 SLOT(onPlayersExistenceChanged(QString, QString, QString)));
}
