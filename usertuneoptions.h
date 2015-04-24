#ifndef USERTUNEOPTIONS_H
#define USERTUNEOPTIONS_H

#include <QWidget>

#include <interfaces/ioptionsmanager.h>

namespace Ui {
	class UserTuneOptions;
}

class UserTuneOptions :
		public QWidget,
		public IOptionsDialogWidget
{
	Q_OBJECT
	Q_INTERFACES(IOptionsDialogWidget)

public:
	explicit UserTuneOptions(QWidget *AParent);
	~UserTuneOptions();
	virtual QWidget* instance() { return this; }

signals:
	void modified();
	void childApply();
	void childReset();

public slots:
	virtual void apply();
	virtual void reset();

private slots:
	void onRefreshPlayers();
	void onVersionChange(int);

private:
	Ui::UserTuneOptions *ui;
};

#endif // USERTUNEOPTIONS_H
