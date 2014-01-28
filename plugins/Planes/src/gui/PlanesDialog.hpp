/*
 * Copyright (C) 2013 Felix Zeltner
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef PLANESDIALOG_HPP
#define PLANESDIALOG_HPP

#include "StelDialog.hpp"
#include "Flight.hpp"
#include "DBCredentials.hpp"

namespace Ui {
class PlanesDialog;
}

class PlanesDialog : public StelDialog
{
    Q_OBJECT

public:
    explicit PlanesDialog();
    ~PlanesDialog();

public slots:
    void retranslate();
    void setDBStatus(QString status);
    void setBSStatus(QString status);

signals:
    void changePathColourMode(Flight::PathColour mode);
    void changePathDrawModw(Flight::PathDrawMode mode);
    void showLabels(bool enabled);
    void fileSelected(QString filename);
    void useInterp(bool interp);
    void useDB(bool useDB);
    void useBS(bool useBS);
    void credentialsChanged(DBCredentials creds);
    void connectDB();
    void disconnectDB();
    void bsHostChanged(QString host);
    void bsPortChanged(quint16 port);
    void bsUseDBChanged(bool useDB);
    void connectBS();
    void disconnectBS();
    void connectOnStartup(bool enabled);

protected:
    void createDialogContent();

private:
    void updateDBFields();

    Ui::PlanesDialog *ui;
    QString cachedDBStatus;
    QString cachedBSStatus;

private slots:
    void solidColClicked()
    {
        emit changePathColourMode(Flight::SolidColour);
    }
    void heightColClicked()
    {
        emit changePathColourMode(Flight::EncodeHeight);
    }
    void velocityColClicked()
    {
        emit changePathColourMode(Flight::EncodeVelocity);
    }
    void noPathsClicked()
    {
        emit changePathDrawModw(Flight::NoPaths);
    }
    void selectedPathClicked()
    {
        emit changePathDrawModw(Flight::SelectedOnly);
    }
    void inViewPathsClicked()
    {
        emit changePathDrawModw(Flight::InViewOnly);
    }
    void allPathsClicked()
    {
        emit changePathDrawModw(Flight::AllPaths);
    }
    void showLabelsClicked(bool enabled)
    {
        emit showLabels(enabled);
    }
    void useInterpClicked(bool checked)
    {
        emit useInterp(checked);
    }
    void minHeightChanged(double val) {
        Flight::setMinHeight(val);
    }
    void maxHeightChanged(double val) {
        Flight::setMaxHeight(val);
    }
    void minVelChanged(double val) {
        Flight::setMinVelocity(val);
    }
    void maxVelChanged(double val) {
        Flight::setMaxVelocity(val);
    }
    void minVertRateChanged(double val) {
        Flight::setMinVertRate(val);
    }
    void maxVertRateChanged(double val) {
        Flight::setMaxVertRate(val);
    }

    void fileOpenClicked();

    void connectClicked()
    {
        emit connectDB();
    }
    void disconnectClicked()
    {
        emit disconnectDB();
    }

    void useDBClicked();
    void credsChanged();

    void bsHostClicked(const QString &host)
    {
        emit bsHostChanged(host);
    }
    void bsPortClicked(const QString &port)
    {
        emit bsPortChanged(port.toUInt());
    }
    void bsUseDBClicked(bool enabled)
    {
        emit bsUseDBChanged(enabled);
    }
    void bsConnectClicked()
    {
        emit connectBS();
    }
    void bsDisconnectClicked()
    {
        emit disconnectBS();
    }
    void useBSClicked();
    void connectOnStartupClicked();
};

#endif // PLANESDIALOG_HPP
