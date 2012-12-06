#include "rpc.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include "lock_server.h"
#include "paxos.h"
#include "rsm.h"

#include "jsl_log.h"

// Main loop of lock_server

int
main(int argc, char *argv[])
{
    int count = 0;

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    srandom(getpid());

    if(argc != 3){
        fprintf(stderr, "Usage: %s [master:]port [me:]port\n", argv[0]);
        exit(1);
    }

    char *count_env = getenv("RPC_COUNT");
    if(count_env != NULL){
        count = atoi(count_env);
    }
    // clean warnings...
    printf("RPC COUNT: %i", count);

    // lab6: done
    rsm  server(argv[1], argv[2]);
    lock_server ls(&server);

    server.reg(lock_protocol::stat, &ls, &lock_server::stat);
    server.reg(lock_protocol::acquire, &ls, &lock_server::acquire);
    server.reg(lock_protocol::release, &ls, &lock_server::release);


    while(1)
        sleep(1000);
}
