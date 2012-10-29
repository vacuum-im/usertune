#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif

#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/rosterindextyperole.h>

#include <definitions/optionvalues.h>

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

#define TUNE_PROTOCOL_URL "http://jabber.org/protocol/tune"
#define TUNE_NOTIFY_PROTOCOL_URL "http://jabber.org/protocol/tune+notify"
#define PEP_SEND_DELAY 5*1000 // delay befo send pep to prevent a large number of updates when a user is skipping through tracks

UserTuneHandler::UserTuneHandler() :
	FPEPManager(NULL),
	FServiceDiscovery(NULL),
	FXmppStreams(NULL),
	FOptionsManager(NULL)
  #ifdef Q_WS_X11
  , FMetaDataFetcher(NULL),
	FMessageWidgets(NULL),
	FMultiUserChatPlugin(NULL)
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
	APluginInfo->version = "1.0.0";
	APluginInfo->author = "Crying Angel";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(PEPMANAGER_UUID);
	APluginInfo->dependences.append(SERVICEDISCOVERY_UUID);
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool UserTuneHandler::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	AInitOrder = 500;

	IPlugin *plugin;

	plugin = APluginManager->pluginInterface("IPEPManager").value(0,NULL);
	if (!plugin) return false;

	FPEPManager = qobject_cast<IPEPManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (!plugin) return false;
	FServiceDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (!plugin) return false;

	FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
	connect(FXmppStreams->instance(), SIGNAL(opened(IXmppStream *)), this, SLOT(onSetMainLabel(IXmppStream*)));
	connect(FXmppStreams->instance(), SIGNAL(closed(IXmppStream *)), this, SLOT(onUnsetMainLabel(IXmppStream*)));

	int streams_size = FXmppStreams->xmppStreams().size();
	for (int i = 0; i < streams_size; i++)
	{
		connect(FXmppStreams->xmppStreams().at(i)->instance(), SIGNAL(aboutToClose()), this, SLOT(onStopPublishing()));
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	Q_ASSERT(plugin);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	Q_ASSERT(plugin);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	Q_ASSERT(plugin);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexToolTips(IRosterIndex *, int, QMultiMap<int,QString> &)),
					SLOT(onRosterIndexToolTips(IRosterIndex *, int, QMultiMap<int,QString> &)));
		}
	}
#ifdef Q_WS_X11
	// player manage (/play, /pause etc command)
	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	Q_ASSERT(plugin);
	if (plugin)
	{
		IMessageProcessor *messageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
		Q_ASSERT(messageProcessor);
		if (messageProcessor)
			messageProcessor->insertMessageEditor(MEO_USERTUNE, this);
	}

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	Q_ASSERT(plugin);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMultiUserChatPlugin").value(0,NULL);
	Q_ASSERT(plugin);
	if (plugin)
	{
		FMultiUserChatPlugin = qobject_cast<IMultiUserChatPlugin *>(plugin->instance());
	}
#endif
	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	Q_ASSERT(plugin);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	Q_ASSERT(plugin);
	if (plugin)
	{
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

	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = NTO_USERTUNE_NOTIFY;
		notifyType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERTUNE_MUSIC);
		notifyType.title = tr("When reminding of contact playing music");
		notifyType.kindMask = INotification::PopupWindow;
		notifyType.kindDefs = notifyType.kindMask;
		FNotifications->registerNotificationType(NNT_USERTUNE,notifyType);
	}

	if (FRostersViewPlugin)
	{
		IRostersLabel label;
		label.order = RLO_USERTUNE;
		label.value = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERTUNE_MUSIC);
		FUserTuneLabelId = FRostersViewPlugin->rostersView()->registerLabel(label);
	}

	return true;
}

bool UserTuneHandler::initSettings()
{
	Options::setDefaultValue(OPV_UT_SHOW_ROSTER_LABEL,true);
	Options::setDefaultValue(OPV_UT_ALLOW_SEND_MUSIC_INFO,true);
	Options::setDefaultValue(OPV_UT_NOT_ALLOW_SEND_URL_INFO,false);
	Options::setDefaultValue(OPV_UT_TAG_FORMAT,QLatin1String("%T - %A - %S"));
#ifdef Q_WS_X11
	Options::setDefaultValue(OPV_UT_PLAYER_NAME,QLatin1String("amarok"));
	Options::setDefaultValue(OPV_UT_PLAYER_VER,FetcherVer::mprisV1);
#elif Q_WS_WIN
	// TODO: сделать для windows
	Options::setDefaultValue(OPV_UT_PLAYER_NAME,QLatin1String(""));
	Options::setDefaultValue(OPV_UT_PLAYER_VER,FetchrVer::fetcherNone);
#endif

	if (FOptionsManager)
	{
		IOptionsDialogNode dnode = { ONO_USERTUNE, OPN_USERTUNE, tr("User Tune"), MNI_USERTUNE_MUSIC };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}

	return true;
}

QMultiMap<int, IOptionsWidget *> UserTuneHandler::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FOptionsManager && ANodeId==OPN_USERTUNE)
	{
#ifdef Q_WS_X11
		widgets.insertMulti(OWO_USERTUNE, new UserTuneOptions(AParent));
#elif Q_WS_WIN
		widgets.insertMulti(OWO_USERTUNE, FOptionsManager->optionsNodeWidget(Options::node(OPV_UT_SHOW_ROSTER_LABEL),tr("Show music icon in roster"),AParent));
		widgets.insertMulti(OWO_USERTUNE, FOptionsManager->optionsNodeWidget(Options::node(OPV_UT_TAG_FORMAT),tr("Tag format:"),AParent));
		widgets.insertMulti(OWO_USERTUNE, FOptionsManager->optionsNodeWidget(Options::node(OPV_UT_PLAYER_NAME),tr("Player name:"),AParent));
#endif
	}
	return widgets;
}

void UserTuneHandler::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_UT_SHOW_ROSTER_LABEL));
	onOptionsChanged(Options::node(OPV_UT_TAG_FORMAT));
#ifdef Q_WS_X11
	onOptionsChanged(Options::node(OPV_UT_ALLOW_SEND_MUSIC_INFO));
	onOptionsChanged(Options::node(OPV_UT_NOT_ALLOW_SEND_URL_INFO));
	updateFetchers();
#endif
}

void UserTuneHandler::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_UT_SHOW_ROSTER_LABEL)
	{
		if (Options::node(OPV_UT_SHOW_ROSTER_LABEL).value().toBool())
		{
			setContactLabel();
		}
		else
		{
			unsetContactLabel();
		}
	}
	else if (ANode.path() == OPV_UT_TAG_FORMAT)
	{
		FFormatTag = Options::node(OPV_UT_TAG_FORMAT).value().toString();
	}
#ifdef Q_WS_X11
	else if (ANode.path() == OPV_UT_ALLOW_SEND_MUSIC_INFO)
	{
		if (!(FAllowSendPEP = Options::node(OPV_UT_ALLOW_SEND_MUSIC_INFO).value().toBool()))
		{
			onStopPublishing();
		}
	}
	else if (ANode.path() == OPV_UT_NOT_ALLOW_SEND_URL_INFO)
	{
		FAllowSendURLInPEP = !Options::node(OPV_UT_NOT_ALLOW_SEND_URL_INFO).value().toBool();
	}
	else if (ANode.path() == OPV_UT_PLAYER_VER)
	{
		updateFetchers();
	}
	else if (ANode.path() == OPV_UT_PLAYER_NAME)
	{
		if (FMetaDataFetcher)
		{
			FMetaDataFetcher->onPlayerNameChange(Options::node(OPV_UT_PLAYER_NAME).value().toString());
		}
	}
#endif
}

#ifdef Q_WS_X11
// for player manage
bool UserTuneHandler::messageReadWrite(int AOrder, const Jid &AStreamJid, Message &AMessage, int ADirection)
{
	Q_UNUSED(AStreamJid);
	Q_ASSERT(FMetaDataFetcher);

	if (!AMessage.body().startsWith('/'))
	{
		return false;
	}

	bool breakNextCheck = false;

	if (FMetaDataFetcher && AOrder == MEO_USERTUNE && ADirection == IMessageProcessor::MessageOut)
	{
		bool clearWidget = false;
#if QT_VERSION >= 0x040800
		QStringRef body = AMessage.body().midRef(1);
#else
		QString body = AMessage.body().mid(1);
#endif

		if (body.startsWith(QLatin1String("play"), Qt::CaseInsensitive) ||
				body.startsWith(QLatin1String("pause"), Qt::CaseInsensitive))
		{
			FMetaDataFetcher->playerPlay();
			breakNextCheck = clearWidget = true;
		}
		else if (body.startsWith(QLatin1String("stop"), Qt::CaseInsensitive))
		{
			FMetaDataFetcher->playerStop();
			breakNextCheck = clearWidget = true;
		}
		else if (body.startsWith(QLatin1String("next"), Qt::CaseInsensitive))
		{
			FMetaDataFetcher->playerNext();
			breakNextCheck = clearWidget = true;
		}
		else if (body.startsWith(QLatin1String("prev"), Qt::CaseInsensitive))
		{
			FMetaDataFetcher->playerPrev();
			breakNextCheck = clearWidget = true;
		}
		else if (body.startsWith(QLatin1String("np"), Qt::CaseInsensitive))
		{
			AMessage.setBody(getTagFormated(FUserTuneData));
			breakNextCheck = !(clearWidget = true);
		}

		if (clearWidget) {
			IEditWidget *widget;

			switch (AMessage.type()) {
			case Message::Chat:
				widget = FMessageWidgets->findChatWindow(AStreamJid,AMessage.stanza().to())->editWidget();
				break;
			case Message::GroupChat:
				widget = FMultiUserChatPlugin->multiChatWindow(AStreamJid,AMessage.stanza().to())->editWidget();
				break;
			default:
				widget = NULL;
				break;
			}
			Q_ASSERT(widget);
			if (widget)
				widget->clearEditor();
		}
	}

	return breakNextCheck;
}
#endif

void UserTuneHandler::onShowNotification(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (FNotifications && FContactTune.contains(AContactJid))
	{
		INotification notify;
		notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_USERTUNE);
		if ((notify.kinds & INotification::PopupWindow) > 0)
		{
			notify.typeId = NNT_USERTUNE;
			notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERTUNE_MUSIC));
			notify.data.insert(NDR_TOOLTIP,tr("User Tune Notification"));
			notify.data.insert(NDR_POPUP_CAPTION,tr("User Tune"));
			notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(AStreamJid, AContactJid));
			notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(AContactJid));

			notify.data.insert(NDR_POPUP_HTML,Qt::escape(getTagFormated(AContactJid)));

			FNotifies.insert(FNotifications->appendNotification(notify),AContactJid);
		}
	}
}

void UserTuneHandler::onNotificationActivated(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		FNotifications->removeNotification(ANotifyId);
	}
}

void UserTuneHandler::onNotificationRemoved(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		FNotifies.remove(ANotifyId);
	}
}
#ifdef Q_WS_X11
void UserTuneHandler::updateFetchers()
{
	if (FMetaDataFetcher)
	{
		delete FMetaDataFetcher;
		FMetaDataFetcher = NULL;
	}

	switch (Options::node(OPV_UT_PLAYER_VER).value().toUInt()) {
#ifdef Q_WS_X11
	case FetcherVer::mprisV1:
		FMetaDataFetcher = new MprisFetcher1(this, Options::node(OPV_UT_PLAYER_NAME).value().toString());
		break;
	case FetcherVer::mprisV2:
		FMetaDataFetcher = new MprisFetcher2(this, Options::node(OPV_UT_PLAYER_NAME).value().toString());
		break;
#elif Q_WS_WIN
	// for Windows players...
#endif
	default:
		break;
	}

	if (FMetaDataFetcher)
	{
		connect(FMetaDataFetcher, SIGNAL(trackChanged(UserTuneData)), this, SLOT(onTrackChanged(UserTuneData)));
		connect(FMetaDataFetcher, SIGNAL(statusChanged(PlayerStatus)), this, SLOT(onPlayerSatusChanged(PlayerStatus)));
	}
	else
	{
		onStopPublishing();
	}
}
#endif
bool UserTuneHandler::processPEPEvent(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid)
	Jid senderJid;
	UserTuneData userSong;

	QDomElement replyElem = AStanza.document().firstChildElement(QLatin1String("message"));

	if (!replyElem.isNull())
	{
		senderJid = replyElem.attribute("from");
		QDomElement eventElem = replyElem.firstChildElement(QLatin1String("event"));

		if (!eventElem.isNull())
		{
			QDomElement itemsElem = eventElem.firstChildElement(QLatin1String("items"));

			if (!itemsElem.isNull())
			{
				QDomElement itemElem = itemsElem.firstChildElement(QLatin1String("item"));

				if (!itemElem.isNull())
				{
					QDomElement tuneElem = itemElem.firstChildElement(QLatin1String("tune"));

					if (!tuneElem.isNull() && !tuneElem.firstChildElement().isNull())
					{
						QDomElement elem;
						elem = tuneElem.firstChildElement(QLatin1String("artist"));
						if (!elem.isNull())
						{
							userSong.artist = elem.text();
						}

						elem = tuneElem.firstChildElement(QLatin1String("length"));
						if (!elem.isNull())
						{
							userSong.length = elem.text().toUInt();
						}

						elem = tuneElem.firstChildElement(QLatin1String("rating"));
						if (!elem.isNull())
						{
							userSong.rating = elem.text().toUInt();
						}

						elem = tuneElem.firstChildElement(QLatin1String("source"));
						if (!elem.isNull())
						{
							userSong.source = elem.text();
						}

						elem = tuneElem.firstChildElement(QLatin1String("title"));
						if (!elem.isNull())
						{
							userSong.title = elem.text();
						}

						elem = tuneElem.firstChildElement(QLatin1String("track"));
						if (!elem.isNull())
						{
							userSong.track = elem.text();
						}

						elem = tuneElem.firstChildElement(QLatin1String("uri"));
						if (!elem.isNull())
						{
							userSong.uri = elem.text();
						}

						setContactLabel(senderJid);
						onShowNotification(AStreamJid, senderJid);
					}
					else // !tuneElem.isNull() && !tuneElem.firstChildElement().isNull()
					{
						unsetContactLabel(senderJid);
					}
				}
			}
		}
	}

	setContactTune(senderJid, userSong);

	return true;
}

#ifdef Q_WS_X11
void UserTuneHandler::onTrackChanged(UserTuneData data)
{
	if (FTimer.isActive())
	{
		FTimer.stop();
	}

	FUserTuneData = data;

	if (FAllowSendPEP)
	{
		FTimer.start();
	}
}

void UserTuneHandler::onSendPep()
{
	QDomDocument doc(QLatin1String(""));
	QDomElement root = doc.createElement(QLatin1String("item"));
	doc.appendChild(root);

	QDomElement tune = doc.createElement(QLatin1String("tune"));
	root.appendChild(tune);

	ADD_CHILD_ELEMENT (doc, tune, QLatin1String("artist"), FUserTuneData.artist)

			if (FUserTuneData.length > 0)
	{
		ADD_CHILD_ELEMENT (doc, tune, QLatin1String("length"), QString::number(FUserTuneData.length))
	}

	ADD_CHILD_ELEMENT (doc, tune, QLatin1String("rating"), QString::number(FUserTuneData.rating))
			ADD_CHILD_ELEMENT (doc, tune, QLatin1String("source"), FUserTuneData.source)
			ADD_CHILD_ELEMENT (doc, tune, QLatin1String("title"), FUserTuneData.title)
			ADD_CHILD_ELEMENT (doc, tune, QLatin1String("track"), FUserTuneData.track)

	if (FAllowSendURLInPEP)
	{
		ADD_CHILD_ELEMENT (doc, tune, QLatin1String("uri"), FUserTuneData.uri.toString())
	}

#ifndef QT_NO_DEBUG
	qDebug() << doc.toString();
#endif
	Jid streamJid;
	int streams_size = FXmppStreams->xmppStreams().size();

	for (int i = 0; i < streams_size; i++)
	{
		streamJid = FXmppStreams->xmppStreams().at(i)->streamJid();
		FPEPManager->publishItem(streamJid, TUNE_PROTOCOL_URL, root);
	}
}

void UserTuneHandler::onPlayerSatusChanged(PlayerStatus status)
{
	if (status.Play == PlayingStatus::Stopped)
	{
		onStopPublishing();
	}
}

void UserTuneHandler::onStopPublishing()
{
	QDomDocument doc(QLatin1String(""));
	QDomElement root = doc.createElement(QLatin1String("item"));
	doc.appendChild(root);

	QDomElement tune = doc.createElement(QLatin1String("tune"));
	root.appendChild(tune);

	Jid streamJid;
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());

	if (stream != NULL)
	{
		streamJid = stream->streamJid();
		FPEPManager->publishItem(streamJid, TUNE_PROTOCOL_URL, root);
		FContactTune.remove(streamJid);
	}
	else
	{
		int streams_size = FXmppStreams->xmppStreams().size();

		for (int i = 0; i < streams_size; i++)
		{
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
	IRosterIndex *index = FRostersModel->streamRoot(AXmppStream->streamJid());
	if (index!=NULL)
	{
		FRostersViewPlugin->rostersView()->insertLabel(FUserTuneLabelId, index);
	}
}

void UserTuneHandler::onUnsetMainLabel(IXmppStream *AXmppStream)
{
	IRosterIndex *index = FRostersModel->streamRoot(AXmppStream->streamJid());
	if (index!=NULL)
	{
		FRostersViewPlugin->rostersView()->removeLabel(FUserTuneLabelId, index);
	}
}

void UserTuneHandler::setContactTune(const Jid &AContactJid, const UserTuneData &ASong)
{
	if (FContactTune.value(AContactJid) != ASong)
	{
		if (!ASong.isEmpty())
		{
			FContactTune.insert(AContactJid,ASong);
		}
		else
		{
			FContactTune.remove(AContactJid);
		}
	}
}

/*
	set music icon to contact
*/
void UserTuneHandler::setContactLabel()
{
	if (Options::node(OPV_UT_SHOW_ROSTER_LABEL).value().toBool())
	{
		foreach (const Jid &contactJid, FContactTune.keys())
		{
			QMultiMap<int, QVariant> findData;
			findData.insert(RDR_TYPE,RIT_CONTACT);
			findData.insert(RDR_PREP_BARE_JID,contactJid.pBare());

			foreach (IRosterIndex *index, FRostersModel->rootIndex()->findChilds(findData,true))
			{
				if (contactJid.pBare() == index->data(RDR_PREP_BARE_JID).toString())
				{
					FRostersViewPlugin->rostersView()->insertLabel(FUserTuneLabelId,index);
				}
				else
				{
					FRostersViewPlugin->rostersView()->removeLabel(FUserTuneLabelId,index);
				}
			}
		}
	}
}

void UserTuneHandler::setContactLabel(const Jid &AContactJid)
{
	if (Options::node(OPV_UT_SHOW_ROSTER_LABEL).value().toBool())
	{
		QMultiMap<int, QVariant> findData;
		findData.insert(RDR_TYPE,RIT_CONTACT);
		findData.insert(RDR_PREP_BARE_JID,AContactJid.pBare());

		foreach (IRosterIndex *index, FRostersModel->rootIndex()->findChilds(findData,true))
		{
			if (AContactJid.pBare() == index->data(RDR_PREP_BARE_JID).toString())
			{
				FRostersViewPlugin->rostersView()->insertLabel(FUserTuneLabelId,index);
			}
			else
			{
				FRostersViewPlugin->rostersView()->removeLabel(FUserTuneLabelId,index);
			}
		}
	}
}

void UserTuneHandler::unsetContactLabel()
{
	if (Options::node(OPV_UT_SHOW_ROSTER_LABEL).value().toBool())
	{
		foreach (const Jid &AContactJid, FContactTune.keys())
		{
			QMultiMap<int, QVariant> findData;
			findData.insert(RDR_TYPE,RIT_CONTACT);
			findData.insert(RDR_PREP_BARE_JID,AContactJid.pBare());

			foreach (IRosterIndex *index, FRostersModel->rootIndex()->findChilds(findData,true))
			{
				FRostersViewPlugin->rostersView()->removeLabel(FUserTuneLabelId,index);
			}
		}
	}
}

void UserTuneHandler::unsetContactLabel(const Jid &AContactJid)
{
	if (Options::node(OPV_UT_SHOW_ROSTER_LABEL).value().toBool())
	{
		QMultiMap<int, QVariant> findData;
		findData.insert(RDR_TYPE,RIT_CONTACT);
		findData.insert(RDR_PREP_BARE_JID,AContactJid.pBare());

		foreach (IRosterIndex *index, FRostersModel->rootIndex()->findChilds(findData,true))
		{
			FRostersViewPlugin->rostersView()->removeLabel(FUserTuneLabelId,index);
		}
	}
}

QString UserTuneHandler::getTagFormated(const Jid &AContactJid) const
{
	return getTagFormated(FContactTune.value(AContactJid, UserTuneData()));
}

QString UserTuneHandler::getTagFormated(const UserTuneData &AUserData) const
{
	if (AUserData.isEmpty())
	{
		return QString();
	}

	QString tagsLine = FFormatTag;
	// TODO: переделать, все в один проход и не оставлять разделителей
	tagsLine.replace(QLatin1String("%A"), AUserData.artist);
	tagsLine.replace(QLatin1String("%L"), secondToString(AUserData.length));
	tagsLine.replace(QLatin1String("%R"), QString::number(AUserData.rating)); // ★☆✮
	tagsLine.replace(QLatin1String("%S"), AUserData.source);
	tagsLine.replace(QLatin1String("%T"), AUserData.title);
	tagsLine.replace(QLatin1String("%N"), AUserData.track);
	tagsLine.replace(QLatin1String("%U"), AUserData.uri.toString());

	return tagsLine;
}

void UserTuneHandler::onRosterIndexToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips)
{
	if (ALabelId == RLID_DISPLAY || ALabelId == FUserTuneLabelId)
	{
		Jid contactJid = AIndex->data(RDR_PREP_BARE_JID).toString();
		if (FContactTune.contains(contactJid))
		{
			QString formatedString = getTagFormated(contactJid).replace(QChar('\n'),
																		QLatin1String("<br />"));
			QString toolTip = QString("%1 <div style='margin-left:10px;'>%2</div>").arg(tr("Listen:"))
					.arg(Qt::escape(formatedString));

			AToolTips.insert(RTTO_USERTUNE, toolTip);
		}
	}
}

void UserTuneHandler::onApplicationQuit()
{
	FPEPManager->removeNodeHandler(FHandlerId);
}

Q_EXPORT_PLUGIN2(plg_pepmanager, UserTuneHandler)
