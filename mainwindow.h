#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QtXml>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "ui_mainwindow.h"
#include "server.h"
#include "bd.h"

enum SESSION_SAFE
{
    IDLE, //свободный режим
    CASHIER, //режим приема наличности
    ENCASHMENT, //режим инкассации
    ENGENEER // сервисный режим
};


enum LEVEL_SAFE
{
    NORMAL, //устройство полнофункционально
    WARNING, //устройство функционирует с ограниченными возможностями
    ERROR // устройство не может выполнять свои функции
};

class MainWindow : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private:

    // state emul
    SESSION_SAFE m_sassion; // текущее состояние эмулятора
    LEVEL_SAFE m_level; // текущий уровень состояния эмулятора

    void update();

    // for bd
    BD *m_base;

    //for Server
    Server *m_ser;


    // for work xml
    QByteArray XML_Response(QByteArray ar);

    QString Response_TimeStamp_now();

    /*
    enum TypeError
    {

    };

    QByteArray Response_Error(TypeError t);
    */

    void Response_Base(QXmlStreamWriter &x); // базовое наполнение xml
    void Response_Tender(QXmlStreamWriter &x); // подготовка блока tender по текущему состоянию сейфа
    void Response_Terminal(QXmlStreamWriter &x); // подготовка блока Terminal по текущему состоянию сейфа
    void Response_Notes(QXmlStreamWriter &x, int Num=-1); // подготовка блока Notes, список внесенных купюр
    void Response_Line(QXmlStreamWriter &x, int den, int count); // подготовка блока Line, список внесенных купюр

    void Response_SafeStatusEx(QXmlStreamWriter &x); //подготовка блока расширенного ответа состояния сейфа
    void Response_SafeEncashment(QXmlStreamWriter &x, bool current=true); //подготовка блока ответа на запрос инкасации. если current=0 -> last

    QString getStatus();
    QString getMessage();

    //for protokol safe
    QByteArray code(QByteArray ba); // оборачивает последовательность в протокол смартсейфа
    QByteArray decode(QByteArray ba); // убирает последовательность протокола и оставляет только данные
    
    
    enum TENDER_TYPE
    {
        NO_TENDER,
        TENDER_SMALL,
        TENDER_SMALL_2,
        TENDER_TIME,
        TENDER
    };
    TENDER_TYPE m_tender_type;

public slots:
    void addLog(QString str); // отображение лога

    void slotData(QByteArray arr); // получение данных. Осносвная работа с клиентом


    // server
    void slotServerRun();
    void slotServerStatus(QString status);

    // deposit
    void slotDepositStatus();
    void slotDepositEnd();
    void slotDepositAdd();

    // inkas
    void slotInkasRun();

    // Engeneer
    void slotEngeneerRun();

    //door
    void slotStatusDoor1(int state);
    void slotStatusDoor2(int state);

    //cassete
    void slotStatusCassete(int state);


signals:
    void signalData(QByteArray); // отправка данных

};
#endif // MAINWINDOW_H
