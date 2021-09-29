#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include "serverthread.h"

class Server : public QTcpServer
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = 0);


protected:
    void incomingConnection (qintptr socketDescriptor) override;


public slots:
    void slotSend(QByteArray ba);

    void slotCloseConnection(qintptr socketDescriptor);

signals:
    void signalLog(QString);
    void signalSend(QByteArray);
    void signalResend(QByteArray);
private:
    ServerThread *st_my;
    QThread *thread;

};

#endif // SERVER_H
