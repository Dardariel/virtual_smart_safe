#include "server.h"

#include <QThread>

Server::Server(QObject *parent) : QTcpServer(parent)
{
}
void Server::incomingConnection(qintptr socketDescriptor)
{
    signalLog(QString("new connection %1").arg(socketDescriptor));

    thread = new QThread;
    st_my = new ServerThread(socketDescriptor);
    st_my->moveToThread(thread);
    thread->start();
    connect(st_my, SIGNAL(signalLog(QString)), SIGNAL(signalLog(QString)));
    //connect(st_my, SIGNAL(signalEnd()), thread, SLOT(terminate()));
    connect(st_my, SIGNAL(signalSend(QByteArray)), SIGNAL(signalSend(QByteArray)));
    connect(this, SIGNAL(signalResend(QByteArray)), st_my, SLOT(slotSend(QByteArray)));


}
void Server::slotCloseConnection(qintptr socketDescriptor)
{
    Q_UNUSED(socketDescriptor)
    disconnect(st_my, SIGNAL(signalLog(QString)), this, SIGNAL(signalLog(QString)));
    disconnect(st_my, SIGNAL(signalEnd()), thread, SLOT(terminate()));
    disconnect(st_my, SIGNAL(signalSend(QByteArray)), this, SIGNAL(signalSend(QByteArray)));
    disconnect(this, SIGNAL(signalResend(QByteArray)), st_my, SLOT(slotSend(QByteArray)));
}
void Server::slotSend(QByteArray ba)
{
    emit signalResend(ba);
}

