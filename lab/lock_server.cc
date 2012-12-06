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
    if( rsm->amiprimary() ) {
        printf("stat server client: %d\n", clt);
        r = nacquire;
    }
    return lock_protocol::OK;
}

/*
 * ACQUIRE
 */
lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r) {

    if( rsm->amiprimary() ) {
        pthread_mutex_lock(&mutex);

        while( blockMap[lid].first == true ) {
            pthread_cond_wait(&freetogo, &mutex);
        }

        blockMap[lid].first = true;
        blockMap[lid].second = clt;
        pthread_mutex_unlock(&mutex);

        r = nacquire;
    }
    return lock_protocol::OK;
}


/*
 * RELEASE
 */
lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r) {

    if( rsm->amiprimary() ) {
        pthread_mutex_lock(&mutex);
        if(blockMap[lid].first == true && blockMap[lid].second == clt) {
            blockMap[lid] = std::make_pair(false, -1);
            pthread_cond_signal(&freetogo);
        }
        pthread_mutex_unlock(&mutex);
        r = nacquire;
    }
    return lock_protocol::OK;
}

std::string
lock_server::marshal_state() {

  pthread_mutex_lock(&mutex);
  marshall rep;
  rep << locks.size();
  std::map< std::string, std::vector >::iterator iter_lock;
  for (iter_lock = locks.begin(); iter_lock != locks.end(); iter_lock++) {
    std::string name = iter_lock->first;
    std::vector vec = locks[name];
    rep << name;
    rep << vec;
  }
  pthread_mutex_unlock(&mutex);
  return rep.str();

}

void
lock_server::unmarshal_state(std::string state) {

  pthread_mutex_lock(&mutex);
  unmarshall rep(state);
  unsigned int locks_size;
  rep >> locks_size;
  for (unsigned int i = 0; i < locks_size; i++) {
    std::string name;
    rep >> name;
    std::vector vec;
    rep >> vec;
    locks[name] = vec;
  }
  pthread_mutex_unlock(&mutex);
}
