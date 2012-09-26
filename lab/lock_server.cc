// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <map>
#include <utility>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>




/*
 * this is the main structure to control the status
 * of the blocks. this also can control the owner of
 * the lock to avoid forced unlocks by other clients
 */
std::map<lock_protocol::lockid_t,
	std::pair<blockStatus,int> > blockMap;


lock_server::lock_server():
  nacquire (0)
{
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat server client: %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r) {

	lock_protocol::status toReturn;
	if( blockMap[lid].first == blockStatus::FREE ) {
		blockMap[lid].first = blockStatus::LOCKED;
		blockMap[lid].second = clt;
		toReturn = lock_protocol::OK;
	}
	else {
		toReturn = lock_protocol::RETRY;
	}

	r = nacquire;
	return toReturn;
}


/*
 * if the block is locked then its unlocked.
 * this will always return OK because unlock
 * a block that already is unlock should not
 * be considered as an error (?)
 *
 * @return lock_protocol::OK
 */
lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r) {

	if( blockMap[lid].first == blockStatus::LOCKED ) {
		if( blockMap[lid].second == clt ) {
			blockMap[lid] = std::make_pair(blockStatus::FREE, -1);
		}
	}
	r = nacquire;
	return lock_protocol::OK;
}

