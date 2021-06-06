#include "LightPollutionWidget.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include <QCursor>
#include <QToolTip>
#include <QSettings>

static constexpr double MIN_LIGHT_POLLUTION_LUMINANCE_MPSAS = 15;
static constexpr double MAX_LIGHT_POLLUTION_LUMINANCE_MPSAS = 26.5;
static constexpr char SETTINGS_KEY_MODE[] = "gui/light_pollution_widget_mode";
static constexpr char SETTINGS_KEY_UNIT[] = "gui/light_pollution_unit";
static constexpr char MODE_NAME_MANUAL[] = "manual";
static constexpr char MODE_NAME_MANUAL_FROM_SQM[] = "sqm";
static constexpr char UNIT_NAME_MPSAS[] = "mpsas";
static constexpr char UNIT_NAME_CDM2[] = "cdm2";
static constexpr char UNIT_NAME_MCDM2[] = "mcdm2";
static constexpr char UNIT_NAME_UCDM2[] = "ucdm2";
static constexpr char PROPERTY_LUMINANCE[] = "StelSkyDrawer.lightPollutionLuminance";
static constexpr char PROPERTY_USE_DB[] = "LandscapeMgr.flagUseLightPollutionFromDatabase";

LightPollutionWidget::LightPollutionWidget(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui_lightPollutionWidget)
{
}

void LightPollutionWidget::setup()
{
	ui->setupUi(this);
	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &LightPollutionWidget::retranslate);
	populate();

	ui->manualSlider->setTracking(true);
	connect(ui->manualSlider, &QSlider::valueChanged, this, &LightPollutionWidget::onSliderMoved);
	connect(ui->fromSQMmag_SB, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
			&LightPollutionWidget::onSpinboxEdited);
	connect(ui->fromSQMcdm2_logSB, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
			&LightPollutionWidget::onSpinboxEdited);
	connect(ui->modeCB, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
			[this](const int index){ setMode(static_cast<Mode>(index)); });
	connect(ui->unitCB, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
			[this](const int index){ setUnit(static_cast<Unit>(index)); });

	const auto luminanceProp = StelApp::getInstance().getStelPropertyManager()->getProperty(PROPERTY_LUMINANCE);
	connect(this, &LightPollutionWidget::luminanceChanged, luminanceProp, &StelProperty::setValue);
	connect(luminanceProp, &StelProperty::changed, this, &LightPollutionWidget::onLuminancePropertyChanged);

	const auto databaseUseProp = StelApp::getInstance().getStelPropertyManager()->getProperty(PROPERTY_USE_DB);
	connect(this, &LightPollutionWidget::databaseUseChanged, databaseUseProp, &StelProperty::setValue);
	connect(databaseUseProp, &StelProperty::changed, this, &LightPollutionWidget::onUseDBPropertyChanged);
}

void LightPollutionWidget::retranslate()
{
	ui->retranslateUi(this);
	updateBortleScaleToolTip();
}

void LightPollutionWidget::populate()
{
	const auto conf = StelApp::getInstance().getSettings();
	const auto propMan = StelApp::getInstance().getStelPropertyManager();

	const bool useDB = propMan->getStelPropertyValue(PROPERTY_USE_DB).toBool();
	const auto modeStr = conf->value(SETTINGS_KEY_MODE).toString();
	if (useDB)
		setMode(Mode::AutoFromDB);
	else if(modeStr==MODE_NAME_MANUAL)
		setMode(Mode::Manual);
	else if(modeStr==MODE_NAME_MANUAL_FROM_SQM)
		setMode(Mode::ManualFromSQM);
	else
		setMode(Mode::Manual);

	const auto unitName = conf->value(SETTINGS_KEY_UNIT).toString();
	if(unitName==UNIT_NAME_MPSAS)
		setUnit(Unit::mpsas);
	else if(unitName==UNIT_NAME_CDM2)
		setUnit(Unit::cdm2);
	else if(unitName==UNIT_NAME_MCDM2)
		setUnit(Unit::mcdm2);
	else if(unitName==UNIT_NAME_UCDM2)
		setUnit(Unit::ucdm2);
	else // if not set
		setUnit(Unit::mpsas);

	const auto luminance = propMan->getStelPropertyValue(PROPERTY_LUMINANCE).toDouble();
	setLuminance(luminance);
}

void LightPollutionWidget::setMode(const Mode mode)
{
    // Prevent window resizes on switching between modes
    const auto minSpinBoxWidth = std::max(ui->fromSQMcdm2_logSB->minimumSizeHint().width(),
                                          ui->fromSQMmag_SB->minimumSizeHint().width());
    const auto minManipulatorWidth = minSpinBoxWidth + ui->unitCB->minimumSizeHint().width();
    ui->manualSlider->setMinimumWidth(minManipulatorWidth);

	ui->modeCB->setCurrentIndex(static_cast<int>(mode));
	const auto propUseDB = StelApp::getInstance().getStelPropertyManager()->getProperty(PROPERTY_USE_DB);
	switch(mode)
	{
	case Mode::AutoFromDB:
        ui->horizontalSpacer->changeSize(minManipulatorWidth,0, QSizePolicy::Minimum);
		ui->manualSlider->hide();
		ui->fromSQMcdm2_logSB->hide();
		ui->fromSQMmag_SB->hide();
		ui->unitCB->hide();
		propUseDB->setValue(true);
		break;
	case Mode::Manual:
        ui->horizontalSpacer->changeSize(0,0, QSizePolicy::Preferred);
		ui->manualSlider->show();
		ui->fromSQMcdm2_logSB->hide();
		ui->fromSQMmag_SB->hide();
		ui->unitCB->hide();
		propUseDB->setValue(false);
		StelApp::getInstance().getSettings()->setValue(SETTINGS_KEY_MODE, MODE_NAME_MANUAL);
		break;
	case Mode::ManualFromSQM:
        ui->horizontalSpacer->changeSize(0,0, QSizePolicy::Preferred);
		ui->manualSlider->hide();
		if(unit()==Unit::mpsas)
		{
			ui->fromSQMcdm2_logSB->hide();
			ui->fromSQMmag_SB->show();
		}
		else
		{
			ui->fromSQMcdm2_logSB->show();
			ui->fromSQMmag_SB->hide();
		}
		ui->unitCB->show();
		propUseDB->setValue(false);
		StelApp::getInstance().getSettings()->setValue(SETTINGS_KEY_MODE, MODE_NAME_MANUAL_FROM_SQM);
		break;
	}
	if(ui->fromSQMmag_SB->isVisible() || ui->fromSQMcdm2_logSB->isVisible())
		updateSpinBoxValue();
	if(ui->manualSlider->isVisible())
		updateSliderValue();
}

void LightPollutionWidget::setUnit(const Unit unit)
{
	{
		QSignalBlocker b(ui->unitCB);
		ui->unitCB->setCurrentIndex(static_cast<int>(unit));
	}
	{
		QSignalBlocker bm(ui->fromSQMmag_SB);
		QSignalBlocker bc(ui->fromSQMcdm2_logSB);
		switch(unit)
		{
		case Unit::mpsas:
			ui->fromSQMmag_SB->setDecimals(1);
			ui->fromSQMmag_SB->setRange(MIN_LIGHT_POLLUTION_LUMINANCE_MPSAS, MAX_LIGHT_POLLUTION_LUMINANCE_MPSAS);
			StelApp::getInstance().getSettings()->setValue(SETTINGS_KEY_UNIT, UNIT_NAME_MPSAS);
			break;
		case Unit::cdm2:
			ui->fromSQMcdm2_logSB->setDecimals(7);
			ui->fromSQMcdm2_logSB->setRange(0, 1);
			StelApp::getInstance().getSettings()->setValue(SETTINGS_KEY_UNIT, UNIT_NAME_CDM2);
			break;
		case Unit::mcdm2:
			ui->fromSQMcdm2_logSB->setDecimals(4);
			ui->fromSQMcdm2_logSB->setRange(0, 999);
			StelApp::getInstance().getSettings()->setValue(SETTINGS_KEY_UNIT, UNIT_NAME_MCDM2);
			break;
		case Unit::ucdm2:
			ui->fromSQMcdm2_logSB->setDecimals(1);
			ui->fromSQMcdm2_logSB->setRange(0, 999999);
			StelApp::getInstance().getSettings()->setValue(SETTINGS_KEY_UNIT, UNIT_NAME_UCDM2);
			break;
		}
        if(mode()==Mode::ManualFromSQM)
        {
            if(unit == Unit::mpsas)
            {
                ui->fromSQMmag_SB->show();
                ui->fromSQMcdm2_logSB->hide();
            }
            else
            {
                ui->fromSQMmag_SB->hide();
                ui->fromSQMcdm2_logSB->show();
            }
        }
	}
	if(ui->fromSQMmag_SB->isVisible() || ui->fromSQMcdm2_logSB->isVisible())
		updateSpinBoxValue();
}

void LightPollutionWidget::updateSliderValue()
{
	const auto mag = StelCore::luminanceToMPSAS(luminanceValue_);
	const auto min = MIN_LIGHT_POLLUTION_LUMINANCE_MPSAS;
	const auto max = MAX_LIGHT_POLLUTION_LUMINANCE_MPSAS;
	{
		QSignalBlocker b(ui->manualSlider);
		ui->manualSlider->setValue((mag - min) / (max - min) * ui->manualSlider->minimum());
	}
}

void LightPollutionWidget::updateSpinBoxValue()
{
	QSignalBlocker bm(ui->fromSQMmag_SB);
	QSignalBlocker bc(ui->fromSQMcdm2_logSB);
	switch(unit())
	{
	case Unit::mpsas:
		ui->fromSQMmag_SB->setValue(StelCore::luminanceToMPSAS(luminanceValue_));
		break;
	case Unit::cdm2:
		ui->fromSQMcdm2_logSB->setValue(luminanceValue_);
		break;
	case Unit::mcdm2:
		ui->fromSQMcdm2_logSB->setValue(luminanceValue_ * 1e3);
		break;
	case Unit::ucdm2:
		ui->fromSQMcdm2_logSB->setValue(luminanceValue_ * 1e6);
		break;
	}
}

void LightPollutionWidget::setLuminance(const double luminance)
{
	luminanceValue_ = luminance;
	updateSliderValue();
	updateSpinBoxValue();
	updateBortleScaleToolTip();
}

void LightPollutionWidget::onSpinboxEdited()
{
	switch(unit())
	{
	case Unit::cdm2:
		luminanceValue_ = ui->fromSQMcdm2_logSB->value();
		break;
	case Unit::mcdm2:
		luminanceValue_ = ui->fromSQMcdm2_logSB->value() * 1e-3;
		break;
	case Unit::ucdm2:
		luminanceValue_ = ui->fromSQMcdm2_logSB->value() * 1e-6;
		break;
	case Unit::mpsas:
		luminanceValue_ = StelCore::mpsasToLuminance(ui->fromSQMmag_SB->value());
		break;
	}
	updateBortleScaleToolTip();
	emit luminanceChanged(luminanceValue_);
}

void LightPollutionWidget::onSliderMoved(const int value)
{
	const auto min = MIN_LIGHT_POLLUTION_LUMINANCE_MPSAS;
	const auto max = MAX_LIGHT_POLLUTION_LUMINANCE_MPSAS;
	const auto mag = value * (max - min) / ui->manualSlider->minimum() + min;
	luminanceValue_ = StelCore::mpsasToLuminance(mag);
	updateBortleScaleToolTip();
	emit luminanceChanged(luminanceValue_);
	QToolTip::showText(QCursor::pos(), ui->fromSQMmag_SB->toolTip(), ui->manualSlider);
}

void LightPollutionWidget::onUseDBPropertyChanged(const QVariant& useVar)
{
	const bool useDB = useVar.toBool();
	if(useDB != (mode()==Mode::AutoFromDB))
		setMode(useDB ? Mode::AutoFromDB : Mode::Manual);
}

void LightPollutionWidget::onLuminancePropertyChanged(const QVariant& lumVar)
{
	const auto lum = lumVar.toDouble();
	if(lum != luminanceValue_)
		setLuminance(lum);
}

auto LightPollutionWidget::unit() const -> Unit
{
	return static_cast<Unit>(ui->unitCB->currentIndex());
}

auto LightPollutionWidget::mode() const -> Mode
{
	return static_cast<Mode>(ui->modeCB->currentIndex());
}

void LightPollutionWidget::updateBortleScaleToolTip()
{
	QStringList list;
	//TRANSLATORS: Short description for Class 1 of the Bortle scale
	list.append(q_("excellent dark-sky site"));
	//TRANSLATORS: Short description for Class 2 of the Bortle scale
	list.append(q_("typical truly dark site"));
	//TRANSLATORS: Short description for Class 3 of the Bortle scale
	list.append(q_("rural sky"));
	//TRANSLATORS: Short description for Class 4 of the Bortle scale
	list.append(q_("rural/suburban transition"));
	//TRANSLATORS: Short description for Class 5 of the Bortle scale
	list.append(q_("suburban sky"));
	//TRANSLATORS: Short description for Class 6 of the Bortle scale
	list.append(q_("bright suburban sky"));
	//TRANSLATORS: Short description for Class 7 of the Bortle scale
	list.append(q_("suburban/urban transition"));
	//TRANSLATORS: Short description for Class 8 of the Bortle scale
	list.append(q_("city sky"));
	//TRANSLATORS: Short description for Class 9 of the Bortle scale
	list.append(q_("inner-city sky"));

	const auto nelm = StelCore::luminanceToNELM(luminanceValue_);
	const auto bortleIndex = StelCore::nelmToBortleScaleIndex(nelm);

	QString tooltip = q_("Bortle class %1: %2\nNaked-eye limiting magnitude: %3")
						.arg(bortleIndex)
						.arg(list.at(bortleIndex - 1))
						.arg(std::round(nelm*10)*0.1);

	ui->manualSlider->setToolTip(tooltip);
	ui->fromSQMmag_SB->setToolTip(tooltip);
	ui->fromSQMcdm2_logSB->setToolTip(tooltip);
}
