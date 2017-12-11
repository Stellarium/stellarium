#include "TelescopeClientINDIWidget.hpp"
#include "ui_TelescopeClientINDIWidget.h"

TelescopeClientINDIWidget::TelescopeClientINDIWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TelescopeClientINDIWidget)
{
    ui->setupUi(this);
    QObject::connect(ui->connectButton, &QPushButton::clicked, this, &TelescopeClientINDIWidget::onConnectionButtonClicked);
    QObject::connect(&mConnection, &INDIConnection::newDeviceReceived, this, &TelescopeClientINDIWidget::onDevicesChanged);
    QObject::connect(&mConnection, &INDIConnection::removeDeviceReceived, this, &TelescopeClientINDIWidget::onDevicesChanged);
    QObject::connect(&mConnection, &INDIConnection::serverDisconnectedReceived, this, &TelescopeClientINDIWidget::onServerDisconnected);
}

TelescopeClientINDIWidget::~TelescopeClientINDIWidget()
{
    delete ui;
}

QString TelescopeClientINDIWidget::host() const
{
    return ui->lineEditHostName->text();
}

void TelescopeClientINDIWidget::setHost(const QString &host)
{
    ui->lineEditHostName->setText(host);
}

int TelescopeClientINDIWidget::port() const
{
    return ui->spinBoxTCPPort->value();
}

void TelescopeClientINDIWidget::setPort(int port)
{
    ui->spinBoxTCPPort->setValue(port);
}

QString TelescopeClientINDIWidget::selectedDevice() const
{
    return ui->devicesComboBox->currentText();
}

void TelescopeClientINDIWidget::setSelectedDevice(const QString &device)
{
    ui->devicesComboBox->setCurrentText(device);
}

void TelescopeClientINDIWidget::onConnectionButtonClicked()
{
    if (mConnection.isServerConnected())
        mConnection.disconnectServer();

    QString host = ui->lineEditHostName->text();
    QString port = ui->spinBoxTCPPort->text();
    mConnection.setServer(host.toStdString().c_str(), port.toInt());
    mConnection.connectServer();
}

void TelescopeClientINDIWidget::onDevicesChanged()
{
    ui->devicesComboBox->clear();
    auto devices = mConnection.devices();
    ui->devicesComboBox->addItems(devices);
}

void TelescopeClientINDIWidget::onServerDisconnected(int code)
{
    ui->devicesComboBox->clear();
}

