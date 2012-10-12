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
    put(0x00000001, "", i);
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

    fs[ id ] = std::make_pair(buf, attr);
    return extent_protocol::OK;
}



int
extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{

    if( fs.count(id) == 0 ) {
        return extent_protocol::NOENT;
    }
    else {
        buf = fs[id].first;
        fs[id].second.atime = time(NULL);

        return extent_protocol::OK;
    }

}


int
extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
    if( fs.count(id) == 0 ) {
        a.size = 0;
        a.ctime = 0;
        a.mtime = 0;
        a.atime = 0;

        return extent_protocol::NOENT;
    }
    else {
        fs[id].second.atime = time(NULL);
        extent_protocol::attr currAttr;
        currAttr = fs[id].second;
        a = currAttr;

        return extent_protocol::OK;
    }
}



int
extent_server::remove(extent_protocol::extentid_t id, int &)
{
    //TODO erase from parent of id
    std::map<extent_protocol::extentid_t,
            std::pair<std::string, extent_protocol::attr> >::iterator entry = fs.find(id);

    if( fs.count(id) == 0 ) {
        return extent_protocol::NOENT;
    }
    else {
        fs.erase( entry );
        return extent_protocol::OK;
    }
}


