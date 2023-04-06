#ifndef SEASONS_WIDGET
#define SEASONS_WIDGET

#include <memory>
#include <QWidget>
#include "ui_seasonsWidget.h"

class SeasonsWidget : public QWidget
{
    Q_OBJECT
public:
    SeasonsWidget(QWidget* parent = nullptr);
    void setup();
    void retranslate();

private slots:
    void setSeasonLabels();
    void setSeasonTimes();

private:
    class StelCore* core;
    class SpecificTimeMgr* specMgr;
    class StelLocaleMgr* localeMgr;

    void populate();

    std::unique_ptr<Ui_seasonsWidget> ui;
};

#endif
