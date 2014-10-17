#ifndef MD5BATCH_H
#define MD5BATCH_H

#include <vector>
#include <QObject>
#include <string>

class MD5Batch: public QObject
{
    Q_OBJECT
public:
    explicit MD5Batch(QObject *parent = 0);

    void hashFile(std::string filename, std::string filepath);
    void setComparePath(std::string path);
    void setVersionNumber(std::string versionNumber);
    void discoverFolderRoot();
    void parseServerFiles();
    void grabChanges();
    void launcherCopy();
    int launcherCheck();
    bool grabCurrentVersion();
    int updateCheck();
    bool launcherUpdate();
    std::string getRootPath(){return rootPath;}
    std::string appdataRoot;
    std::string getLaunchVersionNumber(){return launchVersion;}
    std::string getServerVersion(){return serverVersion;}
signals:
    void setProgressMaximum(int);
    void setProgress(int);
    void setLabel(QString);
private:
    //All of the files in the current directory
    std::vector<std::string> files;
    std::vector<std::string> md5hashes;

    //All of the files on the server
    std::vector<std::string> updateFiles;
    std::vector<std::string> updateMd5hashes;

    //Differences between server and local folder
    std::vector<std::string> fileDiff;
    std::vector<std::string> hashDiff;
    std::vector<int> statusDiff;

    //Boolean vector to identify which of the server files need to be downloaded. Index is into update vectors!
    std::vector<bool> downloadFile;

    std::string rootPath;
    std::string versionFileName;
    std::string comparePath;
    std::string versionNumber;
    std::string currentVersion;
    std::string serverVersion;
    std::string serverRoot;
    std::string launcherRoot;
    std::string launchVersion;
    std::string serverLaunchVersion;
    void discoverCurrentVersion();
    void deleteAppDataFiles();
    void createAppDataFiles();
};

#endif // MD5BATCH_H
