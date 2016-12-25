/**
 * Plugin to allows hadle user tunes for vacuum-im (c) Crying Angel, 2015
 * This plugin uses DBus to get metadata.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 or later as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef USERTUNE_H
#define USERTUNE_H

#include <QDebug>
#include <QTextDocument>
#include <QTimer>

#ifdef Q_OS_LINUX
#  define READ_WRITE_TUNE
#endif

#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/ipepmanager.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ipresencemanager.h>
#include <interfaces/irostermanager.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppstreammanager.h>
#ifdef READ_WRITE_TUNE
#  include <interfaces/imessageprocessor.h>
#  include <interfaces/imessagewidgets.h>
#  include <interfaces/imultiuserchat.h>
#endif

#include <utils/options.h>
#ifdef READ_WRITE_TUNE
#  include "imetadatafetcher.h"
#endif
#include "usertunetypes.h"

#ifdef __cplusplus
  #if __cplusplus < 201103L //C++11
	#define nullptr NULL
  #endif
#endif

#define USERTUNE_UUID  "{b9adf1dd-25e4-48ab-b289-73d3c63e0f4a}"

class UserTuneHandler :
		public QObject,
		public IPlugin,
		public IOptionsDialogHolder,
		public IRosterDataHolder,
		public IPEPHandler
	#ifdef READ_WRITE_TUNE
		,
		public IMessageEditor
	#endif

{
	Q_OBJECT
#ifdef READ_WRITE_TUNE
	Q_INTERFACES(IPlugin IOptionsDialogHolder IRosterDataHolder IPEPHandler IMessageEditor)
#else
	Q_INTERFACES(IPlugin IOptionsDialogHolder IPEPHandler)
#endif
	Q_PLUGIN_METADATA(IID "UserTuneHandler")
public:
	UserTuneHandler();
	~UserTuneHandler();

	static void interfaceNotFoundWarning(const QString ALostInterfaceName) {
		qWarning() << QString("[UserTuneHandler] Interface `%1` not found").arg(ALostInterfaceName);
	}

	//IMessageEditor
#ifdef READ_WRITE_TUNE
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
	virtual QMultiMap<int, IOptionsDialogWidget *> optionsDialogWidgets(const QString &ANodeId, QWidget *AParent);
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
#ifdef READ_WRITE_TUNE
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
	IPresenceManager *FPresenceManager;
	IRosterManager *FRosterManager;
	IRostersModel *FRostersModel;
	IRostersViewPlugin *FRostersViewPlugin;
	IServiceDiscovery *FServiceDiscovery;
	IXmppStreamManager *FXmppStreamManager;
#ifdef READ_WRITE_TUNE
	IMessageWidgets *FMessageWidgets;
	IMetaDataFetcher *FMetaDataFetcher;
	IMultiUserChatManager *FMultiUserChatManager;
	UserTuneData FUserTuneData;
	QTimer FTimer;
#endif
	bool FTuneLabelVisible;
	int FHandlerId;
	quint32 FUserTuneLabelId;
#ifdef READ_WRITE_TUNE
	bool FAllowSendPEP;
	bool FAllowSendURLInPEP;
	QString FFormatTag;
#endif

	QHash<Jid, QHash <QString, UserTuneData> > FContactTune;
	QMap<int, Jid> FNotifies;

private:
#ifdef READ_WRITE_TUNE
	void updateFetchers();
	QString getTagFormated(const Jid &streamJid, const Jid &senderJid) const;
	QString getTagFormated(const UserTuneData &AUserData) const;
#endif
};

#endif // USERTUNE_H

