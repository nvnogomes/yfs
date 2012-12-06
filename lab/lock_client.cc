// RPC stubs for clients to talk to lock_server

#include "lock_client.h"
#include "lock_server.h"
#include "rsm_client.h"
#include "rpc.h"
#include <arpa/inet.h>
#include <sstream>
#include <iostream>
#include <stdio.h>




lock_client::lock_client(std::string dst)
{
    cl = new rsm_client(dst);

    printf("lock_client: call bind\n");
}

int
lock_client::stat(lock_protocol::lockid_t lid)
{
    int r;
    int ret = cl->call(lock_protocol::stat, lid, r);
    assert (ret == lock_protocol::OK);
    return r;
}


/*
 * lock_client::acquire must not return until it has acquired the requested lock.
 */
int
lock_client::acquire(lock_protocol::lockid_t lid)
{
    int r;

    std::cout << "ACQUIRE " << lid << std::endl;

    return cl->call(lock_protocol::acquire, lid, r);
}

int
lock_client::release(lock_protocol::lockid_t lid)
{
    int r;

    std::cout << "RELEASE " << lid << std::endl;

    return cl->call(lock_protocol::release, lid, r);
}

