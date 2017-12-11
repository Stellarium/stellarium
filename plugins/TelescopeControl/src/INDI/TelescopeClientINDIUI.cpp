#include "TelescopeClientINDIUI.hpp"
#include "ui_TelescopeClientINDIUI.h"

TelescopeClientINDIUI::TelescopeClientINDIUI(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TelescopeClientINDIUI)
{
    ui->setupUi(this);
    QObject::connect(ui->connectButton, &QPushButton::clicked, this, &TelescopeClientINDIUI::onConnectionButtonClicked);
    QObject::connect(&mConnection, &INDIConnection::devicesChanged, this, &TelescopeClientINDIUI::onDevicesChanged);
    QObject::connect(&mConnection, &INDIConnection::disconnected, this, &TelescopeClientINDIUI::onServerDisconnected);
}

TelescopeClientINDIUI::~TelescopeClientINDIUI()
{
    delete ui;
}

QString TelescopeClientINDIUI::host() const
{
    return ui->lineEditHostName->text();
}

int TelescopeClientINDIUI::port() const
{
    return ui->spinBoxTCPPort->value();
}

QString TelescopeClientINDIUI::selectedDevice() const
{
    return ui->devicesComboBox->currentText();
}

void TelescopeClientINDIUI::onConnectionButtonClicked()
{
    if (mConnection.isServerConnected())
        mConnection.disconnectServer();

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

void TelescopeClientINDIUI::onServerDisconnected(int code)
{
    ui->devicesComboBox->clear();
}

