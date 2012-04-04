/*
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "QSettings"
#include "CustomInfoDialog.hpp"
#include "ui_CustomInfoDialog.h"

CustomInfoDialog::CustomInfoDialog()
{
	ui = new Ui_CustomInfoDialogForm;	
}

CustomInfoDialog::~CustomInfoDialog()
{	
	delete ui;
}

void CustomInfoDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}


void CustomInfoDialog::createDialogContent()
{
	ui->setupUi(dialog);

	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	ui->nameCheckBox->setChecked(getNameCustomInfoFlag());
	connect(ui->nameCheckBox, SIGNAL(toggled(bool)), this, SLOT(setNameCustomInfoFlag(bool)));

	ui->catalogNumberCheckBox->setChecked(getCatalogNumberCustomInfoFlag());
	connect(ui->catalogNumberCheckBox, SIGNAL(toggled(bool)), this, SLOT(setCatalogNumberCustomInfoFlag(bool)));

	ui->magnitudeCheckBox->setChecked(getMagnitudeCustomInfoFlag());
	connect(ui->magnitudeCheckBox, SIGNAL(toggled(bool)), this, SLOT(setMagnitudeCustomInfoFlag(bool)));

	ui->raDecJ2000CheckBox->setChecked(getRaDecJ2000CustomInfoFlag());
	connect(ui->raDecJ2000CheckBox, SIGNAL(toggled(bool)), this, SLOT(setRaDecJ2000CustomInfoFlag(bool)));

	ui->raDecOfDateCheckBox->setChecked(getRaDecOfDateCustomInfoFlag());
	connect(ui->raDecOfDateCheckBox, SIGNAL(toggled(bool)), this, SLOT(setRaDecOfDateCustomInfoFlag(bool)));

	ui->altAzCheckBox->setChecked(getAltAzCustomInfoFlag());
	connect(ui->altAzCheckBox, SIGNAL(toggled(bool)), this, SLOT(setAltAzCustomInfoFlag(bool)));

	ui->distanceCheckBox->setChecked(getDistanceCustomInfoFlag());
	connect(ui->distanceCheckBox, SIGNAL(toggled(bool)), this, SLOT(setDistanceCustomInfoFlag(bool)));

	ui->sizeCheckBox->setChecked(getSizeCustomInfoFlag());
	connect(ui->sizeCheckBox, SIGNAL(toggled(bool)), this, SLOT(setSizeCustomInfoFlag(bool)));

	ui->extra1CheckBox->setChecked(getExtra1CustomInfoFlag());
	connect(ui->extra1CheckBox, SIGNAL(toggled(bool)), this, SLOT(setExtra1CustomInfoFlag(bool)));

	ui->extra2CheckBox->setChecked(getExtra2CustomInfoFlag());
	connect(ui->extra2CheckBox, SIGNAL(toggled(bool)), this, SLOT(setExtra2CustomInfoFlag(bool)));

	ui->extra3CheckBox->setChecked(getExtra3CustomInfoFlag());
	connect(ui->extra3CheckBox, SIGNAL(toggled(bool)), this, SLOT(setExtra3CustomInfoFlag(bool)));

	ui->hourAngleCheckBox->setChecked(getHourAngleCustomInfoFlag());
	connect(ui->hourAngleCheckBox, SIGNAL(toggled(bool)), this, SLOT(setHourAngleCustomInfoFlag(bool)));

	ui->absoluteMagnitudeCheckBox->setChecked(getAbsoluteMagnitudeCustomInfoFlag());
	connect(ui->absoluteMagnitudeCheckBox, SIGNAL(toggled(bool)), this, SLOT(setAbsoluteMagnitudeCustomInfoFlag(bool)));

}

void CustomInfoDialog::setVisible(bool v)
{
	StelDialog::setVisible(v);
}

bool CustomInfoDialog::getNameCustomInfoFlag()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	return conf->value("custom_selected_info/flag_show_name", false).toBool();
}

void CustomInfoDialog::setNameCustomInfoFlag(bool flag)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("custom_selected_info/flag_show_name", flag);
	StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getCore()->getCustomInfoString()));
}

bool CustomInfoDialog::getCatalogNumberCustomInfoFlag()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	return conf->value("custom_selected_info/flag_show_catalognumber", false).toBool();
}

void CustomInfoDialog::setCatalogNumberCustomInfoFlag(bool flag)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("custom_selected_info/flag_show_catalognumber", flag);
	StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getCore()->getCustomInfoString()));
}

bool CustomInfoDialog::getMagnitudeCustomInfoFlag()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	return conf->value("custom_selected_info/flag_show_magnitude", false).toBool();
}

void CustomInfoDialog::setMagnitudeCustomInfoFlag(bool flag)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("custom_selected_info/flag_show_magnitude", flag);
	StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getCore()->getCustomInfoString()));
}

bool CustomInfoDialog::getRaDecJ2000CustomInfoFlag()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	return conf->value("custom_selected_info/flag_show_radecj2000", false).toBool();
}

void CustomInfoDialog::setRaDecJ2000CustomInfoFlag(bool flag)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("custom_selected_info/flag_show_radecj2000", flag);
	StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getCore()->getCustomInfoString()));
}

bool CustomInfoDialog::getRaDecOfDateCustomInfoFlag()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	return conf->value("custom_selected_info/flag_show_radecofdate", false).toBool();
}

void CustomInfoDialog::setRaDecOfDateCustomInfoFlag(bool flag)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("custom_selected_info/flag_show_radecofdate", flag);
	StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getCore()->getCustomInfoString()));
}

bool CustomInfoDialog::getAltAzCustomInfoFlag()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	return conf->value("custom_selected_info/flag_show_altaz", false).toBool();
}

void CustomInfoDialog::setAltAzCustomInfoFlag(bool flag)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("custom_selected_info/flag_show_altaz", flag);
	StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getCore()->getCustomInfoString()));
}

bool CustomInfoDialog::getDistanceCustomInfoFlag()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	return conf->value("custom_selected_info/flag_show_distance", false).toBool();
}

void CustomInfoDialog::setDistanceCustomInfoFlag(bool flag)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("custom_selected_info/flag_show_distance", flag);
	StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getCore()->getCustomInfoString()));
}

bool CustomInfoDialog::getSizeCustomInfoFlag()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	return conf->value("custom_selected_info/flag_show_size", false).toBool();
}

void CustomInfoDialog::setSizeCustomInfoFlag(bool flag)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("custom_selected_info/flag_show_size", flag);
	StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getCore()->getCustomInfoString()));
}

bool CustomInfoDialog::getExtra1CustomInfoFlag()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	return conf->value("custom_selected_info/flag_show_extra1", false).toBool();
}

void CustomInfoDialog::setExtra1CustomInfoFlag(bool flag)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("custom_selected_info/flag_show_extra1", flag);
	StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getCore()->getCustomInfoString()));
}

bool CustomInfoDialog::getExtra2CustomInfoFlag()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	return conf->value("custom_selected_info/flag_show_extra2", false).toBool();
}

void CustomInfoDialog::setExtra2CustomInfoFlag(bool flag)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("custom_selected_info/flag_show_extra2", flag);
	StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getCore()->getCustomInfoString()));
}

bool CustomInfoDialog::getExtra3CustomInfoFlag()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	return conf->value("custom_selected_info/flag_show_extra3", false).toBool();
}

void CustomInfoDialog::setExtra3CustomInfoFlag(bool flag)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("custom_selected_info/flag_show_extra3", flag);
	StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getCore()->getCustomInfoString()));
}

bool CustomInfoDialog::getHourAngleCustomInfoFlag()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	return conf->value("custom_selected_info/flag_show_hourangle", false).toBool();
}

void CustomInfoDialog::setHourAngleCustomInfoFlag(bool flag)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("custom_selected_info/flag_show_hourangle", flag);
	StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getCore()->getCustomInfoString()));
}

bool CustomInfoDialog::getAbsoluteMagnitudeCustomInfoFlag()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	return conf->value("custom_selected_info/flag_show_absolutemagnitude", false).toBool();
}

void CustomInfoDialog::setAbsoluteMagnitudeCustomInfoFlag(bool flag)
{
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("custom_selected_info/flag_show_absolutemagnitude", flag);
	StelApp::getInstance().getGui()->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getCore()->getCustomInfoString()));
}

