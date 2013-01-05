#ifndef DEF_DEFINITIONS_H
#define DEF_DEFINITIONS_H

#include <utils/advanceditemdelegate.h>

//Namespaces
//#define NS_something                      "somesing"

//MessageEditorOrders
#define MEO_USERTUNE                 500

//Notification Types
#define NNT_USERTUNE                 "UserTuneNotify"

//Notification Type Orders
#define NTO_USERTUNE_NOTIFY                 275

//Menu Icons
#define MNI_USERTUNE_MUSIC                 "usertuneMusic"

//Options
#define OPV_UT_SHOW_ROSTER_LABEL            "usertune.show-roster-label"
#define OPV_UT_ALLOW_SEND_MUSIC_INFO        "usertune.allow-send-music-info"
#define OPV_UT_NOT_ALLOW_SEND_URL_INFO      "usertune.not-allow-send-url-info"
#define OPV_UT_TAG_FORMAT                   "usertune.tag-format"
#define OPV_UT_PLAYER_NAME                  "usertune.player-name"
#define OPV_UT_PLAYER_VER                   "usertune.player-ver"

//Option Nodes
#define OPN_USERTUNE                       "UserTune"

//Option Node Order
#define ONO_USERTUNE                           860

//Option Widget Order
#define OWO_USERTUNE                           500

//Roster Label ID
#define RLID_USERTUNE                      AdvancedDelegateItem::makeId(AdvancedDelegateItem::MiddleCenter,\
                                                                        128,\
                                                                        AdvancedDelegateItem::AlignRightOrderMask | 444)

//Roster ToolTip Order
#define RTTO_USERTUNE                      910

#endif //DEF_DEFINITIONS_H

