/** the lock server implementation
 *
 * for lab6 all the mutex were removed as indicated in the step 0
 *
 *
 */

#include "lock_server.h"
#include "rsm_state_transfer.h"
#include <sstream>
#include <map>
#include <utility>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

using namespace std;



/**
 * @brief blockMap
 * first: block inum
 * second: status, list of clients waiting
 */
std::map<lock_protocol::lockid_t,
std::pair<bool,int> > blockMap;

pthread_mutex_t mutex;
pthread_cond_t freetogo;


/** lab6
 * @brief lock_server::lock_server default constructor
 *
 * @param _rsm replication state machine
 */
lock_server::lock_server(class rsm *_rsm):
    rsm(_rsm), nacquire (0)
{
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init (&freetogo, NULL);
    rsm->set_state_transfer(this);
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


/** lab6
 * @brief lock_server::acquire acquires block
 * only the primary replies to the client
 *
 * @param clt - client id requesting the block acquire
 * @param lid - block inum to acquire
 * @param r
 * @return
 */
lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r) {

    if( rsm->amiprimary() ) {

        if( blockMap[lid].first == true ) {
            return lock_protocol::RETRY;
        }

        blockMap[lid].first = true;
        blockMap[lid].second = clt;

        nacquire++;
        r = nacquire;
    }
    return lock_protocol::OK;
}


/** lab6
 * @brief lock_server::release
 * only the primary replies to the client
 *
 * @param clt - client id requesting the block release
 * @param lid - block inum to release
 * @param r
 * @return OK if the block is releasesed
 */
lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r) {

    if( rsm->amiprimary() ) {
        if(blockMap[lid].first == true && blockMap[lid].second == clt) {
            blockMap[lid] = std::make_pair(false, -1);
//            pthread_cond_signal(&freetogo);
        }

        nacquire--;
        r = nacquire;
    }
    return lock_protocol::OK;
}


/** lab6
 * @brief lock_server::marshal_state
 * @return
 */
std::string
lock_server::marshal_state() {

  pthread_mutex_lock(&mutex);
//  marshall rep;
//  rep << blockMap.size();
//  std::map<lock_protocol::lockid_t,
//  std::pair<bool,int> >::iterator iterLock;
//  for (iterLock = blockMap.begin(); iterLock != blockMap.end(); iterLock++) {
//    lock_protocol::lockid_t inum = iterLock->first;
//    std::pair<bool,int> vec = iterLock->second;
//    rep << inum;
//    rep << vec;
//  }
  pthread_mutex_unlock(&mutex);
//  return rep.str();

}

/** lab6
 * @brief lock_server::unmarshal_state
 * @param state
 */
void
lock_server::unmarshal_state(std::string state) {

  pthread_mutex_lock(&mutex);
//  unmarshall rep(state);
//  rep >> nacquire;
//  for (unsigned int i = 0; i < blockMap.size(); i++) {
//    std::string name;
//    rep >> name;
//    std::vector vec;
//    rep >> vec;
//    locks[name] = vec;
//  }
  pthread_mutex_unlock(&mutex);
}
