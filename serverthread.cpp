#include "serverthread.h"

#include <QtNetwork>
#include <QDateTime>

ServerThread::ServerThread(int socketDescriptor, QObject *parent)
    : QObject(parent), socketDescriptor(socketDescriptor)
{
    socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);
    connect(socket, SIGNAL(disconnected()), SLOT(disconnected()));
    connect(socket, SIGNAL(readyRead()), SLOT(readyRead()));
}

void ServerThread::disconnected()
{
    emit signalLog(QString("disconnection %1").arg(socketDescriptor));
    emit signalEnd(socketDescriptor);
}
void ServerThread::readyRead()
{
    emit signalLog(QString("reading... %1").arg(socketDescriptor));
    QByteArray ba=socket->readAll();
    //emit signalLog(QString::fromStdString(ba.toStdString()));
    emit signalSend(ba);

}
void ServerThread::slotSend(QByteArray ba)
{
    emit signalLog(QString("writing... %1").arg(socketDescriptor));
    emit signalLog(QString::fromStdString(ba.toStdString()));
    socket->write(ba);

    socket->close();
}
