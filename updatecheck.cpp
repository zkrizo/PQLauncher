#include "updatecheck.h"

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
    bool result;
    std::string appdata;

    switch(launcherUpdate){
    case 0:
        hash->discoverFolderRoot();
        result=hash->updateCheck();
        emit setUpdateButtonStatus(result);
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

