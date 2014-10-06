#include "appwindow.h"
#include <QtPlugin>
#include <Windows.h>
#include <qapplication.h>
#include <boost/filesystem.hpp>
#include <QDesktopWidget>

using namespace std;

AppWindow::AppWindow(QWidget *parent) :
    QWidget(parent)
{
    QWidget::setWindowIcon(QIcon("PQIcon.ico"));
    playButton=new QPushButton(tr("Play"));
    updateButton=new QPushButton(tr("Update"));
    integrityCheck = new QPushButton(tr("Check File Integrity"));
    layout=new QVBoxLayout;
    hlayout=new QHBoxLayout;
    progress=new QProgressBar;
    view= new QWebView();
    statusLabel = new QLabel;
    hash = new MD5Batch();
    update = new updateCheck(hash);
    download = new updateFiles(hash);
	tUpdate = new QThread;
    tDownload = new QThread;

    QRect screenSize=QApplication::desktop()->screenGeometry();
    double screenResolution=screenSize.width()/screenSize.height();
    int width,height,minW,minH;

    if(screenResolution==4/3){
        width=(screenSize.width()/4)*.8*4;
        height=(screenSize.height()/3)*.8*3;
        minW=(screenSize.width()/4)*.5*4;
        minH=(screenSize.height()/3)*.5*3;
    }
    else if(screenResolution==16/10){
        width=(screenSize.width()/16)*.8*16;
        height=(screenSize.height()/10)*.8*10;
        minW=(screenSize.width()/16)*.5*16;
        minH=(screenSize.height()/10)*.5*10;
    }
    else if(screenResolution==16/9){
        width=(screenSize.width()/16)*.8*16;
        height=(screenSize.height()/9)*.8*9;
        minW=(screenSize.width()/16)*.5*16;
        minH=(screenSize.height()/9)*.5*9;
    }
    else{
        width=(screenSize.width()/16)*.8*16;
        height=(screenSize.height()/9)*.8*9;
        minW=(screenSize.width()/16)*.5*16;
        minH=(screenSize.height()/9)*.5*9;
    }


    view->load(QUrl("http://www.dragonlensstudios.com"));
    page = view->page();
    frame = page->currentFrame();

    setMaximumSize(width,height);
    setMinimumSize(minW,minH);
    setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);

    setWindowTitle("Phantasy Quest Launcher v" + QString::fromStdString(hash->getLaunchVersionNumber()));
    view->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::MinimumExpanding);
    updateButton->setDisabled(true);
    statusLabel->setText("Checking for update...");

    layout->addWidget(view);
    layout->addWidget(statusLabel);
    hlayout->addWidget(progress);
    hlayout->addWidget(integrityCheck);
    hlayout->addWidget(updateButton);
    hlayout->addWidget(playButton);
    layout->addLayout(hlayout);
    setLayout(layout);

    connect(integrityCheck,SIGNAL(pressed()),this,SLOT(updateLocal()));
    connect(updateButton,SIGNAL(pressed()),this,SLOT(updateLocal()));
    connect(playButton,SIGNAL(pressed()),this,SLOT(launchGame()));
    connect(update,SIGNAL(setUpdateButtonStatus(bool)),this,SLOT(updateButtonEnabled(bool)));
    connect(download,SIGNAL(setLabel(QString)),this,SLOT(setLabel(QString)));
    connect(download,SIGNAL(setProgress(int)),this,SLOT(setProgressBar(int)));
    connect(download,SIGNAL(setProgressMaximum(int)),this,SLOT(setProgressMaximum(int)));
    connect(download,SIGNAL(resetProgressBar()),this,SLOT(resetProgressBar()));
    connect(hash,SIGNAL(setProgress(int)),this,SLOT(setProgressBar(int)));
    connect(hash,SIGNAL(setProgressMaximum(int)),this,SLOT(setProgressMaximum(int)));
    connect(hash,SIGNAL(setLabel(QString)),this,SLOT(setLabel(QString)));
    connect(update,SIGNAL(restartLauncher(QString)),this,SLOT(restartLauncherforUpdate(QString)));
    connect(download,SIGNAL(setPlayButton(bool)),this,SLOT(setPlayButtonEnabled(bool)));
	
	download->moveToThread(tDownload);
	update->moveToThread(tUpdate);

	download->connect(tDownload,SIGNAL(started()),SLOT(run()));
	update->connect(tUpdate,SIGNAL(started()),SLOT(run()));

	tDownload->connect(download,SIGNAL(finished()),SLOT(quit()));
    tUpdate->connect(update,SIGNAL(finished()),SLOT(quit()));

	tUpdate->start();
}

void AppWindow::updateButtonEnabled(bool state)
{
    if(state){
        setLabel(QString("Update Available! New version: " + QString::fromStdString(hash->getServerVersion())));
		updateButton->setEnabled(true);
	}
    else{
        setLabel("Game up to date! Press play to launch!");
        updateButton->setEnabled(false);
    }
}

void AppWindow::updateLocal(){
    updateButton->setDisabled(true);
    playButton->setDisabled(true);
	tDownload->start();
}

void AppWindow::launchGame(){
    PROCESS_INFORMATION pInform;
    STARTUPINFO pStartup;
    memset(&pInform, 0, sizeof(pInform));
    memset(&pStartup, 0, sizeof(pStartup));
    pStartup.cb= sizeof(pStartup);
	string dir = hash->getRootPath();
	dir.append("game.exe");
    wstring tempGameDir = wstring(dir.begin(),dir.end());

    CreateProcess(tempGameDir.c_str(),NULL,NULL,NULL,false,NORMAL_PRIORITY_CLASS,NULL,NULL,&pStartup,&pInform);
	emit QApplication::quit();
}


void AppWindow::setProgressBar(int num){
    progress->setValue(num);
}

void AppWindow::setProgressMaximum(int num){
    progress->setMaximum(num);
}

void AppWindow::resetProgressBar(){
    progress->setValue(0);
}

void AppWindow::setLabel(QString str){
    statusLabel->setText(str);
}

void AppWindow::restartLauncherforUpdate(QString passed){
    PROCESS_INFORMATION pInform;
    STARTUPINFO pStartup;
    memset(&pInform, 0, sizeof(pInform));
    memset(&pStartup, 0, sizeof(pStartup));
    pStartup.cb= sizeof(pStartup);
    string path = passed.toUtf8().constData();
    wstring exe = wstring(path.begin(),path.end());
    CreateProcess(exe.c_str(),NULL,NULL,NULL,false,NORMAL_PRIORITY_CLASS,NULL,NULL,&pStartup,&pInform);
    emit QApplication::quit();
}

void AppWindow::setPlayButtonEnabled(bool b){
    playButton->setEnabled(b);
}
