#include "mainwindow.h"

#include <QPushButton>
#include <QDomElement>


#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("VIRTUAL SMART SAFE");

    setupUi(this);

    m_sassion = SESSION_SAFE::IDLE;
    m_level = LEVEL_SAFE::NORMAL;


    m_base = new BD;
    connect(m_base, SIGNAL(signalLog(QString)), SLOT(addLog(QString)));

    m_base->start();

    tw_cash_in_cassete->setColumnCount(2);
    tw_cash_in_cassete->setRootIsDecorated(false);

    m_ser = new Server(this);
    m_ser->setMaxPendingConnections(1);
    connect(pb_server_run, &QAbstractButton::pressed, this, &MainWindow::slotServerRun);
    connect(m_ser, SIGNAL(signalLog(QString)), SLOT(addLog(QString)));
    connect(m_ser, SIGNAL(signalSend(QByteArray)), SLOT(slotData(QByteArray)));
    connect(this, SIGNAL(signalData(QByteArray)), m_ser, SLOT(slotSend(QByteArray)));


    connect(pb_run_inkas, &QAbstractButton::pressed, this, &MainWindow::slotInkasRun);
    connect(pb_engeneer, &QAbstractButton::pressed, this, &MainWindow::slotEngeneerRun);

    connect(pb_deposit_end, &QAbstractButton::pressed, this, &MainWindow::slotDepositEnd);
    connect(pb_deposit_10, &QAbstractButton::pressed, this, &MainWindow::slotDepositAdd);
    connect(pb_deposit_50, &QAbstractButton::pressed, this, &MainWindow::slotDepositAdd);
    connect(pb_deposit_100, &QAbstractButton::pressed, this, &MainWindow::slotDepositAdd);
    connect(pb_deposit_200, &QAbstractButton::pressed, this, &MainWindow::slotDepositAdd);
    connect(pb_deposit_500, &QAbstractButton::pressed, this, &MainWindow::slotDepositAdd);
    connect(pb_deposit_1000, &QAbstractButton::pressed, this, &MainWindow::slotDepositAdd);
    connect(pb_deposit_2000, &QAbstractButton::pressed, this, &MainWindow::slotDepositAdd);
    connect(pb_deposit_5000, &QAbstractButton::pressed, this, &MainWindow::slotDepositAdd);

    connect(cb_close_door_1, &QCheckBox::stateChanged, this, &MainWindow::slotStatusDoor1);
    connect(cb_close_door_2, &QCheckBox::stateChanged, this, &MainWindow::slotStatusDoor2);
    connect(cb_cassete_input, &QCheckBox::stateChanged, this, &MainWindow::slotStatusCassete);
    update();
}

void MainWindow::addLog(QString str)
{
    te_log->append(str);
}

// server
void MainWindow::slotServerRun()
{

    if(m_ser->isListening())
    {
        m_ser->close();
    }
    else
    {
        int nPort=lineEdit->text().toInt();
        if (!m_ser->listen(QHostAddress::Any, nPort))
        {
            addLog("Server Error: Unable to start the server:" + m_ser->errorString());
            slotServerStatus("Server Error: Unable to start the server:" + m_ser->errorString());
            m_ser->close();
        }
        else
        {
            addLog(QString("Server OK: port %1").arg(m_ser->serverPort()));
            slotServerStatus("Server OK");
        }
    }
}
void MainWindow::slotServerStatus(QString status)
{
    l_server_status->setText(status);
}
//for protokol safe
QByteArray MainWindow::code(QByteArray ba) // оборачивает последовательность в протокол смартсейфа
{
    QByteArray buf;
    for(int i=0; i<ba.length(); i++)
    {
        buf.append(ba.at(i));
    }
    uint32_t len=static_cast<uint32_t>(buf.length());
    static uint8_t const STX = '\x02';
    static uint8_t const ETX = '\x03';
    static uint8_t const NAK = '\x15';
    int buffer_size  = buf.length() + 7;
    uint32_t  lrc =  0;
    char *buffer = new char[buffer_size];
    int sz=0;
    buffer[sz++] = STX;
    QString s = QString("%1").arg(buf.length()).rightJustified(4, '0');
    char bytes[4];
    bytes[0] = len%256;
    len=len/256;
    bytes[1] = len%256;
    len=len/256;
    bytes[2] = len%256;
    len=len/256;
    bytes[3] = len%256;
    for(int k=0;k<4;k++){
        buffer[sz]= bytes[3-k];
        lrc ^=  static_cast<uint32_t>(buffer[sz]);
        sz++;
    }
    for (int k = 0; k < buf.length(); k++){
        buffer[sz]=static_cast<char>(buf.at(k));
        lrc ^= static_cast<uint32_t>(buffer[sz]);
        sz++;
    }
    buffer[sz++]=ETX;
    lrc ^= ETX;
    buffer[sz++]=static_cast<char>(lrc);

    return QByteArray(buffer, buffer_size);
}
QByteArray MainWindow::decode(QByteArray ba) // убирает последовательность протокола и оставляет только данные
{
    // необходимо добавить проверки, но мне лень
    QByteArray buf;
    buf.append(ba.data()+5, ba.count()-7);
    return buf;
}

void MainWindow::slotData(QByteArray arr) // получение данных
{
    QByteArray buf=decode(arr);
    addLog("slotData");
    addLog(QString::fromStdString(buf.toStdString()));

    buf=XML_Response(buf);
    if(!buf.length())
    {
        addLog("Error xml");
    }
    emit signalData(code(buf));
    update();
}


QByteArray MainWindow::XML_Response(QByteArray ar)
{
    QDomDocument doc;

    if (!doc.setContent(ar))
    {
        addLog("NOT XML content");
        return ""; //Response_Error()
    }
    // поиск базового блока
    QDomElement response_elem = doc.firstChildElement("CardServiceRequest");
    if(response_elem.isNull())
    {
        addLog("Wrong XML data none ==CardServiceRequest==");
        return "";
    }
    //поиск атрибутов
    QDomNamedNodeMap attrib = response_elem.attributes();
    if(!attrib.contains("RequestType"))
    {
        addLog("Wrong XML attribute none ==RequestType==");
        return "";
    }
    QDomNode dn_RequestType = attrib.namedItem("RequestType");
    if(dn_RequestType.isNull())
    {
        addLog("Wrong XML DomNode none ==RequestType==");
        return "";
    }
    

    if(dn_RequestType.nodeValue()=="SafeStatus")
    {
        m_request_type=REQUEST_TYPE::SAFESTATUS;
    }
    else if(dn_RequestType.nodeValue()=="SafeStatusEx")
    {
        m_request_type=REQUEST_TYPE::SAFESTATUSEX;
    }
    else if(dn_RequestType.nodeValue()=="SafeDepositSum")
    {
        m_request_type=REQUEST_TYPE::SAFEDEPOSITSUM;
    }
    else if(dn_RequestType.nodeValue()=="SafeEncashment")
    {
        m_request_type=REQUEST_TYPE::SAFEENCASHMENT;
    }
    else if(dn_RequestType.nodeValue()=="SafeDepositBegin")
    {
        m_request_type=REQUEST_TYPE::SAFEDEPOSITBEGIN;
    }
    else if(dn_RequestType.nodeValue()=="SafeDepositInfo")
    {
        m_request_type=REQUEST_TYPE::SAFEDEPOSITINFO;
    }
    else if(dn_RequestType.nodeValue()=="SafeTimeSet")
    {
        m_request_type=REQUEST_TYPE::SAFETIMESET;
    }
    else if(dn_RequestType.nodeValue()=="SafeDepositEnd")
    {
        m_request_type=REQUEST_TYPE::SAFEDEPOSITEND;
    }
    else if(dn_RequestType.nodeValue()=="SafeTransactionWithOutBank")
    {
        m_request_type=REQUEST_TYPE::STWOB;
    }
    
    QDomNode dn_ApplicationSender = attrib.namedItem("ApplicationSender");
    if(dn_ApplicationSender.isNull())
    {
        addLog("Wrong XML DomNode none ==ApplicationSender==");
        return "";
    }
    QDomNode dn_RequestID = attrib.namedItem("RequestID");
    if(dn_RequestID.isNull())
    {
        addLog("Wrong XML DomNode none ==RequestID==");
        return "";
    }
    QDomNode dn_WorkstationID = attrib.namedItem("WorkstationID");
    if(dn_WorkstationID.isNull())
    {
        addLog("Wrong XML DomNode none ==WorkstationID==");
        return "";
    }

    // формирование ответа
    QByteArray res;
    QXmlStreamWriter xml(&res);


    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(-1);
    xml.writeStartDocument();
    xml.writeStartElement("","CardServiceResponce");
    Response_Base(xml);
    xml.writeAttribute("RequestType", dn_RequestType.nodeValue());
    xml.writeAttribute("ApplicationSender",dn_ApplicationSender.nodeValue());
    xml.writeAttribute("OverallResult","Success");
    xml.writeAttribute("RequestID",dn_RequestID.nodeValue());
    xml.writeAttribute("WorkstationID",dn_WorkstationID.nodeValue());
    Response_Tender(xml);
    Response_Terminal(xml);


    // формирование полей в зависимости от запроса
    if(dn_RequestType.nodeValue()==QString("SafeStatus")) //Краткий запрос состояния
    {
        // только базовая посылка
    }
    else if(dn_RequestType.nodeValue()==QString("SafeStatusEx")) //Расширенный запрос состояния
    {
        Response_SafeStatusEx(xml);
    }
    else if(dn_RequestType.nodeValue()==QString("SafeEncashment")) //Запрос инкасации устройства
    {

        // current or last
        QDomElement POSdata_elem = response_elem.firstChildElement("POSdata");
        if(POSdata_elem.isNull())
        {
            addLog("Wrong XML data none ==POSdata==");
            return "";
        }
        QDomElement TypeIncassation_elem = POSdata_elem.firstChildElement("TypeIncassation");
        if(TypeIncassation_elem.isNull())
        {
            addLog("Wrong XML data none ==TypeIncassation_elem==");
            return "";
        }

/*
        QString str_d;
        QTextStream stream_d(&str_d);
        TypeIncassation_elem.save(stream_d, 4);
        qDebug()<<TypeIncassation_elem.tagName()<<str_d;
*/
        if(TypeIncassation_elem.text()==QString("Current"))
        {
            Response_SafeEncashment(xml, true);
        }
        else
        {
            Response_SafeEncashment(xml, false);
        }

    }
    else if(dn_RequestType.nodeValue()==QString("SafeDepositBegin")) //Запрос на разрешение начала внесения
    {
        if(m_sassion==SESSION_SAFE::IDLE)
        {
            m_sassion=SESSION_SAFE::CASHIER;
            m_base->new_deposit();
        }
    }
    else if(dn_RequestType.nodeValue()==QString("SafeDepositEnd")) //Запрос о завершении внесения
    {
        if(m_sassion==SESSION_SAFE::CASHIER)
        {
            m_sassion=SESSION_SAFE::IDLE;
        }
    }
    else if(dn_RequestType.nodeValue()==QString("SafeDepositInfo")) //Запрос о статусе внесения
    {
        Response_SafeEncashment(xml);
    }
    else if(dn_RequestType.nodeValue()==QString("SafeDepositSum")) //Запрос о сумме внесения
    {

    }
    else if(dn_RequestType.nodeValue()==QString("SafeTimeSet")) //Запрос на установку времени СС
    {
    //---
    }
    else if(dn_RequestType.nodeValue()==QString("SafeTransactionWithOutBank")) //Запрос списка непрошедших транзакций
    {

    }
    else
    {
        return "";
    }

    xml.writeEndElement(); // close CardServiceResponce
    addLog(QString::fromStdString(res.toStdString()));
    return res;
}
QString MainWindow::Response_TimeStamp_now()
{
    QDateTime now = QDateTime::currentDateTime();
    int offset = now.offsetFromUtc();
    now.setOffsetFromUtc(offset);
    return now.toString(Qt::ISODate);
}
void MainWindow::Response_Tender(QXmlStreamWriter &x)
{

    int cur, i;
    uint sum;
    QList<QPair<QString,Bills> > gl;

    switch(m_request_type)
    {

        case REQUEST_TYPE::SAFESTATUS:
        case REQUEST_TYPE::SAFESTATUSEX:
            x.writeStartElement("","Tender");
            x.writeAttribute("LanguageCode", "en");

            switch(m_level)
            {
                case LEVEL_SAFE::NORMAL:
                    x.writeTextElement("Level", "Normal");break;
                case LEVEL_SAFE::WARNING:
                    x.writeTextElement("Level", "Warning");break;
                case LEVEL_SAFE::ERROR:
                    x.writeTextElement("Level", "Error");break;
            }
            switch(m_sassion)
            {
                case SESSION_SAFE::IDLE:
                    x.writeTextElement("Session", "Idle");break;
                case SESSION_SAFE::CASHIER:
                    x.writeTextElement("Session", "Cashier");break;
                case SESSION_SAFE::ENCASHMENT:
                    x.writeTextElement("Session", "Encashment");break;
                case SESSION_SAFE::ENGENEER:
                    x.writeTextElement("Session", "Engeneer");break;
            }
            x.writeTextElement("Status", getStatus());
            x.writeTextElement("Message", getMessage());
            x.writeTextElement("TimeStamp", Response_TimeStamp_now());
            x.writeTextElement("StationID", "POS");
            x.writeTextElement("StationNumber", "1");
            x.writeEndElement(); // close Tender
            break;

        case REQUEST_TYPE::SAFEENCASHMENT:
            x.writeStartElement("","Tender");
            x.writeAttribute("BankAccepted", "False");
            if(m_sassion==SESSION_SAFE::ENCASHMENT)
            {
                x.writeTextElement("Incoming", "True");
            }
            else
            {
                x.writeTextElement("Incoming", "False");
            }
            x.writeEndElement(); // close Tender
            break;

        case REQUEST_TYPE::SAFEDEPOSITBEGIN:
            x.writeStartElement("","Tender");
            if(m_sassion==SESSION_SAFE::ENCASHMENT)
            {
                x.writeAttribute("Accepted", "True");
            }
            else
            {
                x.writeAttribute("Accepted", "False");
            }
            x.writeTextElement("Number", QString("%1").arg(m_base->get_last_inkas_number()));
            x.writeEndElement(); // close Tender
            break;

        case REQUEST_TYPE::SAFEDEPOSITINFO:
            x.writeStartElement("","Tender");
            x.writeAttribute("BankAccepted", "False");
            if(m_sassion==SESSION_SAFE::ENCASHMENT)
            {
                x.writeTextElement("Incoming", "True");
            }
            else
            {
                x.writeTextElement("Incoming", "False");
            }
            x.writeEndElement(); // close Tender
            break;

        case REQUEST_TYPE::SAFEDEPOSITSUM:

            x.writeStartElement("","Tender");
            x.writeAttribute("BankAccepted", "False");

            if(m_sassion==SESSION_SAFE::ENCASHMENT)
            {
                x.writeTextElement("Incoming", "True");
            }
            else
            {
                x.writeTextElement("Incoming", "False");
            }

            cur=m_base->get_current_id();
            sum=0;
            gl = m_base->get_list_denaminations_in_cassete(cur);
            for(i=0; i<gl.count(); i++)
            {

                sum= static_cast<Bills>(gl.at(i).second).sum();
            }
            x.writeTextElement("Sum", QString("%1").arg(sum));
            x.writeTextElement("TimeStamp", Response_TimeStamp_now());
            x.writeTextElement("Number", QString("%1").arg(m_base->get_last_inkas_number()));

            x.writeEndElement(); // close Tender

            break;


        case REQUEST_TYPE::SAFETIMESET:
            x.writeStartElement("","Tender");
            x.writeTextElement("TimeStamp", Response_TimeStamp_now());
            x.writeEndElement(); // close Tender
            break;


        case REQUEST_TYPE::STWOB:
        case REQUEST_TYPE::SAFEDEPOSITEND:
        default:
        break;
    }

}

QString MainWindow::getStatus()
{
    if(m_sassion==SESSION_SAFE::ENGENEER)
    {
        return "None";
    }
    if(cb_cassete_input->checkState()==Qt::Unchecked)
    {
       return  "ErrorCashIn";
    }
    if(cb_close_door_1->checkState()==Qt::Unchecked)
    {
       return  "WarningSensor1";
    }
    if(cb_close_door_2->checkState()==Qt::Unchecked)
    {
       return  "WarningSensor2";
    }

    return "ErrorOFFLINE";
}
QString MainWindow::getMessage()
{
    if(m_sassion==SESSION_SAFE::ENGENEER)
    {
        return "Ok";
    }
    if(cb_cassete_input->checkState()==Qt::Unchecked)
    {
        return "Критическая ошибка купюроприемника";
    }
    if(cb_close_door_1->checkState()==Qt::Unchecked)
    {
       return  "Открыта верхняя дверь сейфа";
    }
    if(cb_close_door_2->checkState()==Qt::Unchecked)
    {
       return  "Открыта нижняя дверь сейфа";
    }
    return "Ошибка связи с банком";
}

void MainWindow::Response_Base(QXmlStreamWriter &x)
{
    x.writeNamespace("http://www.w3.org/2001/XMLSchema-instance","xsi");
    x.writeDefaultNamespace("http://www.nrf-arts.org/IXRetail/namespace");
    x.writeNamespace("http://www.ifsf.org/","IFSF");
    x.writeNamespace("http://www.w3.org/2001/XMLSchema","xs");
    x.writeAttribute("http://www.w3.org/2001/XMLSchema-instance", "schemaLocation",".\\CardRequest.xsd");

}

void MainWindow::Response_Terminal(QXmlStreamWriter &x)
{
    x.writeStartElement("","Terminal");
    x.writeAttribute("TerminalBatch", "000001");
    x.writeAttribute("TerminalID", "00000000"); // номер сейфа
    x.writeAttribute("STAN", "053351"); // счетчик
    x.writeAttribute("Version", "143M"); // версия
    x.writeEndElement(); // close Terminal
}
void MainWindow::Response_SafeStatusEx(QXmlStreamWriter &x)  // необходимо собирать данные из БД по текущей смене сейфа
{
    x.writeStartElement("","Extended");
    int c = m_base->get_count_bils();
    x.writeAttribute("NotesCountForFull", QString("%1").arg(2000-c));
    x.writeAttribute("NextIncassation", "");
    x.writeAttribute("TimeToIncassationInMinutes", "");
    x.writeAttribute("MaxNotesCount", "2000");
    x.writeAttribute("LastIncassation", m_base->get_last_inkas_datetime());
    x.writeAttribute("NotesCount", QString("%1").arg(c));
    Response_Notes(x);
    x.writeEndElement(); // close Extended
}
void MainWindow::Response_Notes(QXmlStreamWriter &x, int Num)
{
    int cur;
    if(Num==-1)
    {
        cur=m_base->get_current_id();
    }
    else
    {
        cur=Num;
    }

    QList<QPair<QString,Bills> > gl = m_base->get_list_denaminations_in_cassete(cur);
    for(int i=0; i<gl.count(); i++)
    {
        x.writeStartElement("","Notes");
        if(gl.at(i).first.length())
        {
            x.writeAttribute("CurrencyCode", gl.at(i).first);
        }
        else
        {
            x.writeAttribute("CurrencyCode", "RUR");
        }
        x.writeAttribute("SumInBank", "0");
        QString s = QString("%1").arg(static_cast<Bills>(gl.at(i).second).sum());
        x.writeAttribute("SumWithOutBank", s);
        x.writeAttribute("Count", QString("%1").arg(static_cast<Bills>(gl.at(i).second).count()));
        x.writeAttribute("Sum", s);

        if(gl.at(i).second.count_10)
        {
            Response_Line(x, 10, gl.at(i).second.count_10);
        }
        if(gl.at(i).second.count_50)
        {
            Response_Line(x, 50, gl.at(i).second.count_50);
        }
        if(gl.at(i).second.count_100)
        {
            Response_Line(x, 100, gl.at(i).second.count_100);
        }
        if(gl.at(i).second.count_200)
        {
            Response_Line(x, 200, gl.at(i).second.count_200);
        }
        if(gl.at(i).second.count_500)
        {
            Response_Line(x, 500, gl.at(i).second.count_500);
        }
        if(gl.at(i).second.count_1000)
        {
            Response_Line(x, 1000, gl.at(i).second.count_1000);
        }
        if(gl.at(i).second.count_2000)
        {
            Response_Line(x, 2000, gl.at(i).second.count_2000);
        }
        if(gl.at(i).second.count_5000)
        {
            Response_Line(x, 5000, gl.at(i).second.count_5000);
        }
        x.writeEndElement(); //Notes
    }

}
void MainWindow::Response_Line(QXmlStreamWriter &x, int den, int count)
{
    x.writeStartElement("","Line");
    x.writeAttribute("Denomination", QString("%1").arg(den));
    x.writeAttribute("Sum", QString("%1").arg(den*count));
    x.writeCharacters(QString("%1").arg(count));
    x.writeEndElement(); //Line
}

void MainWindow::Response_SafeEncashment(QXmlStreamWriter &x, bool current)
{
    x.writeStartElement("","ListNotes");
    x.writeAttribute("DateTime", m_base->get_last_inkas_datetime());
    x.writeAttribute("Number", QString("%1").arg(m_base->get_last_inkas_number()));
    if(current)
    {
        x.writeAttribute("NotesCount", QString("%1").arg(m_base->get_count_bils()));
        Response_Notes(x, -1);
    }
    else
    {
        int last=m_base->get_last_inkas_number();
        x.writeAttribute("NotesCount", QString("%1").arg(m_base->get_count_bils(last)));
        Response_Notes(x, last);
    }

    x.writeEndElement(); //ListNotes
}





// deposit
void MainWindow::slotDepositStatus()
{

}
void MainWindow::slotDepositEnd()
{
    if(m_sassion==SESSION_SAFE::CASHIER)
    {
        m_sassion=SESSION_SAFE::IDLE;
    }
    update();
}
void MainWindow::slotDepositAdd()
{
    if(m_sassion!=SESSION_SAFE::CASHIER)
    {
        return;
    }
    int nom;

    if(pb_deposit_10->isDown())
    {
        nom=10;
    }
    else if(pb_deposit_50->isDown())
    {
        nom=50;
    }
    else if(pb_deposit_100->isDown())
    {
        nom=100;
    }
    else if(pb_deposit_200->isDown())
    {
        nom=200;
    }
    else if(pb_deposit_500->isDown())
    {
        nom=500;
    }
    else if(pb_deposit_1000->isDown())
    {
        nom=1000;
    }
    else if(pb_deposit_2000->isDown())
    {
        nom=2000;
    }
    else if(pb_deposit_5000->isDown())
    {
        nom=5000;
    }
    else
    {
        nom=0;
    }

    m_base->insert_denomination(nom, "RUR");
    update();
}


void MainWindow::update()
{

    l_deposit_status->setText("Внесение запрещено");
    switch(m_sassion)
    {
    case SESSION_SAFE::IDLE:
        la_status_sassion->setText("IDLE - свободный режим");
        break;
    case SESSION_SAFE::CASHIER:
        la_status_sassion->setText("CASHIER - режим приема наличности");
        l_deposit_status->setText("Внесение разрешено");
        break;
    case SESSION_SAFE::ENCASHMENT:
        la_status_sassion->setText("ENCASHMENT - режим инкассации");
        break;
    case SESSION_SAFE::ENGENEER:
        la_status_sassion->setText("ENGENEER - режим обслуживания");
        break;
    }

    switch(m_level)
    {
    case LEVEL_SAFE::NORMAL:
        l_status_level->setText("NORMAL - устройство полнофункционально");
        break;
    case LEVEL_SAFE::WARNING:
        l_status_level->setText("WARNING - устройство функционирует с ограниченными возможностями");
        break;
    case LEVEL_SAFE::ERROR:
        l_status_level->setText("ERROR - устройство не может выполнять свои функции");
        break;

    }

    tw_cash_in_cassete->clear();
    QList<QPair<QString,Bills> > gl = m_base->get_list_denaminations_in_cassete();
    for(int i=0; i<gl.count(); i++)
    {
        QTreeWidgetItem *tli = new QTreeWidgetItem();
        tli->setText(0, gl.at(i).first);
        tw_cash_in_cassete->insertTopLevelItem(i,tli);
        tli->insertChild(0, new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), { "10", QString("%1").arg(gl.at(i).second.count_10)}));
        tli->insertChild(1, new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), { "50", QString("%1").arg(gl.at(i).second.count_50)}));
        tli->insertChild(2, new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), { "100", QString("%1").arg(gl.at(i).second.count_100)}));
        tli->insertChild(3, new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), { "200", QString("%1").arg(gl.at(i).second.count_200)}));
        tli->insertChild(4, new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), { "500", QString("%1").arg(gl.at(i).second.count_500)}));
        tli->insertChild(5, new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), { "1000", QString("%1").arg(gl.at(i).second.count_1000)}));
        tli->insertChild(6, new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), { "2000", QString("%1").arg(gl.at(i).second.count_2000)}));
        tli->insertChild(7, new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), { "5000", QString("%1").arg(gl.at(i).second.count_5000)}));

    }
    tw_cash_in_cassete->expandAll();

}

// inkas
void MainWindow::slotInkasRun()
{
    if((m_sassion==SESSION_SAFE::ENGENEER)||(m_sassion==SESSION_SAFE::CASHIER))
    {
        return;
    }
    if(m_sassion==SESSION_SAFE::IDLE)
    {
        m_sassion=SESSION_SAFE::ENCASHMENT;
        pb_engeneer->setEnabled(false);
    }
    else if(m_sassion==SESSION_SAFE::ENCASHMENT)
    {
        m_sassion=SESSION_SAFE::IDLE;
        pb_engeneer->setEnabled(true);
    }
    update();
}
// Engeneer
void MainWindow::slotEngeneerRun()
{
    if((m_sassion==SESSION_SAFE::ENCASHMENT)||(m_sassion==SESSION_SAFE::CASHIER))
    {
        return;
    }
    if(m_sassion==SESSION_SAFE::IDLE)
    {
        m_sassion=SESSION_SAFE::ENGENEER;
        pb_run_inkas->setEnabled(false);
    }
    else if(m_sassion==SESSION_SAFE::ENGENEER)
    {
        m_sassion=SESSION_SAFE::IDLE;
        pb_run_inkas->setEnabled(true);
    }
    update();
}

//door
void MainWindow::slotStatusDoor1(int state)
{
    if(state==Qt::Unchecked)
    {

    }
    else if(state==Qt::Checked)
    {

    }
    update();
}
void MainWindow::slotStatusDoor2(int state)
{
    qDebug()<<"slotStatusDoor2";
    if(state==Qt::Unchecked)
    {
        cb_cassete_input->setEnabled(true);
    }
    else if(state==Qt::Checked)
    {
        cb_cassete_input->setEnabled(false);
    }
    update();
}

//cassete
void MainWindow::slotStatusCassete(int state)
{
    if(state==Qt::Unchecked)
    {
        m_base->end_cassete();
    }
    else if(state==Qt::Checked)
    {
        m_base->new_cassete();
    }
    update();
}
