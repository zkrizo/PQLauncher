#ifndef UPDATECHECK_H
#define UPDATECHECK_H

#include <QThread>
#include "md5batch.h"

class updateCheck : public QObject
{
    Q_OBJECT
signals:
    void setUpdateButtonStatus(bool);
    void restartLauncher(QString);
public slots:
    void run();
public:
    updateCheck(){hash=NULL;}
    updateCheck(MD5Batch * hashptr);
private:
    MD5Batch* hash;
};

#endif // UPDATECHECK_H
