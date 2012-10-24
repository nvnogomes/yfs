// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include "lock_client.h"
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>



yfs_client::yfs_client(std::string extent_dst, std::string lock_dst) {

    ec = new extent_client(extent_dst);
    lc = new lock_client(lock_dst);
}


int
yfs_client::create(yfs_client::inum parent, const char *name, yfs_client::inum ninum) {

    std::string buf;
    int returnValue = IOERR;

    lc->acquire( parent );

    if( ec->get(parent, buf) == extent_protocol::OK ) {

        std::stringstream sstream;
        sstream << name << " " << ninum << std::endl;

        buf += sstream.str();

        if( ec->put(parent, buf) == extent_protocol::OK
                && ec->put(ninum, "")  == extent_protocol::OK ) {
            returnValue = OK;
        }
    }

    lc->release( parent );

    return returnValue;
}


int
yfs_client::createfile(inum parent, const char *name, inum &finum) {

    finum = rand() | 0x80000000;

    return create(parent, name, finum);
}


int
yfs_client::createdir(inum parent, const char *name, inum &dinum) {

    dinum = rand() & 0x7FFFFFFF;

    return create(parent, name, dinum);
}


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


std::string
yfs_client::filename(inum inum) {

    std::ostringstream ost;
    ost << inum;
    return ost.str();
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


int
yfs_client::getdir(inum inum, dirinfo &din) {

    int returnValue = IOERR;
    extent_protocol::attr a;

    printf("getdir %016llx\n", inum);

    lc->acquire( inum );

    if (ec->getattr(inum, a) == extent_protocol::OK) {
        din.atime = a.atime;
        din.mtime = a.mtime;
        din.ctime = a.ctime;

        returnValue = OK;
    }

    lc->release( inum );

    return returnValue;
}


int
yfs_client::getfile(inum inum, fileinfo &fin) {

    int returnValue = IOERR;
    extent_protocol::attr a;

    printf("getfile %016llx\n", inum);

    lc->acquire( inum );

    if (ec->getattr(inum, a) == extent_protocol::OK) {
        fin.atime = a.atime;
        fin.mtime = a.mtime;
        fin.ctime = a.ctime;
        fin.size = a.size;
        printf("getfile %016llx -> sz %llu\n", inum, fin.size);

        returnValue = OK;
    }

    lc->release( inum );

    return returnValue;
}


yfs_client::inum
yfs_client::ilookup(inum di, std::string name) {

    std::string buf;
    yfs_client::inum i = 0;

    lc->acquire( di );

    if( ec->get(di, buf) == extent_protocol::OK ) {
        i = findInum(buf, name);
    }

    lc->release( di );

    return i;
}


bool
yfs_client::isdir(inum inum) {

    return ! isfile(inum);
}


bool
yfs_client::isfile(inum inum) {

    if(inum & 0x80000000)
        return true;
    return false;
}


yfs_client::inum
yfs_client::n2i(std::string n) {

    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}


int
yfs_client::readdir(inum ino, std::vector<dirent> &files) {

    std::string buf;
    int returnValue = IOERR;

    lc->acquire( ino );

    if( ec->get(ino, buf) == extent_protocol::OK ) {
        files = deserialize( buf );
        returnValue = OK;
    }

    lc->release( ino );

    return returnValue;
}


int
yfs_client::readfile(inum ino, off_t off, size_t size, std::string &buf) {

    std::string fileContents;
    int returnValue = IOERR;

    lc->acquire( ino );

    if( isfile(ino) ) {
        ec->get(ino, fileContents);

        buf = fileContents.substr(off, size);
        returnValue = OK;
    }

    lc->release( ino );

    return returnValue;

}


int
yfs_client::remove(inum parent, std::string name) {

    std::string buf, result;
    inum remInum;
    int returnValue = IOERR;

    lc->acquire( parent );

    if( ec->get(parent, buf) == extent_protocol::OK ) {

        result = removeDirectoryFile(buf, name, remInum);

        if( ec->put(parent, result) == extent_protocol::OK ) {

            if( ec->remove( remInum ) == extent_protocol::OK ) {
                returnValue = OK;
            }
        }
    }

    lc->release( parent );

    return returnValue;
}


std::string
yfs_client::removeDirectoryFile(std::string bf, std::string nodeName, inum &ri) {

    // TODO: OPTIMIZE!!
    // mix deserialize here

    inum iRem;
    std::vector<dirent> vec = deserialize( bf );
    std::vector<yfs_client::dirent>::iterator it;
    std::ostringstream buf("");
    for( it = vec.begin() ; it != vec.end() ; it++ ) {

        if( it->name == nodeName ) {
            iRem = it->inum;
            continue;
        }
        buf << it->name << " " << it->inum << std::endl;
    }
    ri = iRem;
    return buf.str();
}


int
yfs_client::setattr(inum ino, struct stat *attr, int to_set) {

    extent_protocol::attr nodeAttr;
    int returnValue = IOERR;

    lc->acquire( ino );

    if( ec->getattr(ino, nodeAttr) == extent_protocol::OK ) {

        std::string fileContent;
        if( ec->get(ino, fileContent) == extent_protocol::OK ) {

            if( attr->st_size < fileContent.size() ) {
                ec->put(ino, fileContent.substr(0, attr->st_size) );
                returnValue = OK;
            }
//            return OK;
        }
    }

    lc->release( ino );

    return returnValue;
}


int
yfs_client::writefile(inum ino, std::string buf, off_t off, size_t &size) {

    std::string fileContents;
    int returnValue = IOERR;

    lc->acquire( ino );

    if( ec->get(ino, fileContents) == extent_protocol::OK ) {

        fileContents.replace(off, buf.size(), buf);
        size = buf.size();

        if( ec->put(ino, fileContents) == extent_protocol::OK ) {
            returnValue = OK;
        }
    }

    lc->release( ino );

    return returnValue;
}
