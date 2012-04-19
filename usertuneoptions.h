#ifndef USERTUNEOPTIONS_H
#define USERTUNEOPTIONS_H

#include <QWidget>

#include <interfaces/ioptionsmanager.h>

#include "imprisfetcher.h"

namespace Ui {
    class UserTuneOptions;
}

class UserTuneOptions : public QWidget,
        public IOptionsWidget
{
    Q_OBJECT
    Q_INTERFACES(IOptionsWidget)
    
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
    void onRefreshPlayer();
    void onVersionChange(int);

private:
    Ui::UserTuneOptions *ui;
};

#endif // USERTUNEOPTIONS_H
