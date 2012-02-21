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
#include "StelSkyDrawer.hpp"
#include "AtmosphereDialog.hpp"
#include "ui_AtmosphereDialog.h"

AtmosphereDialog::AtmosphereDialog()
{
	ui = new Ui_AtmosphereDialogForm;
}

AtmosphereDialog::~AtmosphereDialog()
{
	delete ui;
}

void AtmosphereDialog::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}


void AtmosphereDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Signals and slots
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	const StelSkyDrawer* skyDrawer = StelApp::getInstance().getCore()->getSkyDrawer();
	ui->pressureDoubleSpinBox->setValue(skyDrawer->getAtmospherePressure());
	connect(ui->pressureDoubleSpinBox, SIGNAL(valueChanged(double)),
	        skyDrawer, SLOT(setAtmospherePressure(double)));
	ui->temperatureDoubleSpinBox->setValue(skyDrawer->getAtmosphereTemperature());
	connect(ui->temperatureDoubleSpinBox, SIGNAL(valueChanged(double)),
	        skyDrawer, SLOT(setAtmosphereTemperature(double)));
	ui->extinctionDoubleSpinBox->setValue(skyDrawer->getExtinctionCoefficient());
	connect(ui->extinctionDoubleSpinBox, SIGNAL(valueChanged(double)),
	        skyDrawer, SLOT(setExtinctionCoefficient(double)));
}
