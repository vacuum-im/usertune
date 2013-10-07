//   Plugin popup notifications for vacuum-im (c) Crying Angel, 2012
//   This plugin uses DBus to show notifications.

//   This library is free software; you can redistribute it and/or
//   modify it under the terms of the GNU Library General Public
//   License version 2 or later as published by the Free Software Foundation.
//
//   This library is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//   Library General Public License for more details.
//
//   You should have received a copy of the GNU Library General Public License
//   along with this library; see the file COPYING.LIB.  If not, write to
//   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
//   Boston, MA 02110-1301, USA.

#ifndef USERTUNE_H
#define USERTUNE_H

#include <QTextDocument>
#include <QTimer>

#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/ipepmanager.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ipresence.h>
#include <interfaces/iroster.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppstreams.h>
#ifdef Q_WS_X11
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imultiuserchat.h>
#endif

#include <utils/options.h>
#ifdef Q_WS_X11
#include "imetadatafetcher.h"
#endif
#include "usertunetypes.h"

#define USERTUNE_UUID  "{b9adf1dd-25e4-48ab-b289-73d3c63e0f4a}"

class UserTuneHandler :
		public QObject,
		public IPlugin,
		public IOptionsHolder,
		public IRosterDataHolder,
		public IPEPHandler
		#ifdef Q_WS_X11
		,
		public IMessageEditor
		#endif

{
	Q_OBJECT
#ifdef Q_WS_X11
	Q_INTERFACES(IPlugin IOptionsHolder IRosterDataHolder IPEPHandler IMessageEditor)
#else
	Q_INTERFACES(IPlugin IOptionsHolder IPEPHandler)
#endif
public:
	UserTuneHandler();
	~UserTuneHandler();
	//IMessageEditor
#ifdef Q_WS_X11
	virtual bool messageReadWrite(int AOrder, const Jid &AStreamJid, Message &AMessage, int ADirection);
#endif
	//IPlugin
	virtual QObject *instance()
	{
		return this;
	}
	virtual QUuid pluginUuid() const
	{
		return USERTUNE_UUID;
	}
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin()
	{
		return true;
	}
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IRosterDataHolder
	virtual QList<int> rosterDataRoles(int AOrder) const;
	virtual QVariant rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole);
	//IPEPHandler
	virtual bool processPEPEvent(const Jid &streamJid, const Stanza &AStanza);

signals:
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = nullptr, int ARole = 0);
	//IRostersLabelHolder
	void rosterLabelChanged(quint32 ALabelId, IRosterIndex *AIndex = nullptr);

protected slots:
#ifdef Q_WS_X11
	void onTrackChanged(UserTuneData);
	void onPlayerSatusChanged(PlayerStatus);
	void onSendPep();
	void onStopPublishing();
#endif
	void onContactStateChanged(const Jid &streamJid, const Jid &senderJid, bool AStateOnline);
	void onCopyToClipboardActionTriggered(bool);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	void onOptionsChanged(const OptionsNode &ANode);
	void onOptionsOpened();
	//IRostersView
	void onRosterIndexClipboardMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips);
	//IXmppStreams
	void onSetMainLabel(IXmppStream *);
	void onUnsetMainLabel(IXmppStream *);
	void onShowNotification(const Jid &streamJid, const Jid &senderJid);
	void onApplicationQuit();

protected:
	void onLabelsEnabled(const Jid &streamJid);
	void setContactTune(const Jid &streamJid, const Jid &contactJid, const UserTuneData &song);
	//IRosterDataHolder
	void updateDataHolder(const Jid &streamJid, const Jid &senderJid);

	static const QString secondsToString(unsigned short sec)
	{
		if (sec == 0)
			return QString();
		int min = 0;
		while (sec > 60) {
			++min;
			sec -= 60;
		}
		return QString("%1:%2").arg(min).arg(sec,2,10,QChar('0'));
	}

private:
	INotifications *FNotifications;
	IOptionsManager *FOptionsManager;
	IPEPManager *FPEPManager;
	IPresencePlugin *FPresencePlugin;
	IRoster *FRoster;
	IRosterPlugin *FRosterPlugin;
	IRostersModel *FRostersModel;
	IRostersViewPlugin *FRostersViewPlugin;
	IServiceDiscovery *FServiceDiscovery;
	IXmppStreams *FXmppStreams;
#ifdef Q_WS_X11
	IMessageWidgets *FMessageWidgets;
	IMetaDataFetcher *FMetaDataFetcher;
	IMultiUserChatPlugin *FMultiUserChatPlugin;
	UserTuneData FUserTuneData;
	QTimer FTimer;
#endif
	bool FTuneLabelVisible;
	int FHandlerId;
	quint32 FUserTuneLabelId;
#ifdef Q_WS_X11
	bool FAllowSendPEP;
	bool FAllowSendURLInPEP;
	QString FFormatTag;
#endif

	QHash<Jid, QHash <QString, UserTuneData> > FContactTune;
	QMap<int, Jid> FNotifies;

private:
#ifdef Q_WS_X11
	void updateFetchers();
	QString getTagFormated(const Jid &streamJid, const Jid &senderJid) const;
	QString getTagFormated(const UserTuneData &AUserData) const;
#endif
};

#endif // USERTUNE_H

