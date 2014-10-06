#include "updatefiles.h"
#include <vector>
#include <boost/filesystem.hpp>

using namespace std;

updateFiles::updateFiles()
{
}

updateFiles::updateFiles(MD5Batch* hashptr){
    if(hashptr!=NULL){
        hash=hashptr;
    }
}

void updateFiles::run(){
    vector<boost::filesystem::path> fileList;
	vector<boost::filesystem::path> fileStart;
    std::string root=hash->getRootPath();
    boost::filesystem::path p(root);
    int rootLength=root.length();
    string error;

    bool finished=false;
    bool firstCheck=true;
    emit setLabel("Checking Local Files...");

    //Gather all local files for hashing
    if(!boost::filesystem::is_directory(p)){
        error="Root is not directory";
    }
    else{
    copy(boost::filesystem::directory_iterator(p),boost::filesystem::directory_iterator(),back_inserter(fileStart));
    boost::filesystem::path lastChecked=*fileStart.begin();
    while(!finished){
        std::vector<boost::filesystem::path>::iterator it (fileStart.begin());

        if(firstCheck){
            firstCheck=false;
        }else {
            while((*it)!=lastChecked){
                 ++it;
            }
            ++it;
        }

        if(boost::filesystem::is_directory(*it)){
            lastChecked=*it;
            copy(boost::filesystem::directory_iterator(*it),boost::filesystem::directory_iterator(),back_inserter(fileStart));
        }
        else{
            fileList.push_back((*it));

            lastChecked=*it;
            if(it+1==fileStart.end()){
                finished=true;
            }
        }
    }

    emit setProgressMaximum(fileList.size());
    emit resetProgressBar();

    for(int i=0;i<fileList.size();++i){
        //Gather and trim global file path to local path
        string path=fileList[i].string();
        path.erase(path.begin(),path.begin()+rootLength);

        hash->hashFile(path,(fileList[i].string()));

		//emit a signal every 20 files
		if(i%20==0)
			emit setProgress(i);
    }

    emit resetProgressBar();
    emit setLabel("Checking Server Files...");

    hash->parseServerFiles();

    emit resetProgressBar();
    emit setLabel("Downloading Update from Server...");

    hash->grabChanges();

    emit resetProgressBar();
    emit setProgressMaximum(1);
    emit setProgress(1);
    emit setLabel("Download complete! Update has been applied!");

    emit setPlayButton(true);
    //emit finished();
    }


}
