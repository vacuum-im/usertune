#include <utils/options.h>

#include "imetadatafetcher.h"
#include "usertunetypes.h"

#include <ui_usertuneoptions.h>
#include "usertuneoptions.h"

#include "definitions.h"

UserTuneOptions::UserTuneOptions(QWidget *AParent) :
    QWidget(AParent),
    ui(new Ui::UserTuneOptions)
{
    ui->setupUi(this);

    ui->cb_mpris_version->addItem(tr("Not selected"), FetchrVer::fetcherNone);
    ui->cb_mpris_version->addItem(QLatin1String("MPRISv1"), FetchrVer::mprisV1);
    ui->cb_mpris_version->addItem(QLatin1String("MPRISv2"), FetchrVer::mprisV2);

    connect(ui->cb_mpris_version,SIGNAL(currentIndexChanged(int)),this,SLOT(onVersionChange(int)));

    connect(ui->cb_playerName,SIGNAL(currentIndexChanged(int)),this,SIGNAL(modified()));
    connect(ui->chb_showIcon,SIGNAL(stateChanged(int)),this,SIGNAL(modified()));
    connect(ui->le_format,SIGNAL(textChanged(const QString &)),this,SIGNAL(modified()));
    connect(ui->btn_refreshPlayers, SIGNAL(clicked()),this, SLOT(onRefreshPlayer()));

    reset();
}

UserTuneOptions::~UserTuneOptions()
{
    delete ui;
}

void UserTuneOptions::onRefreshPlayer()
{
    int index = ui->cb_mpris_version->currentIndex();
    int version = ui->cb_mpris_version->itemData(index).toInt();
    QStringList players = getPlayersList(version);

    ui->cb_playerName->clear();
    ui->cb_playerName->addItems(players);

    index = ui->cb_playerName->findText(Options::node(OPV_UT_PLAYER_NAME).value().toString());
    ui->cb_playerName->setCurrentIndex(index);
}

void UserTuneOptions::onVersionChange(int index)
{
    bool enabled = ui->cb_mpris_version->itemData(index).toInt() != FetchrVer::fetcherNone;
    ui->le_format->setEnabled(enabled);
    ui->cb_playerName->setEnabled(enabled);
    ui->btn_refreshPlayers->setEnabled(enabled);

    if (enabled)
    {
        onRefreshPlayer();
    }
}

void UserTuneOptions::apply()
{
    Options::node(OPV_UT_SHOW_ROSTER_LABEL).setValue(ui->chb_showIcon->isChecked());
    Options::node(OPV_UT_TAG_FORMAT).setValue(ui->le_format->text());

    int index = ui->cb_mpris_version->currentIndex();
    int version = ui->cb_mpris_version->itemData(index).toInt();

    QString name = ui->cb_playerName->currentText();

    if (version != FetchrVer::fetcherNone)
    {
        Options::node(OPV_UT_PLAYER_VER).setValue(version);
        Options::node(OPV_UT_PLAYER_NAME).setValue(name);
    }
    else
    {
        Options::node(OPV_UT_PLAYER_VER).setValue(FetchrVer::fetcherNone);
        Options::node(OPV_UT_PLAYER_NAME).setValue(QLatin1String(""));
    }

    emit childApply();
}

void UserTuneOptions::reset()
{
    ui->chb_showIcon->setChecked(Options::node(OPV_UT_SHOW_ROSTER_LABEL).value().toBool());
    ui->le_format->setText(Options::node(OPV_UT_TAG_FORMAT).value().toString());

    int index = ui->cb_playerName->findText(Options::node(OPV_UT_PLAYER_NAME).value().toString());
    ui->cb_playerName->setCurrentIndex(index != -1 ? index : 0);

    index = ui->cb_mpris_version->findData(Options::node(OPV_UT_PLAYER_VER).value().toInt());
    ui->cb_mpris_version->setCurrentIndex(index);

    emit childReset();
}
