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

#include <interfaces/ipluginmanager.h>
#include <interfaces/ipepmanager.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/inotifications.h>
#include <interfaces/iroster.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>

#include <utils/options.h>

#include "mprisfetcher.h"

#ifdef SVNINFO
#  include "svninfo.h"
#else
#  define SVN_REVISION              "0"
#endif

#define USERTUNE_UUID  "{b9adf1dd-25e4-48ab-b289-73d3c63e0f4a}"
#define PEP_USERTUNE              4000

class UserTune
{
public:
    UserTune();
    ~UserTune();
    bool isEmpty() const;
    bool operator ==(const UserTune &AUserTune) const;
    bool operator !=(const UserTune &AUserTune) const;

    QString artist;
    QString source;
    QString title;
    QString track;
    int length;
    int rating;
    QUrl uri;
};

class UserTuneHandler :
    public QObject,
    public IPlugin,
    public IOptionsHolder,
    public IPEPHandler
{
    Q_OBJECT;
    Q_INTERFACES(IPlugin IOptionsHolder IPEPHandler);
public:
    UserTuneHandler();
    ~UserTuneHandler();
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
    //IPEPHandler
    virtual bool processPEPEvent(const Jid &AStreamJid, const Stanza &AStanza);

protected slots:
    void onTrackChanged(QVariantMap trackInfo);
    void onStopPublishing();
    void onOptionsOpened();
    void onOptionsChanged(const OptionsNode &ANode);
    void onRosterIndexInserted(const Jid &AContactJid, const QString &ASong);
    void onRosterIndexToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
    void onShowNotification(const QString &AContactJid);
    void onNotificationActivated(int ANotifyId);
    void onNotificationRemoved(int ANotifyId);
    void onApplicationQuit();

protected:
    void setContactTune(const QString &AContactJid, const UserTune &ASong);
    void setContactLabel();
    QString returnTagFormat(QString);

private:
    IPEPManager *FPEPManager;
    IServiceDiscovery *FServiceDiscovery;
    IXmppStreams *FXmppStreams;
    IOptionsManager *FOptionsManager;
    IRosterPlugin *FRosterPlugin;
    IRostersModel *FRostersModel;
    IRostersViewPlugin *FRostersViewPlugin;
    INotifications *FNotifications;
    MprisFetcher *FMprisFetcher;

    QStringList FPlayers;
    QString nodeName;
    int handlerId;
    int FUserTuneLabelId;
    QString FFormatTag;
    QString FTag;

    QMap<QString, UserTune> FContactTune;
    QMap<int,Jid> FNotifies;
};

#endif // USERTUNE_H

