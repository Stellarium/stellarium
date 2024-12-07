#ifndef MOSAICTCPSERVER_HPP
#define MOSAICTCPSERVER_HPP

#include <QTcpServer>
#include <QTcpSocket>

class MosaicTcpServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit MosaicTcpServer(QObject *parent = nullptr);
    void startServer(quint16 port);

signals:
    void newValuesReceived(QString name, double ra, double dec, double rot);

private slots:
    void onNewConnection();

private:
    void processClientData(QTcpSocket *client);
};

#endif // MOSAICTCPSERVER_HPP
