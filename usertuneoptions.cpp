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

	ui->cb_mpris_version->addItem(QLatin1String("MPRISv1"), FetcherVer::mprisV1);
	ui->cb_mpris_version->addItem(QLatin1String("MPRISv2"), FetcherVer::mprisV2);

	connect(ui->cb_mpris_version,SIGNAL(currentIndexChanged(int)),this,SLOT(onVersionChange(int)));
	connect(ui->btn_refreshPlayers, SIGNAL(clicked()),this, SLOT(onRefreshPlayers()));
	connect(ui->le_format, SIGNAL(textChanged(QString)), SIGNAL(modified()));
	connect(ui->chb_allowSendMusicInfo, SIGNAL(clicked()), SIGNAL(modified()));
	connect(ui->chb_dontSendURI, SIGNAL(clicked()), SIGNAL(modified()));
	connect(ui->chb_showIcon, SIGNAL(clicked()), SIGNAL(modified()));

	reset();
}

UserTuneOptions::~UserTuneOptions()
{
	delete ui;
}

void UserTuneOptions::onRefreshPlayers()
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
	Q_UNUSED(index)

	onRefreshPlayers();

	emit modified();
}

void UserTuneOptions::apply()
{
	Options::node(OPV_UT_SHOW_ROSTER_LABEL).setValue(ui->chb_showIcon->isChecked());
	Options::node(OPV_UT_ALLOW_SEND_MUSIC_INFO).setValue(ui->chb_allowSendMusicInfo->isChecked());
	Options::node(OPV_UT_NOT_ALLOW_SEND_URL_INFO).setValue(ui->chb_dontSendURI->isChecked());
	Options::node(OPV_UT_TAG_FORMAT).setValue(ui->le_format->text());

	int index = ui->cb_mpris_version->currentIndex();

	Options::node(OPV_UT_PLAYER_VER).setValue(ui->cb_mpris_version->itemData(index).toInt());
	Options::node(OPV_UT_PLAYER_NAME).setValue(ui->cb_playerName->currentText());

	emit childApply();
}

void UserTuneOptions::reset()
{
	ui->chb_showIcon->setChecked(Options::node(OPV_UT_SHOW_ROSTER_LABEL).value().toBool());
	ui->chb_allowSendMusicInfo->setChecked(Options::node(OPV_UT_ALLOW_SEND_MUSIC_INFO).value().toBool());
	ui->chb_dontSendURI->setChecked(Options::node(OPV_UT_NOT_ALLOW_SEND_URL_INFO).value().toBool());
	ui->le_format->setText(Options::node(OPV_UT_TAG_FORMAT).value().toString());

	int index = ui->cb_mpris_version->findData(Options::node(OPV_UT_PLAYER_VER).value().toInt());
	ui->cb_mpris_version->setCurrentIndex(index);

	onRefreshPlayers();

	index = ui->cb_playerName->findText(Options::node(OPV_UT_PLAYER_NAME).value().toString());
	ui->cb_playerName->setCurrentIndex(index != -1 ? index : 0);

	emit childReset();
}
