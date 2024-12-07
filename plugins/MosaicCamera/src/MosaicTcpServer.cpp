#include "MosaicTcpServer.hpp"
#include <QByteArray>
#include <QDebug>

MosaicTcpServer::MosaicTcpServer(QObject *parent) : QTcpServer(parent)
{
    connect(this, &MosaicTcpServer::newConnection, this, &MosaicTcpServer::onNewConnection);
}

void MosaicTcpServer::startServer(quint16 port)
{
    if (!this->listen(QHostAddress::Any, port)) {
        qDebug() << "[MosaicCamera] Server failed to start: " << this->errorString();
    } else {
        qDebug() << "[MosaicCamera] Server started on port" << port;
    }
}

void MosaicTcpServer::onNewConnection()
{
    QTcpSocket *client = this->nextPendingConnection();
    connect(client, &QTcpSocket::readyRead, this, [this, client]() {
        processClientData(client);
    });

    connect(client, &QTcpSocket::disconnected, client, &QTcpSocket::deleteLater);
}

void MosaicTcpServer::processClientData(QTcpSocket *client)
{
    QByteArray data = client->readAll();
    QString message(data);

    // Parse RA, Dec, and rotation
    QStringList params = message.split(',');
    if (params.size() == 4) {
        bool ok1, ok2, ok3;
        QString name = params[0];
        double ra = params[1].toDouble(&ok1);
        double dec = params[2].toDouble(&ok2);
        double rot = params[3].toDouble(&ok3);

        if (ok1 && ok2 && ok3) {
            // Emit the parsed values to be processed elsewhere
            emit newValuesReceived(name, ra, dec, rot);
        }
    }
}
