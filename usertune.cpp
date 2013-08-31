#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif

#include <QClipboard>
#include <QApplication>

#include <definitions/menuicons.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/notificationtypes.h>
#include <definitions/optionvalues.h>
#include <definitions/resources.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rostertooltiporders.h>

#include <utils/advanceditemdelegate.h>

#include "usertune.h"
#include "mprisfetcher1.h"
#include "mprisfetcher2.h"
#ifdef Q_WS_X11
#include "usertuneoptions.h"
#endif

#include "definitions.h"

#ifdef Q_WS_X11
#define ADD_CHILD_ELEMENT(document, root_element, child_name, child_data) \
{ \
	QDomElement tag = (document).createElement(child_name); \
	QDomText text = (document).createTextNode(child_data); \
	tag.appendChild(text); \
	(root_element).appendChild(tag); \
	}
#endif

#define ADR_CLIPBOARD_DATA Action::DR_Parametr2

#define ACTION_PREPEND_TEXT QLatin1String("/me ")

#define TUNE_PROTOCOL_URL QLatin1String("http://jabber.org/protocol/tune")
#define TUNE_NOTIFY_PROTOCOL_URL QLatin1String("http://jabber.org/protocol/tune+notify")
// Delay befo send pep to prevent a large number of
// updates when a user is skipping through tracks
#define PEP_SEND_DELAY 5*1000

static const QList<int> RosterKinds = QList<int>() << RIK_CONTACT << RIK_CONTACTS_ROOT << RIK_STREAM_ROOT;

UserTuneHandler::UserTuneHandler() :
	FNotifications(nullptr),
	FOptionsManager(nullptr),
	FPEPManager(nullptr),
	FRoster(nullptr),
	FRosterPlugin(nullptr),
	FRostersModel(nullptr),
	FRostersViewPlugin(nullptr),
	FServiceDiscovery(nullptr),
	FXmppStreams(nullptr)
  #ifdef Q_WS_X11
	,FMessageWidgets(nullptr),
	FMetaDataFetcher(nullptr),
	FMultiUserChatPlugin(nullptr)
  #endif
{
#ifdef Q_WS_X11
	FTimer.setSingleShot(true);
	FTimer.setInterval(PEP_SEND_DELAY);
	connect(&FTimer, SIGNAL(timeout()), this, SLOT(onSendPep()));
#endif
}

UserTuneHandler::~UserTuneHandler()
{

}

void UserTuneHandler::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("User Tune Handler");
	APluginInfo->description = tr("Allows hadle user tunes");
	APluginInfo->version = QLatin1String("1.0.7");
	APluginInfo->author = QLatin1String("Crying Angel");
	APluginInfo->homePage = QLatin1String("http://www.vacuum-im.org");
	APluginInfo->dependences.append(PEPMANAGER_UUID);
	APluginInfo->dependences.append(SERVICEDISCOVERY_UUID);
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool UserTuneHandler::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	AInitOrder = 500;

	IPlugin *plugin;

	plugin = APluginManager->pluginInterface("IPEPManager").value(0, nullptr);
	if (!plugin) return false;

	FPEPManager = qobject_cast<IPEPManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0, nullptr);
	if (!plugin) return false;
	FServiceDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0, nullptr);
	if (!plugin) return false;

	FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
	connect(FXmppStreams->instance(), SIGNAL(opened(IXmppStream *)), this, SLOT(onSetMainLabel(IXmppStream*)));
	connect(FXmppStreams->instance(), SIGNAL(closed(IXmppStream *)), this, SLOT(onUnsetMainLabel(IXmppStream*)));

	int streams_size = FXmppStreams->xmppStreams().size();
	for (int i = 0; i < streams_size; i++) {
		connect(FXmppStreams->xmppStreams().at(i)->instance(), SIGNAL(aboutToClose()), this, SLOT(onStopPublishing()));
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0, nullptr);
	if (plugin) {
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());

		if (FPresencePlugin)	{
			connect(FPresencePlugin->instance(),
					SIGNAL(contactStateChanged(const Jid &, const Jid &, bool)),
					SLOT(onContactStateChanged(const Jid &, const Jid &, bool)));
		}
	}

	plugin = APluginManager->pluginInterface("IRoster").value(0, nullptr);
	if(plugin) {
		FRoster = qobject_cast<IRoster *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0, nullptr);
	Q_ASSERT(plugin);
	if (plugin)	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0, nullptr);
	Q_ASSERT(plugin);
	if (plugin)	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0, nullptr);
	Q_ASSERT(plugin);
	if (plugin)	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)	{
			connect(FRostersViewPlugin->rostersView()->instance(),
					SIGNAL(indexClipboardMenu(const QList<IRosterIndex *> &, quint32, Menu *)),
					SLOT(onRosterIndexClipboardMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
			connect(FRostersViewPlugin->rostersView()->instance(),
					SIGNAL(indexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)),
					SLOT(onRostersViewIndexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)));
		}
	}
#ifdef Q_WS_X11
	// player manage (/play, /pause etc command)
	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0, nullptr);
	Q_ASSERT(plugin);
	if (plugin) {
		IMessageProcessor *messageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
		Q_ASSERT(messageProcessor);
		if (messageProcessor) {
			messageProcessor->insertMessageEditor(MEO_USERTUNE, this);
		}
	}

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0, nullptr);
	Q_ASSERT(plugin);
	if (plugin) {
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMultiUserChatPlugin").value(0, nullptr);
	Q_ASSERT(plugin);
	if (plugin) {
		FMultiUserChatPlugin = qobject_cast<IMultiUserChatPlugin *>(plugin->instance());
	}
#endif
	plugin = APluginManager->pluginInterface("INotifications").value(0, nullptr);
	Q_ASSERT(plugin);
	if (plugin) {
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications) {
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0, nullptr);
	Q_ASSERT(plugin);
	if (plugin) {
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	connect (APluginManager->instance(), SIGNAL(aboutToQuit()), this, SLOT(onApplicationQuit()));

	return true;
}

bool UserTuneHandler::initObjects()
{
	FHandlerId = FPEPManager->insertNodeHandler(TUNE_PROTOCOL_URL, this);

	IDiscoFeature feature;
	feature.active = true;
	feature.name = tr("User tune");
	feature.var = TUNE_PROTOCOL_URL;

	FServiceDiscovery->insertDiscoFeature(feature);

	feature.name = tr("User tune notification");
	feature.var = TUNE_NOTIFY_PROTOCOL_URL;
	FServiceDiscovery->insertDiscoFeature(feature);

	if (FNotifications) {
		INotificationType notifyType;
		notifyType.order = NTO_USERTUNE_NOTIFY;
		notifyType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERTUNE_MUSIC);
		notifyType.title = tr("When reminding of contact playing music");
		notifyType.kindMask = INotification::PopupWindow;
		notifyType.kindDefs = notifyType.kindMask;
		FNotifications->registerNotificationType(NNT_USERTUNE, notifyType);
	}

	if (FRostersModel) {
		FRostersModel->insertRosterDataHolder(RDHO_USERTUNE, this);
	}

	FUserTuneLabelId = 0;

	return true;
}

bool UserTuneHandler::initSettings()
{
	Options::setDefaultValue(OPV_USERTUNE_SHOW_ROSTER_LABEL,true);
	Options::setDefaultValue(OPV_USERTUNE_ALLOW_SEND_MUSIC_INFO,true);
	Options::setDefaultValue(OPV_USERTUNE_NOT_ALLOW_SEND_URL_INFO,false);
	Options::setDefaultValue(OPV_USERTUNE_TAG_FORMAT,QLatin1String("%T - %A - %S"));
#ifdef Q_WS_X11
	Options::setDefaultValue(OPV_USERTUNE_PLAYER_NAME,QLatin1String("amarok"));
	Options::setDefaultValue(OPV_USERTUNE_PLAYER_VER,FetcherVer::mprisV1);
#elif Q_WS_WIN
	// TODO: сделать для windows
	Options::setDefaultValue(OPV_USERTUNE_PLAYER_NAME,QLatin1String(""));
	Options::setDefaultValue(OPV_USERTUNE_PLAYER_VER,FetchrVer::fetcherNone);
#endif

	if (FOptionsManager) {
		IOptionsDialogNode dnode = { ONO_USERTUNE, OPN_USERTUNE, tr("User Tune"), MNI_USERTUNE_MUSIC };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}

	return true;
}

QMultiMap<int, IOptionsWidget *> UserTuneHandler::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FOptionsManager && ANodeId == OPN_USERTUNE)	{
#ifdef Q_WS_X11
		widgets.insertMulti(OWO_USERTUNE, new UserTuneOptions(AParent));
#elif Q_WS_WIN
		widgets.insertMulti(OWO_USERTUNE, FOptionsManager->optionsNodeWidget(Options::node(OPV_USERTUNE_SHOW_ROSTER_LABEL),tr("Show music icon in roster"),AParent));
		widgets.insertMulti(OWO_USERTUNE, FOptionsManager->optionsNodeWidget(Options::node(OPV_USERTUNE_TAG_FORMAT),tr("Tag format:"),AParent));
		widgets.insertMulti(OWO_USERTUNE, FOptionsManager->optionsNodeWidget(Options::node(OPV_USERTUNE_PLAYER_NAME),tr("Player name:"),AParent));
#endif
	}
	return widgets;
}

void UserTuneHandler::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_USERTUNE_SHOW_ROSTER_LABEL));
	onOptionsChanged(Options::node(OPV_USERTUNE_TAG_FORMAT));
#ifdef Q_WS_X11
	onOptionsChanged(Options::node(OPV_USERTUNE_ALLOW_SEND_MUSIC_INFO));
	onOptionsChanged(Options::node(OPV_USERTUNE_NOT_ALLOW_SEND_URL_INFO));
	onOptionsChanged(Options::node(OPV_USERTUNE_PLAYER_VER));
#endif
}

void UserTuneHandler::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_USERTUNE_SHOW_ROSTER_LABEL) {
		FTuneLabelVisible = ANode.value().toBool();
		if(FTuneLabelVisible) {
			if(FUserTuneLabelId == 0) {
				AdvancedDelegateItem notifyLabel(RLID_USERTUNE);
				notifyLabel.d->kind = AdvancedDelegateItem::CustomData;
				notifyLabel.d->data = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERTUNE_MUSIC);
				FUserTuneLabelId = FRostersViewPlugin->rostersView()->registerLabel(notifyLabel);
				foreach (Jid streamJid, FRostersModel->streams()) {
					onLabelsEnabled(streamJid);
				}
			}
		} else if(FUserTuneLabelId != 0) {
			FRostersViewPlugin->rostersView()->removeLabel(FUserTuneLabelId);
			FUserTuneLabelId = 0;
		}
	} else if (ANode.path() == OPV_USERTUNE_TAG_FORMAT) {
		FFormatTag = Options::node(OPV_USERTUNE_TAG_FORMAT).value().toString();
	}
#ifdef Q_WS_X11
	else if (ANode.path() == OPV_USERTUNE_PLAYER_VER) {
		updateFetchers();
	} else if (ANode.path() == OPV_USERTUNE_PLAYER_NAME) {
		Q_ASSERT(FMetaDataFetcher);
		if (FMetaDataFetcher) {
			FMetaDataFetcher->onPlayerNameChange(Options::node(OPV_USERTUNE_PLAYER_NAME).value().toString());
		}
	} else if (ANode.path() == OPV_USERTUNE_ALLOW_SEND_MUSIC_INFO) {
		if (!(FAllowSendPEP = Options::node(OPV_USERTUNE_ALLOW_SEND_MUSIC_INFO).value().toBool())) {
			onStopPublishing();
		} else {
			Q_ASSERT(FMetaDataFetcher);
			if (FMetaDataFetcher) {
				FMetaDataFetcher->updateStatus();
			}
		}
	} else if (ANode.path() == OPV_USERTUNE_NOT_ALLOW_SEND_URL_INFO) {
		FAllowSendURLInPEP = !Options::node(OPV_USERTUNE_NOT_ALLOW_SEND_URL_INFO).value().toBool();
	}
#endif
}

QList<int> UserTuneHandler::rosterDataRoles(int AOrder) const
{
	QList<int> indexRoles;

	if (AOrder == RDHO_USERTUNE) {
		indexRoles << RDR_TUNE_NAME;
	}

	return indexRoles;
}

QVariant UserTuneHandler::rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const
{
	Q_UNUSED(AOrder)
	Q_UNUSED(AIndex)
	Q_UNUSED(ARole)

	return QVariant();
}

bool UserTuneHandler::setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole)
{
	Q_UNUSED(AOrder)
	Q_UNUSED(AIndex)
	Q_UNUSED(ARole)
	Q_UNUSED(AValue)

	return false;
}

#ifdef Q_WS_X11
// for player manage
bool UserTuneHandler::messageReadWrite(int AOrder, const Jid &AStreamJid, Message &AMessage, int ADirection)
{
	Q_UNUSED(AOrder);
	Q_UNUSED(AStreamJid);
	Q_ASSERT(FMetaDataFetcher);

	if (!FMetaDataFetcher || ADirection != IMessageProcessor::MessageOut || !AMessage.body().startsWith('/')) {
		return false;
	}

	bool breakNextCheck = false;

	if (AMessage.body().startsWith(QLatin1String("/play"), Qt::CaseInsensitive) ||
			AMessage.body().startsWith(QLatin1String("/pause"), Qt::CaseInsensitive)) {
		FMetaDataFetcher->playerPlay();
		breakNextCheck = true;
	}
	else if (AMessage.body().startsWith(QLatin1String("/stop"), Qt::CaseInsensitive)) {
		FMetaDataFetcher->playerStop();
		breakNextCheck = true;
	}
	else if (AMessage.body().startsWith(QLatin1String("/next"), Qt::CaseInsensitive)) {
		FMetaDataFetcher->playerNext();
		breakNextCheck = true;
	}
	else if (AMessage.body().startsWith(QLatin1String("/prev"), Qt::CaseInsensitive)) {
		FMetaDataFetcher->playerPrev();
		breakNextCheck = true;
	}
	else if (AMessage.body().startsWith(QLatin1String("/np"), Qt::CaseInsensitive)) {
		AMessage.setBody(getTagFormated(FUserTuneData).prepend(ACTION_PREPEND_TEXT));
		breakNextCheck = false;
	} else {
		return false;
	}

	IMessageEditWidget *widget;

	switch (AMessage.type()) {
		case Message::Chat:
			widget = FMessageWidgets->findChatWindow(AStreamJid,AMessage.stanza().to())->editWidget();
			break;
		case Message::GroupChat:
			widget = FMultiUserChatPlugin->findMultiChatWindow(AStreamJid,AMessage.stanza().to())->editWidget();
			break;
		default:
			widget = nullptr;
			break;
	}
	Q_ASSERT(widget);
	if (widget) {
		widget->document()->clear();
	}

	return breakNextCheck;
}
#endif

void UserTuneHandler::onShowNotification(const Jid &streamJid, const Jid &senderJid)
{
	if (FNotifications && FContactTune[streamJid].contains(senderJid.pBare())
			&& streamJid.pBare() != senderJid.pBare()) {
		INotification notify;
		notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_USERTUNE);
		if ((notify.kinds & INotification::PopupWindow) > 0) {
			notify.typeId = NNT_USERTUNE;
			notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERTUNE_MUSIC));
			notify.data.insert(NDR_STREAM_JID,streamJid.full());
			notify.data.insert(NDR_CONTACT_JID,senderJid.full());
			notify.data.insert(NDR_TOOLTIP,QString("%1 %2").arg(FNotifications->contactName(streamJid, senderJid)).arg(tr("listening to")));
			notify.data.insert(NDR_POPUP_CAPTION,tr("User Tune"));
			notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(streamJid, senderJid));
			notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(senderJid));

			notify.data.insert(NDR_POPUP_HTML,Qt::escape(getTagFormated(streamJid, senderJid)));

			FNotifies.insert(FNotifications->appendNotification(notify),senderJid);
		}
	}
}

void UserTuneHandler::onNotificationActivated(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId)) {
		FNotifications->removeNotification(ANotifyId);
	}
}

void UserTuneHandler::onNotificationRemoved(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId)) {
		FNotifies.remove(ANotifyId);
	}
}

void UserTuneHandler::onRosterIndexClipboardMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId == AdvancedDelegateItem::DisplayId && AIndexes.count() == 1 && RosterKinds.contains(AIndexes.first()->kind())) {
		QString song = getTagFormated(AIndexes.first()->data(RDR_STREAM_JID).toString(), AIndexes.first()->data(RDR_PREP_BARE_JID).toString());
		if (!song.isEmpty()) {
			Action *action = new Action(AMenu);
			action->setText(tr("Music info"));
			action->setData(ADR_CLIPBOARD_DATA, song);
			connect(action, SIGNAL(triggered(bool)), SLOT(onCopyToClipboardActionTriggered(bool)));
			AMenu->addAction(action, AG_DEFAULT, true);
		}
	}
}

void UserTuneHandler::onCopyToClipboardActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action) {
		QApplication::clipboard()->setText(action->data(ADR_CLIPBOARD_DATA).toString());
	}
}

void UserTuneHandler::updateFetchers()
{
	if (FMetaDataFetcher) {
		delete FMetaDataFetcher;
		FMetaDataFetcher = nullptr;
	}

	switch (Options::node(OPV_USERTUNE_PLAYER_VER).value().toUInt()) {
#ifdef Q_WS_X11
	case FetcherVer::mprisV1:
		FMetaDataFetcher = new MprisFetcher1(this, Options::node(OPV_USERTUNE_PLAYER_NAME).value().toString());
		break;
	case FetcherVer::mprisV2:
		FMetaDataFetcher = new MprisFetcher2(this, Options::node(OPV_USERTUNE_PLAYER_NAME).value().toString());
		break;
#elif Q_WS_WIN
	// for Windows players...
#endif
	default:
#ifndef QT_NO_DEBUG
		qWarning() << "Not supported fetcher version: " << Options::node(OPV_USERTUNE_PLAYER_VER).value().toUInt();
#endif
		break;
	}

	if (FMetaDataFetcher) {
		connect(FMetaDataFetcher, SIGNAL(trackChanged(UserTuneData)), this, SLOT(onTrackChanged(UserTuneData)));
		connect(FMetaDataFetcher, SIGNAL(statusChanged(PlayerStatus)), this, SLOT(onPlayerSatusChanged(PlayerStatus)));

		FMetaDataFetcher->updateStatus();
	} else {
		onStopPublishing();
	}
}

bool UserTuneHandler::processPEPEvent(const Jid &streamJid, const Stanza &AStanza)
{
	QDomElement replyElem = AStanza.document().firstChildElement(QLatin1String("message"));
	if (!replyElem.isNull()) {
		UserTuneData userSong;
		Jid senderJid = replyElem.attribute("from");
		QDomElement eventElem = replyElem.firstChildElement(QLatin1String("event"));
		if (!eventElem.isNull()) {
			QDomElement itemsElem = eventElem.firstChildElement(QLatin1String("items"));
			if (!itemsElem.isNull()) {
				QDomElement itemElem = itemsElem.firstChildElement(QLatin1String("item"));
				if (!itemElem.isNull()) {
					QDomElement tuneElem = itemElem.firstChildElement(QLatin1String("tune"));
					if (!tuneElem.isNull() && !tuneElem.firstChildElement().isNull()) {
						QDomElement elem;
						elem = tuneElem.firstChildElement(QLatin1String("artist"));
						if (!elem.isNull()) {
							userSong.artist = elem.text();
						}

						elem = tuneElem.firstChildElement(QLatin1String("length"));
						if (!elem.isNull()) {
							userSong.length = elem.text().toUInt();
						}

						elem = tuneElem.firstChildElement(QLatin1String("rating"));
						if (!elem.isNull()) {
							userSong.rating = elem.text().toUInt();
						}

						elem = tuneElem.firstChildElement(QLatin1String("source"));
						if (!elem.isNull()) {
							userSong.source = elem.text();
						}

						elem = tuneElem.firstChildElement(QLatin1String("title"));
						if (!elem.isNull()) {
							userSong.title = elem.text();
						}

						elem = tuneElem.firstChildElement(QLatin1String("track"));
						if (!elem.isNull()) {
							userSong.track = elem.text();\
						}

						elem = tuneElem.firstChildElement(QLatin1String("uri"));
						if (!elem.isNull()) {
							userSong.uri = elem.text();
						}
					}
				}
			}
		}
		setContactTune(streamJid, senderJid, userSong);
	}

	return true;
}

#ifdef Q_WS_X11
void UserTuneHandler::onTrackChanged(UserTuneData data)
{
	if (FTimer.isActive()) {
		FTimer.stop();
	}

	FUserTuneData = data;

	if (FAllowSendPEP) {
		FTimer.start();
	}
}
#endif

#ifdef Q_WS_X11
void UserTuneHandler::onSendPep()
{
	QDomDocument doc(QLatin1String(""));
	QDomElement root = doc.createElement(QLatin1String("item"));
	doc.appendChild(root);

	QDomElement tune = doc.createElement(QLatin1String("tune"));
	root.appendChild(tune);

	ADD_CHILD_ELEMENT (doc, tune, QLatin1String("artist"), FUserTuneData.artist)

	if (FUserTuneData.length > 0) {
		ADD_CHILD_ELEMENT (doc, tune, QLatin1String("length"), QString::number(FUserTuneData.length))
	}

	ADD_CHILD_ELEMENT (doc, tune, QLatin1String("rating"), QString::number(FUserTuneData.rating))
	ADD_CHILD_ELEMENT (doc, tune, QLatin1String("source"), FUserTuneData.source)
	ADD_CHILD_ELEMENT (doc, tune, QLatin1String("title"), FUserTuneData.title)
	ADD_CHILD_ELEMENT (doc, tune, QLatin1String("track"), FUserTuneData.track)

	if (FAllowSendURLInPEP) {
		ADD_CHILD_ELEMENT (doc, tune, QLatin1String("uri"), FUserTuneData.uri.toString())
	}

#ifndef QT_NO_DEBUG
	qDebug() << doc.toString();
#endif
	Jid streamJid;
	int streams_size = FXmppStreams->xmppStreams().size();

	for (int i = 0; i < streams_size; i++) {
		streamJid = FXmppStreams->xmppStreams().at(i)->streamJid();
		FPEPManager->publishItem(streamJid, TUNE_PROTOCOL_URL, root);
	}
}
#endif

#ifdef Q_WS_X11
void UserTuneHandler::onPlayerSatusChanged(PlayerStatus AStatus)
{
	if (AStatus.Play == PlaybackStatus::Stopped) {
		onStopPublishing();
	}
}
#endif

#ifdef Q_WS_X11
void UserTuneHandler::onStopPublishing()
{
	if (FTimer.isActive()) {
		FTimer.stop();
	}

	QDomDocument doc(QLatin1String(""));
	QDomElement root = doc.createElement(QLatin1String("item"));
	doc.appendChild(root);

	QDomElement tune = doc.createElement(QLatin1String("tune"));
	root.appendChild(tune);

	Jid streamJid;
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());

	if (stream) {
		streamJid = stream->streamJid();
		FPEPManager->publishItem(streamJid, TUNE_PROTOCOL_URL, root);
		FContactTune.remove(streamJid);
	} else {
		int streams_size = FXmppStreams->xmppStreams().size();

		for (int i = 0; i < streams_size; i++) {
			streamJid = FXmppStreams->xmppStreams().at(i)->streamJid();
			FPEPManager->publishItem(streamJid, TUNE_PROTOCOL_URL, root);
		}

		FContactTune.clear();
	}
}
#endif
/*
	set music icon to main accaunt
*/
void UserTuneHandler::onSetMainLabel(IXmppStream *AXmppStream)
{
	if (FRostersViewPlugin) {
		IRostersModel *model = FRostersViewPlugin->rostersView()->rostersModel();
		IRosterIndex *index = model ? model->streamIndex(AXmppStream->streamJid()) : nullptr;
		if (index)
			FRostersViewPlugin->rostersView()->insertLabel(FUserTuneLabelId, index);
	}
}

void UserTuneHandler::onLabelsEnabled(const Jid &streamJid)
{
	if (FRostersViewPlugin) {
		IRostersModel *model = FRostersViewPlugin->rostersView()->rostersModel();
		IRosterIndex *index = model ? model->streamIndex(streamJid) : nullptr;
		if (index) {
			FRostersViewPlugin->rostersView()->insertLabel(FUserTuneLabelId, index);
		}

		updateDataHolder(streamJid, Jid());
	}
}

void UserTuneHandler::onUnsetMainLabel(IXmppStream *AXmppStream)
{
	Jid streamJid = AXmppStream->streamJid();
	FContactTune.remove(streamJid);
	if (FRostersViewPlugin && FRostersModel) {
		IRosterIndex *index = FRostersModel->streamIndex(streamJid);
		FRostersViewPlugin->rostersView()->removeLabel(FUserTuneLabelId,index);
		emit rosterDataChanged(index, RDR_USERTUNE);
	}
}

void UserTuneHandler::onContactStateChanged(const Jid &streamJid, const Jid &senderJid, bool AStateOnline)
{
	if (!AStateOnline && FContactTune[streamJid].contains(senderJid.pBare())) {
		FContactTune[streamJid].remove(senderJid.pBare());
	}
}

void UserTuneHandler::setContactTune(const Jid &AStreamJid, const Jid &ASenderJid, const UserTuneData &ASong)
{
	if (FContactTune[AStreamJid].value(ASenderJid.pBare()) != ASong) {
		IRoster *roster = FRosterPlugin ? FRosterPlugin->findRoster(AStreamJid) : nullptr;
		if((roster && roster->rosterItem(ASenderJid).isValid)
				|| AStreamJid.pBare() == ASenderJid.pBare()) {
			if (!ASong.title.isEmpty()) {
				FContactTune[AStreamJid].insert(ASenderJid.pBare(),ASong);
				onShowNotification(AStreamJid, ASenderJid);
			} else {
				FContactTune[AStreamJid].remove(ASenderJid.pBare());
			}
		}
	}
	if (FTuneLabelVisible) {
		updateDataHolder(AStreamJid, ASenderJid);
	}
}

void UserTuneHandler::updateDataHolder(const Jid &streamJid, const Jid &senderJid)
{
	if (FRostersViewPlugin && FRostersModel) {
		static QMultiMap<int,QVariant> findData;
		if (findData.isEmpty()) {
			findData.insert(RDR_PREP_BARE_JID, senderJid.pBare());
			findData.insert(RDR_KIND, RIK_STREAM_ROOT);
			findData.insert(RDR_KIND, RIK_CONTACT);
			findData.insert(RDR_KIND, RIK_CONTACTS_ROOT);
		}

		QList<IRosterIndex *> indexes = FRostersModel->streamIndex(streamJid)->findChilds(findData, true);
		foreach (IRosterIndex *index, indexes) {
			if (FContactTune[streamJid].contains(index->data(RDR_PREP_BARE_JID).toString())) {
				FRostersViewPlugin->rostersView()->insertLabel(FUserTuneLabelId,index);
			} else {
				FRostersViewPlugin->rostersView()->removeLabel(FUserTuneLabelId,index);
			}

			emit rosterDataChanged(index, RDR_USERTUNE);
		}
	}
}

QString UserTuneHandler::getTagFormated(const Jid &streamJid, const Jid &senderJid) const
{
	return getTagFormated(FContactTune[streamJid].value(senderJid.pBare(), UserTuneData()));
}

QString UserTuneHandler::getTagFormated(const UserTuneData &AUserData) const
{
	if (AUserData.isEmpty()) {
		return QString();
	}

	QString tagsLine = FFormatTag;
	// TODO: переделать, все в один проход и не оставлять разделителей
	tagsLine.replace(QLatin1String("%A"), AUserData.artist);
	tagsLine.replace(QLatin1String("%L"), secondsToString(AUserData.length));
	tagsLine.replace(QLatin1String("%R"), QString::number(AUserData.rating)); // ★☆✮
	tagsLine.replace(QLatin1String("%S"), AUserData.source);
	tagsLine.replace(QLatin1String("%T"), AUserData.title);
	tagsLine.replace(QLatin1String("%N"), AUserData.track);
	tagsLine.replace(QLatin1String("%U"), AUserData.uri.toString());

	return tagsLine;
}

void UserTuneHandler::onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips)
{
	if (ALabelId == FUserTuneLabelId
			|| (ALabelId == AdvancedDelegateItem::DisplayId && RosterKinds.contains(AIndex->kind()))) {
		Jid streamJid = AIndex->data(RDR_FULL_JID).toString();
		Jid senderJid = AIndex->data(RDR_PREP_BARE_JID).toString();

		if (FContactTune[streamJid].contains(senderJid.pBare())) {
			QString formatedString = getTagFormated(streamJid, senderJid).replace(QLatin1String("\n"), QLatin1String("<br />"));
			QString toolTip = QString("%1 <div style='margin-left:10px;'>%2</div>").arg(tr("Listen:")).arg(Qt::escape(formatedString));
			AToolTips.insert(RTTO_USERTUNE, toolTip);
		}
	}
}

void UserTuneHandler::onApplicationQuit()
{
	FPEPManager->removeNodeHandler(FHandlerId);
}

Q_EXPORT_PLUGIN2(plg_pepmanager, UserTuneHandler)
