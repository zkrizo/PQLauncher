#ifndef UPDATEFILES_H
#define UPDATEFILES_H

#include <QThread>
#include "md5batch.h"

class updateFiles : public QObject
{
    Q_OBJECT
public slots:
    void run();
signals:
    void setProgressMaximum(int);
    void setProgress(int);
    void setLabel(QString);
    void resetProgressBar();
    void setPlayButton(bool);
public:
    updateFiles();
    updateFiles(MD5Batch*);
private:
    MD5Batch* hash;
};

#endif // UPDATEFILES_H
