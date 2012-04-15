#include <utils/options.h>

#include <ui_usertuneoptions.h>
#include "usertuneoptions.h"

#include "definitions.h"

UserTuneOptions::UserTuneOptions(QWidget *AParent) :
    QWidget(AParent),
    ui(new Ui::UserTuneOptions)
{
    ui->setupUi(this);

    onRefreshPlayer();

    connect(ui->cb_playerName,SIGNAL(currentIndexChanged(int)),SIGNAL(modified()));
    connect(ui->chb_showIcon,SIGNAL(stateChanged(int)),SIGNAL(modified()));
    connect(ui->le_format,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
    connect(ui->btn_refreshPlayers, SIGNAL(clicked()), SLOT(onRefreshPlayer()));

    reset();
}

UserTuneOptions::~UserTuneOptions()
{
    delete ui;
}

void UserTuneOptions::onRefreshPlayer()
{
    t_playersList players = getPlayersList();
    ui->cb_playerName->clear();
    ui->cb_playerName->addItem(tr("Not selected"),mprisNone);
    for (t_playersList::ConstIterator it = players.constBegin(); it != players.constEnd(); ++it)
    {
        ui->cb_playerName->addItem(it.key(),it.value());
    }
}

void UserTuneOptions::apply()
{
    Options::node(OPV_UT_SHOW_ROSTER_LABEL).setValue(ui->chb_showIcon->isChecked());
    Options::node(OPV_UT_TAG_FORMAT).setValue(ui->le_format->text());

    int index = ui->cb_playerName->currentIndex();

    int version = ui->cb_playerName->itemData(index).toInt();
    QString name = ui->cb_playerName->itemText(index);

    if (version != mprisNone)
    {
        QRegExp rx(MPRIS_PATTERN);

        Options::node(OPV_UT_PLAYER_VER).setValue(version);
        Options::node(OPV_UT_PLAYER_NAME).setValue(name.replace(rx,""));
    }
    else
    {
        Options::node(OPV_UT_PLAYER_VER).setValue(mprisNone);
        Options::node(OPV_UT_PLAYER_NAME).setValue(QString());
    }

    emit childApply();
}

void UserTuneOptions::reset()
{
    ui->chb_showIcon->setChecked(Options::node(OPV_UT_SHOW_ROSTER_LABEL).value().toBool());
    ui->le_format->setText(Options::node(OPV_UT_TAG_FORMAT).value().toString());

    int index = ui->cb_playerName->findText(Options::node(OPV_UT_PLAYER_NAME).value().toString());
    ui->cb_playerName->setCurrentIndex(index);

    emit childReset();
}
