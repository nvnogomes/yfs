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
    int create(inum parent, const char *name, inum ninum);
    std::vector<dirent> deserialize( std::string s );
    static std::string filename(inum);
    int findInum(std::string bf, std::string lname);
    static inum n2i(std::string);
    std::string removeDirectoryFile(std::string bf, std::string nodeName, inum &ri);

public:

    yfs_client(std::string, std::string);

    inum ilookup(inum di, std::string name);
    int remove(inum parent, std::string name);
    int setattr(inum ino, struct stat *attr, int to_set);

    int createfile(inum parent, const char *name, inum &finum);
    int getfile(inum, fileinfo &);
    bool isfile(inum);
    int readfile(inum, off_t off, size_t size, std::string &buf);
    int writefile(inum ino, std::string buf, off_t off, size_t &size);

    int createdir(inum parent, const char *name, inum &dinum);
    int getdir(inum, dirinfo &);
    bool isdir(inum);
    int readdir(inum ino, std::vector<dirent> &files);

};

#endif
