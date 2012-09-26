// RPC stubs for clients to talk to lock_server

#include "lock_client.h"
#include "lock_server.h"
#include "rpc.h"
#include <arpa/inet.h>

#include <sstream>
#include <iostream>
#include <stdio.h>


lock_client::lock_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() < 0) {
    printf("lock_client: call bind\n");
  }
}

int
lock_client::stat(lock_protocol::lockid_t lid)
{
  int r;
  int ret = cl->call(lock_protocol::stat, cl->id(), lid, r);
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
	while(1) {
		int ret = cl->call(lock_protocol::acquire, cl->id(), lid, r);

		if( ret == lock_protocol::OK ) {
			break;
		}
		sleep(2000);
	}

	return r;
}

int
lock_client::release(lock_protocol::lockid_t lid)
{

	return cl->call(lock_protocol::release, cl->id(), lid, r);
}

