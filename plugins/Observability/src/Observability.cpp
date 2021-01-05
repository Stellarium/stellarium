#include "StelUtils.hpp"
#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelVertexArray.hpp"

#include "Observability.hpp"

#include <QDebug>
#include <QTimer>
#include <QPixmap>
#include <QSettings>
#include <QKeyEvent>
#include <QMouseEvent>
#include <cmath>
#include <stdexcept>

Observability::Observability()
{
  config = StelApp::getInstance().getSettings();
}

void Observability::showGoodNights(bool b)
{
	flagShowGoodNights = b;
  // save setting
	config->setValue("Observability/show_good_nights", flagShowGoodNights);
	emit flagShowGoodNightsChanged(b);
}

void Observability::showBestNight(bool b)
{
	flagShowBestNight = b;
  // save setting
	config->setValue("Observability/show_best_night", flagShowBestNight);
	emit flagShowBestNightChanged(b);
}

void Observability::showToday(bool b)
{
	flagShowToday = b;
  // save setting
	config->setValue("Observability/show_today", flagShowToday);
	emit flagShowTodayChanged(b);
}

void Observability::showFullMoon(bool b)
{
	flagShowFullMoon = b;
  // save setting
	config->setValue("Observability/show_full_moon", flagShowFullMoon);
	emit flagShowFullMoonChanged(b);
}

void Observability::restoreDefaultSettings()
{
	Q_ASSERT(config);
  // Remove the old values
	config->remove("Observability");
  // load default settings
	loadSettings();

  config->beginGroup("Observability");
  
	config->endGroup();
}

void Observability::loadSettings()
{
	showGoodNights(config->value("Observability/show_good_nights", true).toBool());
  showBestNight(config->value("Observability/show_best_night", true).toBool());
  showToday(config->value("Observability/show_today", true).toBool());
  showFullMoon(config->value("Observability/show_full_moon", true).toBool());

}