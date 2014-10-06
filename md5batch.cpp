#define URDL_HEADER_ONLY 1
#define URDL_DISABLE_SSL 1
#define NTDDI_VERSION NTDDI_WIN7

#include "md5batch.h"
#include <QMessageBox>
#include <fstream>
#include <urdl/istream.hpp>
#include <boost/filesystem.hpp>
#include <boost\algorithm\string\replace.hpp>
#include <Windows.h>
#include "md5.h"
#include <ShlObj.h>
#include <ObjBase.h>
#include <stdio.h>
#include <errno.h>

using namespace std;

enum FILE_STATUS{
    FILE_NOCHANGE=0,
    FILE_ADDED,
    FILE_MODIFIED,
    FILE_DELETED
};

MD5Batch::MD5Batch(QObject *parent): QObject(parent)
{
    serverRoot="http://phantasyquest.dragonlensstudios.com/Storage/PhantasyQuest/Latest/";
    launcherRoot = "http://phantasyquest.dragonlensstudios.com/Storage/PhantasyQuest/Launcher/";
    launchVersion = "1.002";
}

//hashFile() opens and hashes the file sent. Note: Filepath is in GLOBAL coordinates, not LOCAL!
void MD5Batch::hashFile(std::string filename, std::string filepath){
    ifstream file;

	file.open(filepath,ios_base::binary);

    if(!file){
        return;
    }
	
    MD5 hash;
    char buf[64];

    while(file)
    {
        file.read(buf,64);

        if(file){
            hash.update(buf,64);
        }
    }

    file.close();

    hash.finalize();

    string result = hash.hexdigest();

    md5hashes.push_back(result);
    files.push_back(filename);

}


//setComparePath() sets the comparison file path if applicable that stores the last versions MD5s for comparison with this
//new version.
void MD5Batch::setComparePath(std::string path){
    comparePath = path;
}

//setVersionNumber() stores the version number for the output files containing the MD5 hashes after the program has finished processing
void MD5Batch::setVersionNumber(std::string version){
    versionNumber = version;
}

//discoverFolderRoot() retrieves the global path for the launcher exe
void MD5Batch::discoverFolderRoot(){
    boost::filesystem::path path = boost::filesystem::initial_path();

    if(!path.empty()){
        rootPath = path.string();
        rootPath.append("/");
    }
    else{
        QMessageBox msg;
        msg.setInformativeText("Could not get file path for exe");
        msg.exec();
    }

    discoverCurrentVersion();
}

void MD5Batch::discoverCurrentVersion(){
    ifstream readFile;

    string version;
    string filename = rootPath;
    filename.append("currentVersion.dat");

    readFile.open(filename,ios::binary);

    if(readFile){
        if(getline(readFile,version)){
            currentVersion=version;
        }
    }

    readFile.close();
}

bool MD5Batch::grabCurrentVersion()
{
    string filename=serverRoot;
    filename.append("currentVersion.dat");
    urdl::istream is(filename);
    string version;

    if(!is){
        return false;
    }

    if(getline(is,version)){
        serverVersion=version;
		is.close();
        return true;
    }
    else{
		is.close();
        return false;
    }

}

bool MD5Batch::updateCheck(){
    if(!grabCurrentVersion()){
        return false;
    }

    if(serverVersion!=currentVersion){
        return true;
    }
    return false;
}


void MD5Batch::parseServerFiles(){
    string serverFile = serverRoot;
    serverFile.append(serverVersion);
    serverFile.append("%20-%20Full.dat");
    string input;

    urdl::istream is(serverFile);

    if(!is){
        return;
    }

    bool filestate=true;
    while(filestate){
        filestate=getline(is,input);
        if(filestate){
            updateFiles.push_back(input);
        }
        else{
            break;
        }

        filestate=getline(is,input);
        if(filestate){
            updateMd5hashes.push_back(input);
        }

        filestate=getline(is,input);
    }

    bool *compareCheck = new bool[updateFiles.size()];

    for(int n=0;n<updateFiles.size();++n){
        compareCheck[n]=true;
    }

    //iterate through the local files and see if there is a match in the comparison file
    for(int i=0;i<files.size();++i){
        vector<string>::iterator it=find(updateFiles.begin(),updateFiles.end(),files[i]);

        int index=it-updateFiles.begin();
        if(it!=updateFiles.end()){
            if(updateMd5hashes[index]==md5hashes[i]){
                fileDiff.push_back(files[i]);
                hashDiff.push_back(md5hashes[i]);
                statusDiff.push_back(FILE_NOCHANGE);
                compareCheck[index]=false;
            }
            else{
                fileDiff.push_back(files[i]);
                hashDiff.push_back(md5hashes[i]);
                statusDiff.push_back(FILE_MODIFIED);
                compareCheck[index]=false;
            }
        }
        else{
            fileDiff.push_back(files[i]);
            hashDiff.push_back(md5hashes[i]);
            statusDiff.push_back(FILE_DELETED);
        }
    }

    //iterate through the server files to find any files that may have been added between versions
    for(int j=0;j<updateFiles.size();++j){
        if(compareCheck[j]){
            fileDiff.push_back(updateFiles[j]);
            hashDiff.push_back(updateMd5hashes[j]);
            statusDiff.push_back(FILE_ADDED);
        }
    }

    delete [] compareCheck;

    //the Diff vectors now contain ALL files that need to be downloaded or deleted from local depending
    //on their statusDiff flag
}

void MD5Batch::grabChanges(){
    emit setProgressMaximum(fileDiff.size());

    wchar_t* localAppData =0;
    SHGetKnownFolderPath(FOLDERID_LocalAppData,0,NULL,&localAppData);

    wstringstream ss;
    ss<<localAppData<<L"\\PhantasyQuest\\";

	//Ensure directories exist, temp included
	boost::filesystem::path dir(wstring(ss.str()));
	if(!boost::filesystem::exists(dir)){
		boost::filesystem::create_directory(dir);
    }

    for(int i=0;i<fileDiff.size();++i){
		if(i%20==0)
			emit setProgress(i);

        if(statusDiff[i]==FILE_DELETED){
            //Delete the local file
            string localpath=rootPath;
            localpath.append(fileDiff[i]);
            if(boost::filesystem::exists(boost::filesystem::path(localpath))){
                //boost::filesystem::remove(boost::filesystem::path(localpath));
            }
        }
        else{
            if(statusDiff[i]!=FILE_NOCHANGE){
                //Download and replace the local file.
                string localpath=rootPath;
                string serverpath=serverRoot;
                localpath.append(fileDiff[i]);
                serverpath.append(fileDiff[i]);
                wstring tmp(ss.str());

                QString tempFile(QString::fromStdWString(tmp));

				//Extract the filename itself from fileDiff
				string file = fileDiff[i];
				if(file.find('\\')!=string::npos){
					file.erase(file.begin(),file.begin()+file.find_last_of('\\')+1);
				}

                tempFile.append(QString::fromStdString(file));

                ofstream outFile;
                urdl::istream inFile;

                outFile.open(tempFile.toUtf8().constData(),ios_base::binary);

				//If folder doesn't exist, run through Boost's filesystem to create the directory and try reopening file
				if(errno==ENOENT)
				{
					string directory(tempFile.toUtf8().constData());
					directory.erase(directory.begin()+directory.find_last_of('\\'),directory.end());
					boost::filesystem::path buildDir(directory);
					boost::filesystem::create_directories(buildDir);

                    QString fileErr = "File to open:\n";
                    fileErr.append(tempFile.toUtf8().constData());

                    outFile.clear();
                    outFile.close();
                    outFile.open(tempFile.toUtf8().constData(),ios_base::binary);
				}

                if(outFile.good()){
					//Ensure that path is correct for urls
					boost::replace_all(serverpath,"\\","/");
					boost::replace_all(serverpath," ","%20");
                    inFile.open(serverpath);

					if(inFile.good()){
						string endline;
						string extension = tempFile.toUtf8().constData();
						extension.erase(extension.begin(),extension.begin()+extension.find_last_of('.'));

						if(extension==".txt"){
							endline="\r\n";
						}else{
							endline='\n';
						}
						
						string buffer;
						while(getline(inFile,buffer)){
							if(inFile.eof())
								outFile<< buffer;
							else
								outFile<< buffer<< endline;

							outFile.flush();
						}

                        outFile.clear();
                        outFile.close();
                        inFile.clear();
                        inFile.close();

						string directory(localpath);
						directory.erase(directory.begin()+directory.find_last_of('\\'),directory.end());
						boost::filesystem::path buildDir(directory);
						if(!boost::filesystem::exists(buildDir)){
							boost::filesystem::create_directories(buildDir);
						}

                        boost::filesystem::copy_file(boost::filesystem::path(tempFile.toUtf8().constData()),boost::filesystem::path(localpath),boost::filesystem::copy_option::overwrite_if_exists);
						boost::filesystem::remove(tempFile.toUtf8().constData());
                    }
                }
				else{
                    QString fileErr = "File to open:\n";
                    fileErr.append(tempFile.toUtf8().constData());
				}
            }
        }
    }

    ofstream outFile;
    string filename = rootPath;
    filename.append("currentVersion.dat");
    outFile.open(filename,ios_base::binary);

    if(outFile.good()){
        outFile << serverVersion << '\n';
    }

    outFile.close();
    outFile.clear();

    CoTaskMemFree(static_cast<void*>(localAppData));
}

//launcherCheck() checks the appdata folder to see if the update files are in place
int MD5Batch::launcherCheck(){
    wchar_t* localAppData =0;
    SHGetKnownFolderPath(FOLDERID_LocalAppData,0,NULL,&localAppData);

    wstringstream ss;
    ss<<localAppData<<L"\\PhantasyQuest\\";
    appdataRoot = boost::filesystem::path(wstring(ss.str())).string();
    ss<<"launcherData.dat";

    //Ensure directories exist, temp included
    boost::filesystem::path dir(wstring(ss.str()));

    if(boost::filesystem::exists(dir)){
        ifstream inFile(dir.string(),ios_base::binary);

        if(inFile.good()){
            string buffer;
            if(getline(inFile,buffer)){
                rootPath=buffer;

                inFile.close();

                CoTaskMemFree(static_cast<void*>(localAppData));
                return 2;
            }
        }
    }
    else{
        string applaunch(appdataRoot);
        applaunch.append("PQLauncher.exe");

        if(boost::filesystem::exists(boost::filesystem::path(applaunch))){
            deleteAppDataFiles();
        }

        string launcherDat=launcherRoot;
        string buffer;
        launcherDat.append("launcherVersion.dat");
        urdl::istream inFile(launcherDat);

        if(inFile.good()){
            if(getline(inFile,buffer)){
                serverLaunchVersion=buffer;
            }
            else{
                inFile.close();

                CoTaskMemFree(static_cast<void*>(localAppData));
                return false;
            }

            inFile.clear();
            inFile.close();

            if(serverLaunchVersion!=launchVersion){

                CoTaskMemFree(static_cast<void*>(localAppData));
                return 1;
            }
        }
    }

    CoTaskMemFree(static_cast<void*>(localAppData));

    return false;
}


//launcherUpdate() downloads the new launcher, and puts important data in place to update the launcher
bool MD5Batch::launcherUpdate(){
    discoverFolderRoot();
    string launcherDat=launcherRoot;
    string buffer;
    launcherDat.append("launcherVersion.dat");
    urdl::istream inFile(launcherDat);

    if(inFile.good()){
        if(getline(inFile,buffer)){
            serverLaunchVersion=buffer;
        }
        else{
            inFile.close();
            return false;
        }

        inFile.clear();
        inFile.close();

        if(serverLaunchVersion!=launchVersion){
            wchar_t* localAppData =0;
            SHGetKnownFolderPath(FOLDERID_LocalAppData,0,NULL,&localAppData);

            wstringstream ss;
            ss<<localAppData<<L"\\PhantasyQuest\\";

            //Ensure directories exist, temp included
            boost::filesystem::path dir(wstring(ss.str()));
            if(!boost::filesystem::exists(dir)){
                boost::filesystem::create_directory(dir);
            }

            ofstream outFile;
            string file=dir.string();
            file.append("launcherData.dat");

            outFile.open(file,ios_base::binary);

            if(outFile.good()){
                outFile << rootPath << endl;
            }

            outFile.clear();
            outFile.close();

            string launcherexe = launcherRoot;
            launcherexe.append("PQLauncher.exe");
            string output = dir.string();
            output.append("PQLauncher.exe");

            inFile.open(launcherexe);

            if(inFile.good()){
                outFile.open(output,ios_base::binary);
                if(outFile.good()){
                    while(getline(inFile,buffer)){
                        outFile << buffer << endl;
                    }

                    createAppDataFiles();
                }
                else{
                    return false;
                }
            }
            else{
                return false;
            }


            CoTaskMemFree(static_cast<void*>(localAppData));
            inFile.close();
            outFile.close();
            return true;
        }
        else{
            return false;
        }
    }
	return false;
}

void MD5Batch::launcherCopy(){
    string from(appdataRoot);
    from.append("PQLauncher.exe");
    string to(rootPath);
    to.append("PQLauncher.exe");
    boost::filesystem::path dat = appdataRoot + "launcherData.dat";

    boost::filesystem::copy_file(boost::filesystem::path(from),boost::filesystem::path(to),boost::filesystem::copy_option::overwrite_if_exists);
    boost::filesystem::remove(dat);
}

void MD5Batch::deleteAppDataFiles(){
    vector<boost::filesystem::path> fileList;
    boost::filesystem::path file;

    file=appdataRoot+"platforms/qwindows.dll";
    fileList.push_back(file);
    file=appdataRoot+"icudt52.dll";
    fileList.push_back(file);
    file=appdataRoot+"icuin52.dll";
    fileList.push_back(file);
    file=appdataRoot+"icuuc52.dll";
    fileList.push_back(file);
    file=appdataRoot+"libEGL.dll";
    fileList.push_back(file);
    file=appdataRoot+"Qt5Core.dll";
    fileList.push_back(file);
    file=appdataRoot+"Qt5Gui.dll";
    fileList.push_back(file);
    file=appdataRoot+"Qt5Multimedia.dll";
    fileList.push_back(file);
    file=appdataRoot+"Qt5MultimediaWidgets.dll";
    fileList.push_back(file);
    file=appdataRoot+"Qt5Network.dll";
    fileList.push_back(file);
    file=appdataRoot+"Qt5OpenGL.dll";
    fileList.push_back(file);
    file=appdataRoot+"Qt5Positioning.dll";
    fileList.push_back(file);
    file=appdataRoot+"Qt5PrintSupport.dll";
    fileList.push_back(file);
    file=appdataRoot+"Qt5Qml.dll";
    fileList.push_back(file);
    file=appdataRoot+"Qt5Quick.dll";
    fileList.push_back(file);
    file=appdataRoot+"Qt5Sensors.dll";
    fileList.push_back(file);
    file=appdataRoot+"Qt5Sql.dll";
    fileList.push_back(file);
    file=appdataRoot+"Qt5WebKit.dll";
    fileList.push_back(file);
    file=appdataRoot+"Qt5WebKitWidgets.dll";
    fileList.push_back(file);
    file=appdataRoot+"Qt5Widgets.dll";
    fileList.push_back(file);
    file=appdataRoot+"PQLauncher.exe";
    fileList.push_back(file);

    for(int i=0;i!=fileList.size();++i){
        try{
            boost::filesystem::remove(fileList[i]);
        }
        catch(boost::filesystem::filesystem_error err){
            QMessageBox msg;
            msg.setText("Filesystem Exception: ");
            msg.setInformativeText(err.what());
            msg.exec();
        }
    }
}

void MD5Batch::createAppDataFiles(){
    vector<boost::filesystem::path> fileList;
    vector<boost::filesystem::path> fromList;

    boost::filesystem::path file;
    boost::filesystem::path fromFile;

    file=appdataRoot+"platforms";
    fromFile=rootPath+"platforms/qwindows.dll";
    boost::filesystem::create_directory(file);
    file=appdataRoot+"platforms/qwindows.dll";
    boost::filesystem::copy_file(fromFile,file,boost::filesystem::copy_option::overwrite_if_exists);

    file=appdataRoot+"icudt52.dll";
    fromFile=rootPath+"icudt52.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"icuin52.dll";
    fromFile=rootPath+"icuin52.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"icuuc52.dll";
    fromFile=rootPath+"icuuc52.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"libEGL.dll";
    fromFile=rootPath+"libEGL.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"Qt5Core.dll";
    fromFile=rootPath+"Qt5Core.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"Qt5Gui.dll";
    fromFile=rootPath+"Qt5Gui.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"Qt5Multimedia.dll";
    fromFile=rootPath+"Qt5Multimedia.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"Qt5MultimediaWidgets.dll";
    fromFile=rootPath+"Qt5MultimediaWidgets.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"Qt5Network.dll";
    fromFile=rootPath+"Qt5Network.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"Qt5OpenGL.dll";
    fromFile=rootPath+"Qt5OpenGL.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"Qt5Positioning.dll";
    fromFile=rootPath+"Qt5Positioning.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"Qt5PrintSupport.dll";
    fromFile=rootPath+"Qt5PrintSupport.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"Qt5Qml.dll";
    fromFile=rootPath+"Qt5Qml.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"Qt5Quick.dll";
    fromFile=rootPath+"Qt5Quick.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"Qt5Sensors.dll";
    fromFile=rootPath+"Qt5Sensors.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"Qt5Sql.dll";
    fromFile=rootPath+"Qt5Sql.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"Qt5WebKit.dll";
    fromFile=rootPath+"Qt5WebKit.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"Qt5WebKitWidgets.dll";
    fromFile=rootPath+"Qt5WebKitWidgets.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);
    file=appdataRoot+"Qt5Widgets.dll";
    fromFile=rootPath+"Qt5Widgets.dll";
    fromList.push_back(fromFile);
    fileList.push_back(file);

    for(int i=0;i!=fileList.size();++i){
        try{
            boost::filesystem::copy_file(fromList[i],fileList[i],boost::filesystem::copy_option::overwrite_if_exists);
        }
        catch(boost::filesystem::filesystem_error err){

            QMessageBox msg;
            msg.setText("Filesystem Exception:");
            msg.setInformativeText(err.what());
            msg.exec();
        }
    }
}