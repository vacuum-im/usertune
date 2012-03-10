#include <QDebug>

#include <definitions/optionvalues.h>
#include <utils/options.h>

#include "usertune.h"

#define TUNE_PROTOCOL_URL "http://jabber.org/protocol/tune"
#define NOTIFY_PROTOCOL_URL "http://jabber.org/protocol/tune+notify"

UserTuneHandler::UserTuneHandler()
{
        FPEPManager = NULL;
        FServiceDiscovery = NULL;
        FXmppStreams = NULL;
        FOptionsManager = NULL;
        FMprisFetcher = new MprisFetcher(this);
}

UserTuneHandler::~UserTuneHandler()
{

}

void UserTuneHandler::pluginInfo(IPluginInfo *APluginInfo)
{
        APluginInfo->name = tr("User Tune Handler");
        APluginInfo->description = tr("Allows hadle user tunes");
        APluginInfo->version = "0.9.beta";
        APluginInfo->author = "Crying Angel";
        APluginInfo->homePage = "http://www.vacuum-im.org";
        APluginInfo->dependences.append(PEPMANAGER_UUID);
        APluginInfo->dependences.append(SERVICEDISCOVERY_UUID);
        APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool UserTuneHandler::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
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

                 plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
                 if (plugin)
                 {
                     FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
                  /*   if (FRosterPlugin)
                     {
                         connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
                             SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
                     } */
                 }


                 plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
                 if (plugin)
                 {
                     FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
                    /* if (FRostersModel)
                     {
                         connect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)),SLOT(onRosterIndexInserted(IRosterIndex *)));
                     } */
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

        FPlayers = FMprisFetcher->getPlayers();
        if (FPlayers.isEmpty()){
     // зачем? Если я запускаю вакуум раньше плеер потом лезть влкючать плагин? Пусть висит, а плеер можно выбрать в настройках
            qWarning() << "No MPRIS capable players detected. Disabling plugin...";
            return false;
        }
//        player = FMprisFetcher->getPlayers().first();
//        FMprisFetcher->setFormat("%artist - %title < %album >");
        FMprisFetcher->setPlayer(FPlayers.first());

        handlerId = FPEPManager->insertNodeHandler(TUNE_PROTOCOL_URL, this);

        IDiscoFeature feature;
        feature.active = true;

        feature.var = TUNE_PROTOCOL_URL;
        FServiceDiscovery->insertDiscoFeature(feature);

        feature.var = NOTIFY_PROTOCOL_URL;
        FServiceDiscovery->insertDiscoFeature(feature);

        QObject::connect(FMprisFetcher, SIGNAL(trackChanged(QVariantMap)), this, SLOT(onTrackChanged(QVariantMap)));

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
                widgets.insertMulti(OWO_USERTUNE, FOptionsManager->optionsNodeWidget(Options::node(OPV_UT_SHOW_ROSTER_LABEL),tr("Show music icon in roster"),AParent));

        }
        return widgets;
}


void UserTuneHandler::onOptionsOpened()
{
        onOptionsChanged(Options::node(OPV_UT_SHOW_ROSTER_LABEL));
}

void UserTuneHandler::onOptionsChanged(const OptionsNode &ANode)
{
        if (ANode.path() == OPV_UT_SHOW_ROSTER_LABEL)
        {
                //true action
        }
        else
        {
               //false action
        }


   /*     else if (ANode.path() == OPV_)
        {

        } */
}

bool UserTuneHandler::processPEPEvent(const Jid &AStreamJid, const Stanza &AStanza)
{

    QString song;
    QString senderJid;

    QDomElement replyElem = AStanza.document().firstChildElement("message");

    if (!replyElem.isNull()) {
        senderJid = replyElem.attribute("from");
        QDomElement eventElem = replyElem.firstChildElement("event");

        if (!eventElem.isNull()) {
            QDomElement itemsElem = eventElem.firstChildElement("items");

            if (!itemsElem.isNull()) {
                QDomElement itemElem = itemsElem.firstChildElement("item");

                if (!itemElem.isNull()) {
                    QDomElement tuneElem = itemElem.firstChildElement("tune");

                    if (!tuneElem.isNull()) {
                        QDomElement artistElem = tuneElem.firstChildElement("artist");

                        if (!artistElem.isNull()) {
                            song = artistElem.text();
                        }

                        QDomElement titleElem = tuneElem.firstChildElement("title");

                        if (!titleElem.isNull()) {
                            song.append(QString(" - %1").arg(titleElem.text()));
                        }

                        QDomElement sourceElem = tuneElem.firstChildElement("source");

                        if (!sourceElem.isNull()) {
                            song.append(QString(" <%1>").arg(sourceElem.text()));
                        }
                    }
                }
            }
        }
    }

    qDebug() << senderJid << " listen " << song;

    setContactTune(senderJid, song);

    return true;
}

void UserTuneHandler::onTrackChanged(QVariantMap trackInfo)
{
    QDomDocument doc("");
    QDomElement root = doc.createElement("item");
    doc.appendChild(root);

    QDomElement tune = doc.createElement("tune");
    root.appendChild(tune);

    QDomElement tag = doc.createElement("artist");
    tune.appendChild(tag);

    QDomText t1 = doc.createTextNode(trackInfo.value("artist").toString());
    tag.appendChild(t1);

    QDomElement title = doc.createElement("title");
    tune.appendChild(title);

    QDomText t2 = doc.createTextNode(trackInfo.value("title").toString());
    title.appendChild(t2);

    QDomElement album = doc.createElement("source");
    tune.appendChild(album);

    QDomText t3 = doc.createTextNode(trackInfo.value("album").toString());
    album.appendChild(t3);

    Jid streamJid;

    for (int i = 0; i < FXmppStreams->xmppStreams().size(); i++) {
        streamJid = FXmppStreams->xmppStreams().at(i)->streamJid();
        FPEPManager->publishItem(streamJid, TUNE_PROTOCOL_URL, root);
    }
}

void UserTuneHandler::setContactTune(const Jid &AContactJid, const QString &ASong)
{
    Jid contactJid = AContactJid.pBare();
    if (FContactTune.value(contactJid) != ASong)
    {
        if (!ASong.isEmpty())
            FContactTune.insert(contactJid,ASong);
        else
            FContactTune.remove(contactJid);
    }

    QMultiMap<int, QVariant> findData;
    findData.insert(RDR_TYPE,RIT_CONTACT);
    findData.insert(RDR_PREP_BARE_JID,AContactJid.pBare());
    foreach (IRosterIndex *index, FRostersModel->rootIndex()->findChilds(findData,true))

    if (!ASong.isEmpty() && (AContactJid == index->data(RDR_PREP_BARE_JID).toString()))
        FRostersViewPlugin->rostersView()->insertLabel(FUserTuneLabelId,index);
    else
        FRostersViewPlugin->rostersView()->removeLabel(FUserTuneLabelId,index);

}

void UserTuneHandler::onRosterIndexToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips)
{
    if (ALabelId==RLID_DISPLAY || ALabelId==FUserTuneLabelId)
    {
        Jid contactJid = AIndex->data(RDR_PREP_BARE_JID).toString();
        if (!FContactTune.value(contactJid).isEmpty())
        {
            QString tip = QString("%1 <div style='margin-left:10px;'>%2</div>").arg(tr("Listen:")).arg(Qt::escape(FContactTune.value(contactJid)).replace("\n","<br>"));
            AToolTips.insert(RTTO_USERTUNE,tip);
        }
    }
}

void UserTuneHandler::onApplicationQuit()
{
    FPEPManager->removeNodeHandler(handlerId);
}

Q_EXPORT_PLUGIN2(plg_pepmanager, UserTuneHandler)

