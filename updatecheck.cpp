#include "updatecheck.h"
#include <qmessagebox.h>

updateCheck::updateCheck(MD5Batch *hashptr){
    if(hashptr!=NULL){
        hash=hashptr;
    }
}

void updateCheck::run(){
    if(hash==NULL){
        return;
    }
    int launcherUpdate = hash->launcherCheck();
    int result;
    std::string appdata;
        QMessageBox msg;

    switch(launcherUpdate){
    case 0:
        hash->discoverFolderRoot();
        result=hash->updateCheck();
        if(hash->getCurrentVersion()=="-1"){
            emit setUpdateButtonLabel("Download");
            emit setUpdateButtonStatus(true);
        }
        else{
            emit setUpdateButtonStatus(result);
        }
		break;
    case 1:
        hash->launcherUpdate();
        appdata=hash->appdataRoot;
        appdata.append("PQLauncher.exe");
        emit restartLauncher(QString::fromStdString(appdata));
		break;
    case 2:
        emit setUpdateButtonStatus(false);
        hash->launcherCopy();
        std::string install(hash->getRootPath());
        install.append("PQLauncher.exe");
        emit restartLauncher(QString::fromStdString(install));
		break;
    }
}

