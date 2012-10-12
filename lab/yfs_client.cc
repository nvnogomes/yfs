// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>



yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
    ec = new extent_client(extent_dst);
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
yfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
    if(inum & 0x80000000)
        return true;
    return false;
}

bool
yfs_client::isdir(inum inum)
{
    return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:

    return r;
}


int
yfs_client::readFile(inum ino, std::string &buf) {

    if( isfile(ino) ) {
        ec->get(ino, buf);
        return OK;
    }
    return IOERR;
}

int
yfs_client::readDir(inum ino, std::vector<yfs_client::dirent> &files) {

    std::string buf;
    if( ec->get(ino, buf) == extent_protocol::OK ) {
        files = fileSystem[ino];
        return yfs_client::OK;
    }
    else {
        return yfs_client::IOERR;
    }

}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    return r;
}

yfs_client::inum
yfs_client::ilookup(inum di, std::string name) {

    std::vector< yfs_client::dirent >::iterator it;
    for( it = fileSystem[di].begin() ; it != fileSystem[di].end() ; it++ ) {
        if( it->name == name ) {
            return it->inum;
        }
    }

    return -1;
}




int
yfs_client::create(inum parent, const char *name, mode_t mode, bool isdir,
                   inum &ninum) {

    std::cout << "CREATE " << parent << " " << std::string (name) << std::endl;

    std::string nameStr (name);
    inum inumByName = yfs_client::n2i( nameStr );
//    inum newEntryInum =  isdir ? inumByName | 0x00000001 : inumByName & 0x11111110;
    inum newEntryInum =  isdir ? inumByName | 0x80000000 : inumByName & 0x11111110;

    if( ec->put(newEntryInum, "") == extent_protocol::OK ) {

        yfs_client::dirent entryStruct;
        entryStruct.inum = newEntryInum;
        entryStruct.name = name;

        if( isdir ) {
            fileSystem[newEntryInum] = std::vector<yfs_client::dirent>();
        }

        fileSystem[parent].push_back( entryStruct );
        ninum = newEntryInum;
        return OK;
    }
    else {
        return IOERR;
    }
}


int
yfs_client::remove(inum di) {

    return NOENT;
}
