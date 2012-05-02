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
#include "usertuneoptions.h"

#include "definitions.h"

#define ADD_CHILD_ELEMENT(document, root_element, child_name, child_data) \
{ \
    QDomElement tag = (document).createElement(child_name); \
    QDomText text = (document).createTextNode(child_data); \
    tag.appendChild(text); \
    (root_element).appendChild(tag); \
}

#define TUNE_PROTOCOL_URL "http://jabber.org/protocol/tune"
#define TUNE_NOTIFY_PROTOCOL_URL "http://jabber.org/protocol/tune+notify"

UserTuneHandler::UserTuneHandler() :
    FPEPManager(NULL),
    FServiceDiscovery(NULL),
    FXmppStreams(NULL),
    FOptionsManager(NULL),
    FMprisFetcher(NULL)
{

}

UserTuneHandler::~UserTuneHandler()
{

}

void UserTuneHandler::pluginInfo(IPluginInfo *APluginInfo)
{
    APluginInfo->name = tr("User Tune Handler");
    APluginInfo->description = tr("Allows hadle user tunes");
    APluginInfo->version = "0.9.3";
    APluginInfo->author = "Crying Angel";
    APluginInfo->homePage = "http://www.vacuum-im.org";
    APluginInfo->dependences.append(PEPMANAGER_UUID);
    APluginInfo->dependences.append(SERVICEDISCOVERY_UUID);
    APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool UserTuneHandler::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
    AInitOrder=500;

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

    plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
    if (plugin)
    {
        FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
    }

    plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
    if (plugin)
    {
        FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
    }

    plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
    if (plugin)
    {
        FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
        if (FRostersViewPlugin)
        {
            connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexToolTips(IRosterIndex *, int, QMultiMap<int,QString> &)),
                    SLOT(onRosterIndexToolTips(IRosterIndex *, int, QMultiMap<int,QString> &)));
        }
    }

    plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
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
    handlerId = FPEPManager->insertNodeHandler(TUNE_PROTOCOL_URL, this);

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
    Options::setDefaultValue(OPV_UT_SHOW_ROSTER_LABEL,false);
    Options::setDefaultValue(OPV_UT_TAG_FORMAT,"%T - %A - %S");
#ifdef Q_WS_X11
    Options::setDefaultValue(OPV_UT_PLAYER_NAME,"amarok");
#elif Q_WS_WIN
    // TODO: сделать для windows
    Options::setDefaultValue(OPV_UT_PLAYER_NAME,"");
#endif
#ifdef Q_WS_X11
    Options::setDefaultValue(OPV_UT_PLAYER_VER,mprisV1);
#elif Q_WS_WIN
    Options::setDefaultValue(OPV_UT_PLAYER_VER,"");
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
        widgets.insertMulti(OWO_USERTUNE, new UserTuneOptions(AParent));
    }
    return widgets;
}

void UserTuneHandler::onOptionsOpened()
{
    onOptionsChanged(Options::node(OPV_UT_SHOW_ROSTER_LABEL));
    onOptionsChanged(Options::node(OPV_UT_TAG_FORMAT));

    updateFetchers();
}

void UserTuneHandler::onOptionsChanged(const OptionsNode &ANode)
{
    if (ANode.path() == OPV_UT_SHOW_ROSTER_LABEL)
    {
        setContactLabel();
    }
    else if (ANode.path() == OPV_UT_TAG_FORMAT)
    {
        FFormatTag = Options::node(OPV_UT_TAG_FORMAT).value().toString();
    }
    else if (ANode.path() == OPV_UT_PLAYER_NAME)
    {
        FMprisFetcher->onPlayerNameChange(Options::node(OPV_UT_PLAYER_NAME).value().toString());
    }
    else if (ANode.path() == OPV_UT_PLAYER_VER)
    {
        updateFetchers();
    }
}

void UserTuneHandler::onShowNotification(const Jid &AStreamJid, const Jid &AContactJid)
{
    if (FNotifications && FNotifications->notifications().isEmpty() && FContactTune.contains(AContactJid))
    {
        INotification notify;
        notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_USERTUNE);
        if ((notify.kinds & INotification::PopupWindow) > 0)
        {
            notify.typeId = NNT_USERTUNE;
            notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERTUNE_MUSIC));
            notify.data.insert(NDR_POPUP_CAPTION,tr("User Tune Notification"));
            notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(AStreamJid, AContactJid));
            notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(AContactJid));

            notify.data.insert(NDR_POPUP_HTML,getTagFormat(AContactJid));

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

void UserTuneHandler::updateFetchers()
{
    if (FMprisFetcher)
    {
        delete FMprisFetcher;
        FMprisFetcher = NULL;
    }

    switch (Options::node(OPV_UT_PLAYER_VER).value().toUInt()) {
#ifdef Q_WS_X11
    case mprisV1:
        FMprisFetcher = new MprisFetcher1(this, Options::node(OPV_UT_PLAYER_NAME).value().toString());
        break;
    case mprisV2:
        FMprisFetcher = new MprisFetcher2(this, Options::node(OPV_UT_PLAYER_NAME).value().toString());
        break;
#elif Q_WS_WIN
    // for Windows players...
#endif
    case mprisNone:
        // disable send data, only recive
    default:
        break;
    }

    if (FMprisFetcher)
    {
        connect(FMprisFetcher, SIGNAL(trackChanged(UserTuneData)), this, SLOT(onTrackChanged(UserTuneData)));
        connect(FMprisFetcher, SIGNAL(statusChanged(PlayerStatus)), this, SLOT(onPlayerSatusChanged(PlayerStatus)));
    }
}

bool UserTuneHandler::processPEPEvent(const Jid &AStreamJid, const Stanza &AStanza)
{
    Q_UNUSED(AStreamJid)
    Jid senderJid;
    UserTuneData userSong;

    QDomElement replyElem = AStanza.document().firstChildElement("message");

    if (!replyElem.isNull())
    {
        senderJid = replyElem.attribute("from");
        QDomElement eventElem = replyElem.firstChildElement("event");

        if (!eventElem.isNull())
        {
            QDomElement itemsElem = eventElem.firstChildElement("items");

            if (!itemsElem.isNull())
            {
                QDomElement itemElem = itemsElem.firstChildElement("item");

                if (!itemElem.isNull() && !itemElem.firstChildElement().isNull())
                {
                    QDomElement tuneElem = itemElem.firstChildElement("tune");

                    if (!tuneElem.isNull())
                    {
                        QDomElement elem;
                        elem = tuneElem.firstChildElement("artist");
                        if (!elem.isNull())
                        {
                            userSong.artist = elem.text();
                        }

                        elem = tuneElem.firstChildElement("length");
                        if (!elem.isNull())
                        {
                            userSong.length = elem.text().toUInt();
                        }

                        elem = tuneElem.firstChildElement("rating");
                        if (!elem.isNull())
                        {
                            userSong.rating = elem.text().toUInt();
                        }

                        elem = tuneElem.firstChildElement("source");
                        if (!elem.isNull())
                        {
                            userSong.source = elem.text();
                        }

                        elem = tuneElem.firstChildElement("title");
                        if (!elem.isNull())
                        {
                            userSong.title = elem.text();
                        }

                        elem = tuneElem.firstChildElement("track");
                        if (!elem.isNull())
                        {
                            userSong.track = elem.text();
                        }

                        elem = tuneElem.firstChildElement("uri");
                        if (!elem.isNull())
                        {
                            userSong.uri = elem.text();
                        }
                    }
                    else // !tuneElem.isNull() && !itemElem.firstChildElement().isNull()
                    {
                        if (Options::node(OPV_UT_SHOW_ROSTER_LABEL).value().toBool())
                        {
                            QMultiMap<int, QVariant> findData;
                            findData.insert(RDR_TYPE,RIT_CONTACT);
                            findData.insert(RDR_PREP_BARE_JID,senderJid.pBare());

                            foreach (IRosterIndex *index, FRostersModel->rootIndex()->findChilds(findData,true))
                            {
                                FRostersViewPlugin->rostersView()->removeLabel(FUserTuneLabelId,index);
                            }
                        }
                    }
                }
            }
        }
    }

    setContactTune(AStreamJid, senderJid, userSong);

    return true;
}

void UserTuneHandler::onTrackChanged(UserTuneData data)
{
    QDomDocument doc("");
    QDomElement root = doc.createElement("item");
    doc.appendChild(root);

    QDomElement tune = doc.createElement("tune");
    root.appendChild(tune);

    ADD_CHILD_ELEMENT (doc, tune, "artist", data.artist)
    ADD_CHILD_ELEMENT (doc, tune, "length", QString::number(data.length))
    ADD_CHILD_ELEMENT (doc, tune, "rating", QString::number(data.rating))
    ADD_CHILD_ELEMENT (doc, tune, "source", data.source)
    ADD_CHILD_ELEMENT (doc, tune, "title", data.title)
    ADD_CHILD_ELEMENT (doc, tune, "track", data.track)
    ADD_CHILD_ELEMENT (doc, tune, "uri", data.uri.toString())

#ifndef QT_NO_DEBUG
    qDebug() << doc.toString();
#endif
    Jid streamJid;

    for (int i = 0; i < FXmppStreams->xmppStreams().size(); i++)
    {
        streamJid = FXmppStreams->xmppStreams().at(i)->streamJid();
        FPEPManager->publishItem(streamJid, TUNE_PROTOCOL_URL, root);
    }
}

void UserTuneHandler::onPlayerSatusChanged(PlayerStatus status)
{
    if (status.Play == PSStopped) {
        onStopPublishing();
    }
}

void UserTuneHandler::onStopPublishing()
{
    QDomDocument doc("");
    QDomElement root = doc.createElement("item");
    doc.appendChild(root);

    QDomElement tune = doc.createElement("tune");
    root.appendChild(tune);

    Jid streamJid;
    int streams_size = FXmppStreams->xmppStreams().size();

    for (int i = 0; i < streams_size; i++)
    {
        streamJid = FXmppStreams->xmppStreams().at(i)->streamJid();
        FPEPManager->publishItem(streamJid, TUNE_PROTOCOL_URL, root);
    }
}

// TODO: выводить значок рядом с замком
//void UserTuneHandler::onSetMainLabel()
//{
//    FRostersViewPlugin->rostersView()->insertLabel(FUserTuneLabelId, FRostersModel->streamRoot(AStreamJid));
//}

void UserTuneHandler::setContactTune(const Jid &AStreamJid, const Jid &AContactJid, const UserTuneData &ASong)
{
    UserTuneData data = FContactTune.value(AContactJid);
    if (data != ASong)
    {
        if (!ASong.isEmpty())
            FContactTune.insert(AContactJid,ASong);
        else
            FContactTune.remove(AContactJid);
    }
    setContactLabel();
    onShowNotification(AStreamJid, AContactJid);
}

void UserTuneHandler::setContactLabel()
{
    foreach (const Jid &AContactJid, FContactTune.keys())
    {
        QMultiMap<int, QVariant> findData;
        findData.insert(RDR_TYPE,RIT_CONTACT);
        findData.insert(RDR_PREP_BARE_JID,AContactJid.pBare());

        foreach (IRosterIndex *index, FRostersModel->rootIndex()->findChilds(findData,true))
        {
         // TODO: сделать удаление при изменении параметра OPV_UT_SHOW_ROSTER_LABEL; проверку вынести за цикл
            if (Options::node(OPV_UT_SHOW_ROSTER_LABEL).value().toBool()
                    && (AContactJid.pBare() == index->data(RDR_PREP_BARE_JID).toString())
                    && FContactTune.contains(AContactJid))
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

QString UserTuneHandler::getTagFormat(const Jid &AContactJid)
{
    FTag = FFormatTag;

    FTag.replace(QString("%A"), Qt::escape(FContactTune.value(AContactJid).artist));
    FTag.replace(QString("%L"), Qt::escape(secToTime(FContactTune.value(AContactJid).length)));
    FTag.replace(QString("%R"), Qt::escape(QString::number(FContactTune.value(AContactJid).rating)));
    FTag.replace(QString("%S"), Qt::escape(FContactTune.value(AContactJid).source));
    FTag.replace(QString("%T"), Qt::escape(FContactTune.value(AContactJid).title));
    FTag.replace(QString("%N"), Qt::escape(FContactTune.value(AContactJid).track));
    FTag.replace(QString("%U"), Qt::escape(FContactTune.value(AContactJid).uri.toString()));

    return FTag;
}

void UserTuneHandler::onRosterIndexToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips)
{
    if (ALabelId==RLID_DISPLAY || ALabelId==FUserTuneLabelId)
    {
        Jid contactJid = AIndex->data(RDR_PREP_BARE_JID).toString();
        if (FContactTune.contains(contactJid))
        {
            QString tip = QString("%1 <div style='margin-left:10px;'>%2</div>").arg(tr("Listen:")).arg(getTagFormat(contactJid).replace("\n","<br />"));
            AToolTips.insert(RTTO_USERTUNE,tip);
        }
    }
}

void UserTuneHandler::onApplicationQuit()
{
    FPEPManager->removeNodeHandler(handlerId);
}

Q_EXPORT_PLUGIN2(plg_pepmanager, UserTuneHandler)
