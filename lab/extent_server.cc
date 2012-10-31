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
extent_server::get(extent_protocol::extentid_t id, std::string &buf) {

    std::cout << "GET" << std::endl
              << " id: " << id << " ";

    if( fs.count(id) == 0 ) {
        std::cout << "not found"  << std::endl;
        return extent_protocol::NOENT;
    }
    else {
        buf = fs[id].first;

        fs[id].second.atime = time(NULL);

        std::cout << "Size: " << buf.size() << std::endl;
        return extent_protocol::OK;
    }

}


int
extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a) {

    std::cout << "GETATTR  id=" << id << std::endl;

    if( fs.count(id) == 0 ) {
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
extent_server::put(extent_protocol::extentid_t id, std::string buf, int &) {

    std::cout << "PUT" << std::endl
              << "  id= " << id << "; " << std::endl
              << "  str= "<< buf << std::endl;

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
extent_server::remove(extent_protocol::extentid_t id, int &) {

    std::cout << "REMOVE id: " << id << std::endl;

    if( fs.find(id) == fs.end() ) {

        printMap();

        return extent_protocol::NOENT;
    }
    else {
        fs.erase( id );
        return extent_protocol::OK;
    }
}


int
extent_server::setattr(extent_protocol::extentid_t id, extent_protocol::attr a, int &foo) {

    std::cout << "SETATTR id: " << id << " " << a.size << std::endl;

//    fs[id].second = a;
    std::string buf;
    int difference = a.size - fs[id].second.size;
    if( difference <= 0 ) {
        buf = fs[id].first.substr(0, a.size);
    }
    else {
        buf.append( difference , (char) 0 );
    }

    if( put( id, buf, foo) != extent_protocol::OK ) {
        return extent_protocol::IOERR;
    }

    return extent_protocol::OK;
}


int
extent_server::printMap() {
    std::map<extent_protocol::extentid_t,
            std::pair<std::string, extent_protocol::attr> >::iterator it;

    std::cout << "FS size: " << fs.size() <<std::endl;

    for( it = fs.begin() ; it != fs.end() ; it++ ) {
        std::cout << "entry: " << std::endl
                  << " ino: " << it->first
                  << " buf: " << it->second.first
                  << std::endl;
    }
    return 0;
}

