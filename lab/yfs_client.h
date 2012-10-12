#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>
#include <map>


class yfs_client {
    extent_client *ec;

public:   
    typedef unsigned long long inum;
    enum xxstatus { OK, RPCERR, NOENT, IOERR, FBIG };
    typedef int status;

    struct fileinfo {
        unsigned long long size;
        unsigned long atime;
        unsigned long mtime;
        unsigned long ctime;
    };
    struct dirinfo {
        unsigned long atime;
        unsigned long mtime;
        unsigned long ctime;
    };
    struct dirent {
        std::string name;
        unsigned long long inum;
    };


private:
    static std::string filename(inum);
    static inum n2i(std::string);
     std::map<inum, std::vector<yfs_client::dirent> > fileSystem;

public:

    yfs_client(std::string, std::string);

    inum ilookup(inum di, std::string name);
    int remove(inum finum);

    int createfile(inum parent, const char *name, inum &finum);
    int getfile(inum, fileinfo &);
    bool isfile(inum);
    int readfile(inum, std::string &buf);

    int createdir(inum parent, const char *name, inum &dinum);
    int getdir(inum, dirinfo &);
    bool isdir(inum);
    int readdir(inum ino, std::vector<yfs_client::dirent> &files);

};

#endif 
