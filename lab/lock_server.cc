// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <map>
#include <utility>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

using namespace std;


/*
 * this is the main structure to control the status
 * of the blocks. this also can control the owner of
 * the lock to avoid forced unlocks by other clients
 *
 * boolean: TRUE -> LOCKED
 */
std::map<lock_protocol::lockid_t,
	std::pair<bool,int> > blockMap;

pthread_mutex_t mutex;
pthread_cond_t freetogo;


lock_server::lock_server(class rsm *_rsm):
	rsm(_rsm), nacquire (0)
{
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init (&freetogo, NULL);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  printf("stat server client: %d\n", clt);
  r = nacquire;
  return lock_protocol::OK;
}

/*
 * ACQUIRE
 */
lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r) {

	pthread_mutex_lock(&mutex);
	while( blockMap[lid].first == true ) {
		pthread_cond_wait(&freetogo, &mutex);
	}

	blockMap[lid].first = true;
	blockMap[lid].second = clt;
	pthread_mutex_unlock(&mutex);

	r = nacquire;
	return lock_protocol::OK;
}


/*
 * RELEASE
 */
lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r) {

	pthread_mutex_lock(&mutex);
	if(blockMap[lid].first == true && blockMap[lid].second == clt) {
		blockMap[lid] = std::make_pair(false, -1);
		pthread_cond_signal(&freetogo);
	}
	pthread_mutex_unlock(&mutex);
	r = nacquire;
	return lock_protocol::OK;
}
