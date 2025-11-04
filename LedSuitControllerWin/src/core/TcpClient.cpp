#include "include/core/TcpClient.h"
#include <QDebug>

TcpClient::TcpClient(const QString& hostAddress, quint16 port, QObject* parent)
    : QObject(parent), hostAddress(hostAddress), port(port), socket(new QTcpSocket(this)) {
    connect(socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), this,
            [this](QAbstractSocket::SocketError) {
                emit errorOccurred(socket->errorString());
            });
}

TcpClient::~TcpClient() {
    disconnectFromServer();
}

bool TcpClient::connectToServer() {
    socket->connectToHost(hostAddress, port);
    if (!socket->waitForConnected(3000)) { // Wait up to 3 seconds for connection
        qWarning() << "Connection failed:" << socket->errorString();
        return false;
    }
    qWarning() << "Connected to" << hostAddress << ":" << port;
    return true;
}

bool TcpClient::sendData(const std::vector<uint8_t>& data) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "Socket is not connected!";
        return false;
    }

    QByteArray byteArray(reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()));

    // Log the data being sent as hex
    QString byteString;
    for (uint8_t byte : data) {
        byteString += QString("%1 ").arg(byte, 2, 16, QChar('0')).toUpper();
    }
    qWarning() << "Sending data (hex):" << byteString.trimmed();

    qint64 bytesWritten = socket->write(byteArray);
    if (bytesWritten == -1) {
        qWarning() << "Failed to send data:" << socket->errorString();
        return false;
    }
    if (!socket->waitForBytesWritten(3000)) { // Wait up to 3 seconds for data to be sent
        qWarning() << "Timeout while sending data!";
        return false;
    }

    qWarning() << "Sent" << bytesWritten << "bytes to" << hostAddress << ":" << port;
    return true;
}


void TcpClient::disconnectFromServer() {
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->disconnectFromHost();
        if (socket->state() == QAbstractSocket::ConnectedState) {
            socket->waitForDisconnected(3000); // Wait up to 3 seconds for disconnection
        }
        qWarning() << "Disconnected from" << hostAddress << ":" << port;
    }
}

void TcpClient::onReadyRead() {
    QByteArray data = socket->readAll();
    qWarning() << "Received data from server:" << data;
}

