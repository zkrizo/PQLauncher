#ifndef APPWINDOW_H
#define APPWINDOW_H

#include <QWidget>
#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKitWidgets/QWebPage>
#include <QPushButton>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QLabel>
#include "md5batch.h"
#include "updatecheck.h"
#include "updatefiles.h"

class AppWindow : public QWidget
{
    Q_OBJECT
public:
    explicit AppWindow(QWidget *parent = 0);

signals:

public slots:
    void updateLocal();
    void launchGame();
    void updateButtonEnabled(bool);
    void setProgressBar(int);
    void setProgressMaximum(int);
    void resetProgressBar();
    void setLabel(QString);
    void restartLauncherforUpdate(QString);
    void setPlayButtonEnabled(bool);

private:
    QWebView* view;
    QWebPage* page;
    QWebFrame* frame;
    QPushButton* playButton;
    QPushButton* updateButton;
    QPushButton* integrityCheck;
    QVBoxLayout* layout;
    QHBoxLayout* hlayout;
    QProgressBar* progress;
    QLabel* statusLabel;
    MD5Batch* hash;
    updateCheck* update;
    updateFiles* download;
	QThread *tUpdate;
    QThread *tDownload;
    std::string launcherVersion;
};

#endif // APPWINDOW_H
