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

#include <interfaces/ipluginmanager.h>
#include <interfaces/ipepmanager.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/ioptionsmanager.h>
#ifdef Q_WS_X11
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imultiuserchat.h>
#endif
#include <interfaces/inotifications.h>
#include <interfaces/iroster.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>

#include <utils/options.h>
#ifdef Q_WS_X11
#include "imetadatafetcher.h"
#endif
#include "usertunetypes.h"

#define USERTUNE_UUID  "{b9adf1dd-25e4-48ab-b289-73d3c63e0f4a}"
#define PEP_USERTUNE              4000

class UserTuneHandler :
    public QObject,
    public IPlugin,
    public IOptionsHolder,
    public IPEPHandler
#ifdef Q_WS_X11
        ,
    public IMessageEditor
#endif

{
    Q_OBJECT
#ifdef Q_WS_X11
    Q_INTERFACES(IPlugin IOptionsHolder IPEPHandler IMessageEditor)
#else
    Q_INTERFACES(IPlugin IOptionsHolder IPEPHandler)
#endif
public:
    explicit UserTuneHandler();
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
    //IPEPHandler
    virtual bool processPEPEvent(const Jid &AStreamJid, const Stanza &AStanza);

protected slots:
#ifdef Q_WS_X11
    void onTrackChanged(UserTuneData);
    void onSendPep();
    void onPlayerSatusChanged(PlayerStatus);
    void onStopPublishing();
#endif
    void onSetMainLabel(IXmppStream *);
    void onUnsetMainLabel(IXmppStream *);
    void onOptionsOpened();
    void onOptionsChanged(const OptionsNode &ANode);
    void onRosterIndexInserted(const Jid &AContactJid, const QString &ASong);
    void onRosterIndexToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
    void onShowNotification(const Jid &AStreamJid, const Jid &AContactJid);
    void onNotificationActivated(int ANotifyId);
    void onNotificationRemoved(int ANotifyId);
    void onApplicationQuit();

protected:
    void setContactTune(const Jid &AContactJid, const UserTuneData &ASong);
    void setContactLabel();
    inline void setContactLabel(const Jid &AContactJid);
    void unsetContactLabel();
    inline void unsetContactLabel(const Jid &AContactJid);
    static const QString secondToString(unsigned short sec)
    {
        if (sec == 0)
        {
            return QString();
        }

        int min = 0;

        while (sec > 60) {
            ++min;
            sec -= 60;
        }

        return QString("%1:%2").arg(min).arg(sec,2,10,QChar('0'));
    }

private:
    IPEPManager *FPEPManager;
    IServiceDiscovery *FServiceDiscovery;
    IXmppStreams *FXmppStreams;
    IOptionsManager *FOptionsManager;
    IRosterPlugin *FRosterPlugin;
    IRostersModel *FRostersModel;
    IRostersViewPlugin *FRostersViewPlugin;
    INotifications *FNotifications;
#ifdef Q_WS_X11
    IMetaDataFetcher *FMetaDataFetcher;
    IMessageWidgets *FMessageWidgets;
    IMultiUserChatPlugin *FMultiUserChatPlugin;
    UserTuneData FUserTuneData;
    QTimer FTimer;
#endif

    int FHandlerId;
    int FUserTuneLabelId;
#ifdef Q_WS_X11
	bool FAllowSendPEP;
	bool FAllowSendURLInPEP;
    QString FFormatTag;
#endif

    QMap<Jid, UserTuneData> FContactTune;
    QMap<int, Jid> FNotifies;

private:
#ifdef Q_WS_X11
    void updateFetchers();
    QString getTagFormated(const Jid &AContactJid) const;
    QString getTagFormated(const UserTuneData &AUserData) const;
#endif
};

#endif // USERTUNE_H

