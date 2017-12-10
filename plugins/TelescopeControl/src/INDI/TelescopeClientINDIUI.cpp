#include "TelescopeClientINDIUI.hpp"
#include "ui_TelescopeClientINDIUI.h"

TelescopeClientINDIUI::TelescopeClientINDIUI(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TelescopeClientINDIUI)
{
    ui->setupUi(this);
    QObject::connect(ui->getDevicesPushButton, &QPushButton::clicked, this, &TelescopeClientINDIUI::onGetDevicesPushButtonClicked);
    QObject::connect(&mConnection, &INDIConnection::devicesChanged, this, &TelescopeClientINDIUI::onDevicesChanged);
}

TelescopeClientINDIUI::~TelescopeClientINDIUI()
{
    delete ui;
}

void TelescopeClientINDIUI::onGetDevicesPushButtonClicked()
{
    if (mConnection.isConnected())
        mConnection.disconnectServer();

    QString host = ui->lineEditHostName->text();
    QString port = ui->spinBoxTCPPort->text();
    mConnection.setServer(host.toStdString().c_str(), port.toInt());
    mConnection.connectServer();
}

void TelescopeClientINDIUI::onDevicesChanged()
{
    auto devices = mConnection.devices();
    ui->devicesComboBox->clear();
    ui->devicesComboBox->addItems(devices);
}

