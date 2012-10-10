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
#include <map>
#include <vector>


yfs_client::yfs_client(std::string extent_dst, std::string lock_dst):
    lastInum(0)
{
    ec = new extent_client(extent_dst);
    fileSystem ();
    fileSystem.insert( 0, std::vector() );
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
    std::string buf;
    if( ec->get(di, buf) == extent_protocol::IOERR ) {
        return IOERR;
    }
    else {
        return di;
    }
}


int
yfs_client::create(inum parent, const char *name, mode_t mode,
                       fuse_entry_param e, bool isdir) {

    // generate inum
    // root directory -> 0x000000001

    inum inumByName = yfs_client::n2i( name );
    inum newEntryInum =  isdir ? inumByName | 0x01 : inumByName;

    if( ec->put(newEntryInum, "") == extent_protocol::OK ) {

        yfs_client::dirent entryStruct;
        entryStruct.inum = newEntryInum;
        entryStruct.name = name;

        fileSystem[parent].push_back( entryStruct );

        if( isdir ) {
            std::vector<yfs_client::dirent> files();
            fileSystem.insert( newEntryInum, files );
        }

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
