#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <vector>
#include <cstdint>

class TcpClient : public QObject {
    Q_OBJECT

public:
    explicit TcpClient(const QString& hostAddress, quint16 port, QObject* parent = nullptr);
    ~TcpClient();

    bool connectToServer();
    bool sendData(const std::vector<uint8_t>& data);
    void disconnectFromServer();

signals:
    void errorOccurred(const QString& error);

private slots:
    void onReadyRead();

private:
    QString hostAddress;
    quint16 port;
    QTcpSocket* socket;
};

#endif // TCPCLIENT_H

