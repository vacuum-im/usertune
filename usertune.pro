include(qmake/debug.inc)
include(qmake/config.inc)

#Project configuration
TARGET              = usertune
QT                  = core gui dbus xml
include(usertune.pri)

#Default progect configuration
include(qmake/plugin.inc)

#Translation
TRANS_SOURCE_ROOT   = .
include(translations/languages.inc)
