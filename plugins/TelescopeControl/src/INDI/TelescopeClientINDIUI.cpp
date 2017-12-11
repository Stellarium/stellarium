#include "TelescopeClientINDIUI.hpp"
#include "ui_TelescopeClientINDIUI.h"

TelescopeClientINDIUI::TelescopeClientINDIUI(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TelescopeClientINDIUI)
{
    ui->setupUi(this);
    QObject::connect(ui->connectButton, &QPushButton::clicked, this, &TelescopeClientINDIUI::onConnectionButtonClicked);
    QObject::connect(&mConnection, &INDIConnection::devicesChanged, this, &TelescopeClientINDIUI::onDevicesChanged);
    QObject::connect(&mConnection, &INDIConnection::connected, this, &TelescopeClientINDIUI::onServerConnected);
    QObject::connect(&mConnection, &INDIConnection::disconnected, this, &TelescopeClientINDIUI::onServerDisconnected);
}

TelescopeClientINDIUI::~TelescopeClientINDIUI()
{
    delete ui;
}

void TelescopeClientINDIUI::onConnectionButtonClicked()
{
    if (mConnection.isConnected())
    {
        ui->connectButton->setText("Disconnecting ...");
        mConnection.disconnectServer();
        return;
    }

    ui->connectButton->setText("Connecting ...");
    QString host = ui->lineEditHostName->text();
    QString port = ui->spinBoxTCPPort->text();
    mConnection.setServer(host.toStdString().c_str(), port.toInt());
    mConnection.connectServer();
}

void TelescopeClientINDIUI::onDevicesChanged()
{
    ui->devicesComboBox->clear();
    auto devices = mConnection.devices();
    ui->devicesComboBox->addItems(devices);
}

void TelescopeClientINDIUI::onServerConnected()
{
    ui->serverSettings->setEnabled(false);
    ui->connectButton->setText("Disconnect");
}

void TelescopeClientINDIUI::onServerDisconnected(int code)
{
    ui->devicesComboBox->clear();
    ui->serverSettings->setEnabled(true);
    ui->connectButton->setText("Connect");
}

