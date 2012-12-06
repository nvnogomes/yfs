// RPC stubs for clients to talk to lock_server

#include "lock_client.h"
#include "lock_server.h"
#include "rsm_client.h"
#include "rpc.h"
#include <arpa/inet.h>
#include <sstream>
#include <iostream>
#include <stdio.h>




/** lab6
 * @brief lock_client::lock_client
 *  Modify lock_client to create a rsm_client object in its constructor.
 * @param dst - lock server name
 */
lock_client::lock_client(std::string dst)
{

    cl = new rsm_client(dst);

    printf("lock_client: call bind with rsm_client\n");
}

/**
 * @brief lock_client::stat status of the given block
 * @param lid - inum of the block
 * @return int block status
 */
int
lock_client::stat(lock_protocol::lockid_t lid)
{
    int r;
    int ret = cl->call(lock_protocol::stat, lid, r);
    assert (ret == lock_protocol::OK);
    return r;
}


/**
 * @brief lock_client::acquire lock_client::acquire must
 * not return until it has acquired the requested lock.
 *
 * @param lid block inum
 * @return OK when the block acquired is obtained
 */
int
lock_client::acquire(lock_protocol::lockid_t lid)
{
    int r;
    std::cout << "ACQUIRE " << lid << std::endl;


    /* if the block is already in use, the client will receive
     * a RETRY status, uppon this the client must sleep 50ms
     *
     * the sleep duration is increased each time the acquire fails to succeed
     */
    for(int counter = 1; true ; counter++) {
        lock_protocol::status status = cl->call(lock_protocol::acquire, lid, r);
        if( status == lock_protocol::RETRY ) {
            counter++;
            sleep(50*counter);
        }
        else {
            if( status == lock_protocol::OK ) {
                break;
            }
        }
    }
    return r;
}

/**
 * @brief lock_client::release releases the block with the given inum
 *
 * @param lid block inum to be released
 * @return OK when block released
 */
int
lock_client::release(lock_protocol::lockid_t lid)
{
    int r;

    std::cout << "RELEASE " << lid << std::endl;

    return cl->call(lock_protocol::release, lid, r);
}

