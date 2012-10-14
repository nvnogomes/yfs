// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>


std::vector<yfs_client::dirent>
yfs_client::deserialize( std::string s ) {

    std::string name;
    yfs_client::inum i;
    std::vector<yfs_client::dirent> dsv;
    std::istringstream sstream (s);
    for(;;) {
        sstream >> name >> i;
        yfs_client::dirent d = {name, i};
        if( sstream.eof() ) {
            break;
        }
        dsv.push_back( d );
    }
    return dsv;
}

int
yfs_client::findInum(std::string bf, std::string lname) {

    std::string name;
    yfs_client::inum i;
    std::stringstream sstream (bf);
    for(;;) {
        sstream >> name >> i;
        if( sstream.eof() ) {
            return 0;
        }
        if( name == lname ) {
            return i;
        }
    }
}

std::string
yfs_client::removeDirectoryFile(std::string bf, std::string lname) {

    std::string name;
    std::stringstream output ("");
    yfs_client::inum i;
    std::istringstream sstream (bf);
    for(;;) {
        sstream >> name >> i;

        if( sstream.eof() )  break;
        if( lname == name) continue;

        output << name << i << std::endl;
    }

    return output.str();
}



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
yfs_client::readfile(inum ino, std::string &buf) {

    if( isfile(ino) ) {
        ec->get(ino, buf);
        return OK;
    }
    else {
        return IOERR;
    }
}

int
yfs_client::readdir(inum ino, std::vector<dirent> &files) {

    std::string buf;
    if( ec->get(ino, buf) == extent_protocol::OK ) {
        files = deserialize( buf );
        return yfs_client::OK;
    }
    else {
        return IOERR;
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

    std::string buf;
    yfs_client::inum i = -1;
    if( ec->get(di, buf) == extent_protocol::OK ) {
        i = findInum(buf, name);
    }
    return i;
}



int
yfs_client::create(yfs_client::inum parent, const char *name, yfs_client::inum ninum) {


    std::string buf;
    if( ec->get(parent, buf) == extent_protocol::OK ) {

        std::stringstream sstream;
        sstream << name << " " << ninum << std::endl;

        buf += sstream.str();

        if( ec->put(parent, buf) == extent_protocol::OK
                && ec->put(ninum, "")  == extent_protocol::OK ) {
            return yfs_client::OK;
        }
    }
    return yfs_client::IOERR;
}


int
yfs_client::createfile(inum parent, const char *name, inum &finum) {

    std::cout << "CREATE yfs_client" << std::endl;

    finum = rand() | 0x80000000;

    return create(parent, name, finum);
}


int
yfs_client::createdir(inum parent, const char *name, inum &dinum) {

    dinum = rand() & 0x7FFFFFFF;

    return create(parent, name, dinum);
}


int
yfs_client::remove(inum parent, std::string name) {

    std::string buf, result;
    inum foundInum = ilookup(parent, name);

    if( foundInum > 0 ) {
        // remove entry
        if( ec->remove( foundInum ) == extent_protocol::OK ) {

            // remove entry in the parent directory
            if( ec->get(parent, buf)  == extent_protocol::OK ) {
                result = removeDirectoryFile(buf, name);

                // update directory contents
                if( ec->put(parent, result) == extent_protocol::OK ) {
                    return OK;
                }
            }
        }
    }
    return IOERR;
}

