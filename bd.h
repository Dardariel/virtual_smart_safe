#ifndef BD_H
#define BD_H
#include <QObject>
#include <QtSql>


struct Bills
{
    unsigned int count_10;
    unsigned int count_50;
    unsigned int count_100;
    unsigned int count_200;
    unsigned int count_500;
    unsigned int count_1000;
    unsigned int count_2000;
    unsigned int count_5000;
    unsigned int count(){return count_10+count_50+count_100+count_200+count_500+count_1000+count_2000+count_5000;}
    unsigned int sum(){return 10*count_10+50*count_50+100*count_100+200*count_200+500*count_500+1000*count_1000+2000*count_2000+5000*count_5000;}
    void clear(){count_10=0; count_50=0; count_100=0; count_200=0; count_500=0; count_1000=0; count_2000=0; count_5000=0;}


    QString debug()
    {
        return QString("count_10=%1 ").arg(count_10)
                +QString("count_50=%1 ").arg(count_50)
                +QString("count_100=%1 ").arg(count_100)
                +QString("count_200=%1 ").arg(count_200)
                +QString("count_500=%1 ").arg(count_500)
                +QString("count_1000=%1 ").arg(count_1000)
                +QString("count_2000=%1 ").arg(count_2000)
                +QString("count_5000=%1 ").arg(count_5000);
    }
};


class BD : public QObject
{
    Q_OBJECT
public:
    BD();

    bool start();

    int get_last_deposit_id(){return m_deposit_id;}
    void new_deposit(){m_deposit_id++;}
    int get_current_id(){return m_current_id;}
    int get_last_id()
    {
        if(m_current_id==-1)
            return m_current_id;
        else
            return m_current_id-1;
    }

    void end_cassete();
    void new_cassete();

    int get_count_bils(int id=-1);
    QString get_last_inkas_datetime();
    int get_last_inkas_number();

    QList<QPair<QString,Bills> > get_list_denaminations_in_cassete(int currency=-1);

    void insert_denomination(int denomination, QString currency);

signals:
    void signalLog(QString);

private:

    bool control_BD();
    void make_BD();

    QString datetime_now()
    {
        QDateTime now = QDateTime::currentDateTime();
        int offset = now.offsetFromUtc();
        now.setOffsetFromUtc(offset);
        return now.toString(Qt::ISODate);
    }

    QSqlDatabase sdb;
    int m_current_id;
    int m_deposit_id;

};

#endif // BD_H
