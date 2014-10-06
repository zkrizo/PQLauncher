#include "appwindow.h"
#include <QApplication>
#include <QtConcurrent/QtConcurrent>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AppWindow w;
    w.show();
    return a.exec();
}
