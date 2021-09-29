#include "bd.h"

#include <QSqlQuery>
#include <QPair>
#include <QList>

BD::BD(): QObject()
{

}
bool BD::start()
{
    m_current_id=-1;
    m_deposit_id=0;
    sdb = QSqlDatabase::addDatabase("QSQLITE");
    sdb.setDatabaseName("virtual_smart_safe.sqlite");
    if (!sdb.open())
    {
          signalLog(sdb.lastError().text());
          return false;
    }

    if(!control_BD())
    {
        make_BD();
    }
    signalLog("BD start OK");
    return true;
}
bool BD::control_BD()
{
    QSqlQuery query("SELECT id, dt_start, dt_end FROM collection WHERE dt_end is null ORDER BY id DESC;", sdb);
    if(!query.exec())
    {
        return false;
    }
    else
    {
        if(query.next())
        {
            m_current_id=query.value(0).toInt();
            signalLog(QString("BD last open id=%1").arg(m_current_id));
        }
        else
        {
            new_cassete();
        }
    }

    query.prepare("SELECT id_collection, id_deposit, denomination, currency FROM deposit ORDER BY id_deposit DESC;");
    if(!query.exec())
    {
        return false;
    }
    else
    {
        if(query.next())
        {
            m_deposit_id=query.value(1).toInt();
            signalLog(QString("BD last open deposit=%1").arg(m_deposit_id));
        }
    }
    return true;
}
void BD::new_cassete()
{
     QSqlQuery query("SELECT id FROM collection ORDER BY id DESC;", sdb);
     if(query.next())
     {
         m_current_id=query.value(0).toInt()+1;
         signalLog(QString("New open id=%1").arg(m_current_id));
     }

     query.prepare(QString("INSERT INTO collection(id, dt_start) VALUES (%1, '%2');").arg(m_current_id).arg(datetime_now()));
     if(!query.exec())
     {
         signalLog("BD creating the new record ERROR:"+query.lastError().text());
         return;
     }
}
void BD::end_cassete()
{
    if(m_current_id==-1)
        return;
    QSqlQuery query(QString("UPDATE collection SET dt_end='%1' WHERE id=%2;").arg(datetime_now()).arg(m_current_id));
    if(!query.exec())
    {
        signalLog("BD close cassete ERROR:"+query.lastError().text());
        return;
    }
}
void BD::make_BD()
{
    QSqlQuery query("CREATE TABLE collection (id INTEGER NOT_NULL, dt_start TEXT NOT_NULL, dt_end TEXT);", sdb);
    if(!query.exec())
    {
        signalLog("BD make collection ERROR:"+query.lastError().text());
        return;
    }
    query.prepare("CREATE TABLE deposit (id_collection INTEGER NOT_NULL, id_deposit INTEGER NOT_NULL, denomination INTEGER NOT_NULL, currency TEXT NOT_NULL);");
    if(!query.exec())
    {
        signalLog("BD make deposit ERROR:"+query.lastError().text());
        return;
    }

    query.prepare("INSERT INTO collection(id, dt_start) VALUES (1, '"+datetime_now()+"');");
    if(!query.exec())
    {
        signalLog("BD creating the first record ERROR:"+query.lastError().text());
        return;
    }
}

void BD::insert_denomination(int denomination, QString currency)
{
    if(m_current_id==-1)
    {
        return;
    }
    QSqlQuery query(sdb);
    if(!query.exec(QString("INSERT INTO deposit VALUES (%1, %2, %3, '%4');").arg(m_current_id).arg(m_deposit_id).arg(denomination).arg(currency)))
    {
        signalLog("BD insert_denomination ERROR:"+query.lastError().text());
        return;
    }
}
QList<QPair<QString,Bills> > BD::get_list_denaminations_in_cassete(int currency)
{
    QList<QPair<QString,Bills> > lpsb;
    if((m_current_id==-1)&&(currency==-1))
    {
        return lpsb;
    }
    int cur;
    if(currency==-1)
    {
        cur=m_current_id;
    }
    else
    {
        cur=currency;
    }

    QPair<QString,Bills> pair;
    QString nv;
    Bills nb;
    nb.clear();
    bool f=false;
    QSqlQuery query(QString("select currency, denomination from deposit where id_collection=%1 ORDER BY currency;").arg(cur), sdb);
    while(query.next())
    {
        if(nv!=query.value(0).toString())
        {
            if(f)
            {
                lpsb.append(QPair<QString,Bills> (nv, nb));
            }
            nv=query.value(0).toString();
            nb.clear();

        }
        switch(query.value(1).toInt())
        {
        case 10:
            nb.count_10++;
            break;
        case 50:
            nb.count_50++;
            break;
        case 100:
            nb.count_100++;
            break;
        case 200:
            nb.count_200++;
            break;
        case 500:
            nb.count_500++;
            break;
        case 1000:
            nb.count_1000++;
            break;
        case 2000:
            nb.count_2000++;
            break;
        case 5000:
            nb.count_5000++;
            break;
        }

        f=true;
    }
    lpsb.append(QPair<QString,Bills> (nv, nb));

    return lpsb;
}
int BD::get_count_bils(int id)
{
    int cur;
    if(id==-1)
    {
        cur=m_current_id;
    }
    else
    {
        cur=id;
    }
    int den=0;
    QSqlQuery query(QString("select count(*) from deposit where id_collection=%1;").arg(cur), sdb);
    if(query.next())
    {
        den=query.value(0).toInt();
    }
    return den;
}

QString BD::get_last_inkas_datetime()
{
    QSqlQuery query("SELECT dt_end FROM collection WHERE dt_end not null ORDER BY id DESC;", sdb);
    if(query.next())
    {
        return query.value(0).toString();
    }
    return "";
}
int BD::get_last_inkas_number()
{
    QSqlQuery query("SELECT id FROM collection WHERE dt_end not null ORDER BY id DESC;", sdb);
    if(query.next())
    {
        return query.value(0).toInt();
    }
    return -1;
}
