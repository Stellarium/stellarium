#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QDebug>

class MosaicTcpServer : public QTcpServer {
    Q_OBJECT

public:
    MosaicTcpServer(QObject *parent = nullptr) : QTcpServer(parent) {
        connect(this, &QTcpServer::newConnection, this, &MosaicTcpServer::onNewConnection);
    }

    void startServer(quint16 port) {
        if (!this->listen(QHostAddress::Any, port)) {
            qDebug() << "Server failed to start: " << this->errorString();
        } else {
            qDebug() << "Server started on port" << port;
        }
    }

private slots:
    void onNewConnection() {
        QTcpSocket *client = this->nextPendingConnection();
        connect(client, &QTcpSocket::readyRead, this, [client, this]() {
            QByteArray data = client->readAll();
            QString message(data);

            // Parse RA, Dec, and rotation
            QStringList params = message.split(',');
            if (params.size() == 3) {
                bool ok1, ok2, ok3;
                double ra = params[0].toDouble(&ok1);
                double dec = params[1].toDouble(&ok2);
                double rot = params[2].toDouble(&ok3);

                if (ok1 && ok2 && ok3) {
                    // Update plugin settings
                    emit newValuesReceived(ra, dec, rot);
                }
            }
        });

        connect(client, &QTcpSocket::disconnected, client, &QTcpSocket::deleteLater);
    }

signals:
    void newValuesReceived(double ra, double dec, double rot);
};
