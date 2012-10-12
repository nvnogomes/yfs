// the extent server implementation

#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <map>
#include <time.h>
#include "extent_server.h"



extent_server::extent_server()
{
    int i;
    put(1, "", i);
}


int
extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
    extent_protocol::attr attr;
    int current = (int) time(NULL);
    attr.size = buf.size();
    attr.atime = current;
    attr.mtime = current;
    attr.ctime = current;

    fs[ id ] = std::pair<std::string, extent_protocol::attr>(buf, attr);
    return extent_protocol::OK;
}



int
extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
    if( fs.find(id) == fs.end() ) {
        return extent_protocol::IOERR;
    }
    else {
        buf = fs[ id ].first;
        fs[id].second.atime = (int) time(NULL);
    }
    return extent_protocol::OK;

}


int
extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{

    if( fs.find(id) == fs.end() ) {
        return extent_protocol::IOERR;
    }
    else {
        a = fs[ id ].second;
    }

    return extent_protocol::OK;
}



int
extent_server::remove(extent_protocol::extentid_t id, int &)
{
    //TODO erase from parent of id
    std::map<extent_protocol::extentid_t,
            std::pair<std::string, extent_protocol::attr> >::iterator entry = fs.find(id);

    if( entry == fs.end() ) {
        return extent_protocol::IOERR;
    }
    else {
        fs.erase( entry );
        return extent_protocol::OK;
    }
}


