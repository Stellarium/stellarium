/*
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelSkyDrawer.hpp"
#include "AtmosphereDialog.hpp"
#include "ui_AtmosphereDialog.h"

AtmosphereDialog::AtmosphereDialog()
{
    ui=new Ui_AtmosphereDialogForm;
}

AtmosphereDialog::~AtmosphereDialog()
{
    delete ui;
}

void AtmosphereDialog::languageChanged()
{
        if (dialog)
                ui->retranslateUi(dialog);
}


void AtmosphereDialog::createDialogContent()
{
        ui->setupUi(dialog);

        //Signals and slots
        connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(languageChanged()));
        connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

        ui->pressureDoubleSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getAtmospherePressure());
        connect(ui->pressureDoubleSpinBox, SIGNAL(valueChanged(double)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setAtmospherePressure(double)));
        ui->temperatureDoubleSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getAtmosphereTemperature());
        connect(ui->temperatureDoubleSpinBox, SIGNAL(valueChanged(double)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setAtmosphereTemperature(double)));
        ui->extinctionDoubleSpinBox->setValue(StelApp::getInstance().getCore()->getSkyDrawer()->getExtinctionCoefficient());
        connect(ui->extinctionDoubleSpinBox, SIGNAL(valueChanged(double)), StelApp::getInstance().getCore()->getSkyDrawer(), SLOT(setExtinctionCoefficient(double)));
}
