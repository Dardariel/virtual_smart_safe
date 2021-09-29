#ifndef SERVERTHREAD_H
#define SERVERTHREAD_H


#include <QObject>
#include <QThread>
#include <QTcpSocket>

class ServerThread : public QObject
{
    Q_OBJECT
public:
    ServerThread(int socketDescriptor, QObject *parent = nullptr);



signals:
    void error(QTcpSocket::SocketError socketError);

    void signalLog(QString);
    void signalEnd(qintptr socketDescriptor);

    void signalSend(QByteArray);

private:
    int socketDescriptor;
    QTcpSocket *socket;


public slots:
    void disconnected();
    void readyRead();
    void slotSend(QByteArray ba);


};

#endif // SERVERTHREAD_H
