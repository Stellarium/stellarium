/*
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include "CustomInfoDialog.hpp"
#include "ui_CustomInfoDialog.h"

#include "Dialog.hpp"
#include "StelApp.hpp"
#include "StelObjectMgr.hpp"

#include <QDebug>

CustomInfoDialog::CustomInfoDialog()
{
	ui = new Ui_CustomInfoDialogForm;
	conf = StelApp::getInstance().getSettings();
	gui = StelApp::getInstance().getGui();
}

CustomInfoDialog::~CustomInfoDialog()
{	
	delete ui;
	ui=NULL;
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

	//An object's name
	ui->nameCheckBox->setChecked(getNameCustomInfoFlag());
	connect(ui->nameCheckBox, SIGNAL(toggled(bool)), this, SLOT(setNameCustomInfoFlag(bool)));

	//Catalog numbers
	ui->catalogNumberCheckBox->setChecked(getCatalogNumberCustomInfoFlag());
	connect(ui->catalogNumberCheckBox, SIGNAL(toggled(bool)), this, SLOT(setCatalogNumberCustomInfoFlag(bool)));

	//Magnitude related data
	ui->magnitudeCheckBox->setChecked(getMagnitudeCustomInfoFlag());
	connect(ui->magnitudeCheckBox, SIGNAL(toggled(bool)), this, SLOT(setMagnitudeCustomInfoFlag(bool)));

	//The equatorial position (J2000 ref)
	ui->raDecJ2000CheckBox->setChecked(getRaDecJ2000CustomInfoFlag());
	connect(ui->raDecJ2000CheckBox, SIGNAL(toggled(bool)), this, SLOT(setRaDecJ2000CustomInfoFlag(bool)));

	//The equatorial position (of date)
	ui->raDecOfDateCheckBox->setChecked(getRaDecOfDateCustomInfoFlag());
	connect(ui->raDecOfDateCheckBox, SIGNAL(toggled(bool)), this, SLOT(setRaDecOfDateCustomInfoFlag(bool)));

	//The position (Altitude/Azimuth)
	ui->altAzCheckBox->setChecked(getAltAzCustomInfoFlag());
	connect(ui->altAzCheckBox, SIGNAL(toggled(bool)), this, SLOT(setAltAzCustomInfoFlag(bool)));

	//Info about an object's distance
	ui->distanceCheckBox->setChecked(getDistanceCustomInfoFlag());
	connect(ui->distanceCheckBox, SIGNAL(toggled(bool)), this, SLOT(setDistanceCustomInfoFlag(bool)));

	//Info about an object's size
	ui->sizeCheckBox->setChecked(getSizeCustomInfoFlag());
	connect(ui->sizeCheckBox, SIGNAL(toggled(bool)), this, SLOT(setSizeCustomInfoFlag(bool)));

	//Derived class-specific extra fields
	ui->extra1CheckBox->setChecked(getExtra1CustomInfoFlag());
	connect(ui->extra1CheckBox, SIGNAL(toggled(bool)), this, SLOT(setExtra1CustomInfoFlag(bool)));

	//Derived class-specific extra fields
	ui->extra2CheckBox->setChecked(getExtra2CustomInfoFlag());
	connect(ui->extra2CheckBox, SIGNAL(toggled(bool)), this, SLOT(setExtra2CustomInfoFlag(bool)));

	//Derived class-specific extra fields
	ui->extra3CheckBox->setChecked(getExtra3CustomInfoFlag());
	connect(ui->extra3CheckBox, SIGNAL(toggled(bool)), this, SLOT(setExtra3CustomInfoFlag(bool)));

	//The hour angle + DE (of date)
	ui->hourAngleCheckBox->setChecked(getHourAngleCustomInfoFlag());
	connect(ui->hourAngleCheckBox, SIGNAL(toggled(bool)), this, SLOT(setHourAngleCustomInfoFlag(bool)));

	//The absolute magnitude
	ui->absoluteMagnitudeCheckBox->setChecked(getAbsoluteMagnitudeCustomInfoFlag());
	connect(ui->absoluteMagnitudeCheckBox, SIGNAL(toggled(bool)), this, SLOT(setAbsoluteMagnitudeCustomInfoFlag(bool)));

}

void CustomInfoDialog::setVisible(bool v)
{
	StelDialog::setVisible(v);	
}

bool CustomInfoDialog::getNameCustomInfoFlag()
{
	return conf->value("custom_selected_info/flag_show_name", false).toBool();
}

void CustomInfoDialog::setNameCustomInfoFlag(bool flag)
{
	conf->setValue("custom_selected_info/flag_show_name", flag);
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getStelObjectMgr().getCustomInfoString()));
}

bool CustomInfoDialog::getCatalogNumberCustomInfoFlag()
{
	return conf->value("custom_selected_info/flag_show_catalognumber", false).toBool();
}

void CustomInfoDialog::setCatalogNumberCustomInfoFlag(bool flag)
{
	conf->setValue("custom_selected_info/flag_show_catalognumber", flag);
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getStelObjectMgr().getCustomInfoString()));
}

bool CustomInfoDialog::getMagnitudeCustomInfoFlag()
{
	return conf->value("custom_selected_info/flag_show_magnitude", false).toBool();
}

void CustomInfoDialog::setMagnitudeCustomInfoFlag(bool flag)
{
	conf->setValue("custom_selected_info/flag_show_magnitude", flag);
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getStelObjectMgr().getCustomInfoString()));
}

bool CustomInfoDialog::getRaDecJ2000CustomInfoFlag()
{
	return conf->value("custom_selected_info/flag_show_radecj2000", false).toBool();
}

void CustomInfoDialog::setRaDecJ2000CustomInfoFlag(bool flag)
{
	conf->setValue("custom_selected_info/flag_show_radecj2000", flag);
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getStelObjectMgr().getCustomInfoString()));
}

bool CustomInfoDialog::getRaDecOfDateCustomInfoFlag()
{
	return conf->value("custom_selected_info/flag_show_radecofdate", false).toBool();
}

void CustomInfoDialog::setRaDecOfDateCustomInfoFlag(bool flag)
{
	conf->setValue("custom_selected_info/flag_show_radecofdate", flag);
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getStelObjectMgr().getCustomInfoString()));
}

bool CustomInfoDialog::getAltAzCustomInfoFlag()
{
	return conf->value("custom_selected_info/flag_show_altaz", false).toBool();
}

void CustomInfoDialog::setAltAzCustomInfoFlag(bool flag)
{
	conf->setValue("custom_selected_info/flag_show_altaz", flag);
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getStelObjectMgr().getCustomInfoString()));
}

bool CustomInfoDialog::getDistanceCustomInfoFlag()
{
	return conf->value("custom_selected_info/flag_show_distance", false).toBool();
}

void CustomInfoDialog::setDistanceCustomInfoFlag(bool flag)
{
	conf->setValue("custom_selected_info/flag_show_distance", flag);
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getStelObjectMgr().getCustomInfoString()));
}

bool CustomInfoDialog::getSizeCustomInfoFlag()
{
	return conf->value("custom_selected_info/flag_show_size", false).toBool();
}

void CustomInfoDialog::setSizeCustomInfoFlag(bool flag)
{
	conf->setValue("custom_selected_info/flag_show_size", flag);
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getStelObjectMgr().getCustomInfoString()));
}

bool CustomInfoDialog::getExtra1CustomInfoFlag()
{
	return conf->value("custom_selected_info/flag_show_extra1", false).toBool();
}

void CustomInfoDialog::setExtra1CustomInfoFlag(bool flag)
{
	conf->setValue("custom_selected_info/flag_show_extra1", flag);
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getStelObjectMgr().getCustomInfoString()));
}

bool CustomInfoDialog::getExtra2CustomInfoFlag()
{
	return conf->value("custom_selected_info/flag_show_extra2", false).toBool();
}

void CustomInfoDialog::setExtra2CustomInfoFlag(bool flag)
{
	conf->setValue("custom_selected_info/flag_show_extra2", flag);
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getStelObjectMgr().getCustomInfoString()));
}

bool CustomInfoDialog::getExtra3CustomInfoFlag()
{
	return conf->value("custom_selected_info/flag_show_extra3", false).toBool();
}

void CustomInfoDialog::setExtra3CustomInfoFlag(bool flag)
{
	conf->setValue("custom_selected_info/flag_show_extra3", flag);
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getStelObjectMgr().getCustomInfoString()));
}

bool CustomInfoDialog::getHourAngleCustomInfoFlag()
{
	return conf->value("custom_selected_info/flag_show_hourangle", false).toBool();
}

void CustomInfoDialog::setHourAngleCustomInfoFlag(bool flag)
{
	conf->setValue("custom_selected_info/flag_show_hourangle", flag);
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getStelObjectMgr().getCustomInfoString()));
}

bool CustomInfoDialog::getAbsoluteMagnitudeCustomInfoFlag()
{
	return conf->value("custom_selected_info/flag_show_absolutemagnitude", false).toBool();
}

void CustomInfoDialog::setAbsoluteMagnitudeCustomInfoFlag(bool flag)
{
	conf->setValue("custom_selected_info/flag_show_absolutemagnitude", flag);
	gui->setInfoTextFilters(StelObject::InfoStringGroup(StelApp::getInstance().getStelObjectMgr().getCustomInfoString()));
}

