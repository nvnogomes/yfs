#include "paxos.h"
#include <iostream>
#include "handle.h"
// #include <signal.h>
#include <stdio.h>

// This module implements the proposer and acceptor of the Paxos
// distributed algorithm as described by Lamport's "Paxos Made
// Simple".  To kick off an instance of Paxos, the caller supplies a
// list of nodes, a proposed value, and invokes the proposer.  If the
// majority of the nodes agree on the proposed value after running
// this instance of Paxos, the acceptor invokes the upcall
// paxos_commit to inform higher layers of the agreed value for this
// instance.


bool
operator> (const prop_t &a, const prop_t &b)
{
    return (a.n > b.n || (a.n == b.n && a.m > b.m));
}

bool
operator>= (const prop_t &a, const prop_t &b)
{
    return (a.n > b.n || (a.n == b.n && a.m >= b.m));
}

std::string
print_members(const std::vector<std::string> &nodes)
{
    std::string s;
    s.clear();
    for (unsigned i = 0; i < nodes.size(); i++) {
        s += nodes[i];
        if (i < (nodes.size()-1))
            s += ",";
    }
    return s;
}

bool isamember(std::string m, const std::vector<std::string> &nodes)
{
    for (unsigned i = 0; i < nodes.size(); i++) {
        if (nodes[i] == m) return 1;
    }
    return 0;
}

bool
proposer::isrunning()
{
    bool r;
    assert(pthread_mutex_lock(&pxs_mutex)==0);
    r = !stable;
    assert(pthread_mutex_unlock(&pxs_mutex)==0);
    return r;
}

// check if the servers in l2 contains a majority of servers in l1
bool
proposer::majority(const std::vector<std::string> &l1,
                   const std::vector<std::string> &l2)
{
    unsigned n = 0;

    for (unsigned i = 0; i < l1.size(); i++) {
        if (isamember(l1[i], l2))
            n++;
    }
    return n >= (l1.size() >> 1) + 1;
}

proposer::proposer(class paxos_change *_cfg, class acceptor *_acceptor,
                   std::string _me)
    : cfg(_cfg), acc (_acceptor), me (_me), break1 (false), break2 (false),
      stable (true)
{
    my_n.m = _me;
    assert (pthread_mutex_init(&pxs_mutex, NULL) == 0);

}

void
proposer::setn()
{
    my_n.n = acc->get_n_h().n + 1 > my_n.n + 1 ? acc->get_n_h().n + 1 : my_n.n + 1;
}

bool
proposer::run(int instance, std::vector<std::string> newnodes, std::string newv)
{
    std::vector<std::string> accepts;
    std::vector<std::string> nodes;
    std::vector<std::string> nodes1;
    std::string v;
    bool r = false;

    pthread_mutex_lock(&pxs_mutex);
    printf("start: initiate paxos for %s w. i=%d v=%s stable=%d\n",
           print_members(newnodes).c_str(), instance, newv.c_str(), stable);
    if (!stable) {  // already running proposer?
        printf("proposer::run: already running\n");
        pthread_mutex_unlock(&pxs_mutex);
        return false;
    }
    stable = false;

    setn();
    accepts.clear();
    nodes.clear();
    v.clear();
    c_nodes = newnodes;
    c_v = newv;
    nodes = c_nodes;
    if (prepare(instance, accepts, nodes, v)) {

        if (majority(c_nodes, accepts)) {
            printf("paxos::manager: received a majority of prepare responses\n");

            if (v.size() == 0) {
                v = c_v;
            }

            breakpoint1();

            nodes1 = accepts;
            accepts.clear();
            accept(instance, accepts, nodes1, v);

            if (majority(c_nodes, accepts)) {
                printf("paxos::manager: received a majority of accept responses\n");

                breakpoint2();

                decide(instance, accepts, v);
                r = true;
            } else {
                printf("paxos::manager: no majority of accept responses\n");
            }
        } else {
            printf("paxos::manager: no majority of prepare responses\n");
        }
    } else {
        printf("paxos::manager: prepare is rejected %d\n", stable);
    }
    stable = true;
    pthread_mutex_unlock(&pxs_mutex);
    return r;
}


/**
 * @brief proposer::prepare
 * @param instance
 * @param accepts
 * @param nodes
 * @param v
 * @return
 */
bool
proposer::prepare(unsigned instance, std::vector<std::string> &accepts,
                  std::vector<std::string> nodes,
                  std::string &v)
{
    // TODO
    //send prepare(instance, n) to all servers including self

    paxos_protocol::preparearg arg;
    arg.instance = instance;
    arg.n = this->my_n;
    arg.v = this->c_v;

    std::vector<std::string>::iterator node;
    for(node = nodes.begin() ; node != nodes.end() ; node++) {
        handle hdl( *node );

        // send prepare(n) to all acceptors
        paxos_protocol::prepareres res;
        int result = hdl.get_rpcc()->call(paxos_protocol::preparereq, this->me, arg, res, rpcc::to(1000));

        if( result == paxos_protocol::OK ) {
			if( res.oldinstance == 0) {
		        if( res.accept ){
		            accepts.push_back( *node );

                }

                // update biggest propose
                if(res.n_a > arg.n ){
                    v = res.v_a;
                }
            }
            else{
                acc->commit(instance, res.v_a);
            }
        }
    }
    return true;
}


/**
 * @brief proposer::accept
 * @param instance
 * @param accepts
 * @param nodes
 * @param v
 */
void
proposer::accept(unsigned instance, std::vector<std::string> &accepts,
                 std::vector<std::string> nodes, std::string v)
{
    // TODO
    std::vector<std::string>::iterator node;
    for(node = nodes.begin() ; node != nodes.end() ; node++) {
        handle hdl( *node );


        paxos_protocol::acceptarg arg;
        arg.instance=instance;
        arg.n = my_n;

        arg.v = v;
        int accepted;
        pthread_mutex_unlock(&pxs_mutex);
        int result = hdl.get_rpcc()->call(paxos_protocol::acceptreq, me, arg, accepted, rpcc::to(1000));
        pthread_mutex_lock(&pxs_mutex);

        if(result == paxos_protocol::OK){
            if( accepted ){
                accepts.push_back( *node );
            }
        }

    }

}

/**
 * @brief proposer::decide
 * @param instance
 * @param accepts
 * @param v : view
 */
void
proposer::decide(unsigned instance, std::vector<std::string> accepts,
                 std::string v)
{
    // TODO

   std::vector<std::string>::iterator node;
   for(node = accepts.begin() ; node != accepts.end() ; node++) {
       handle hdl( *node );

       paxos_protocol::decidearg arg;
       arg.instance = instance;
       arg.v = v;

       int r;
       hdl.get_rpcc()->call(paxos_protocol::decidereq, me, arg, r, rpcc::to(1000));
   }
}

acceptor::acceptor(class paxos_change *_cfg, bool _first, std::string _me,
                   std::string _value)
    : cfg(_cfg), me (_me), instance_h(0)
{
    assert (pthread_mutex_init(&pxs_mutex, NULL) == 0);

    n_h.n = 0;
    n_h.m = me;
    n_a.n = 0;
    n_a.m = me;
    v_a.clear();

    l = new log (this, me);

    if (instance_h == 0 && _first) {
        values[1] = _value;
        l->loginstance(1, _value);
        instance_h = 1;
    }

    pxs = new rpcs(atoi(_me.c_str()));
    pxs->reg(paxos_protocol::preparereq, this, &acceptor::preparereq);
    pxs->reg(paxos_protocol::acceptreq, this, &acceptor::acceptreq);
    pxs->reg(paxos_protocol::decidereq, this, &acceptor::decidereq);
}

/**
 * @brief acceptor::preparereq
 *
 * acceptor prepare(instance, n) handler:
 *      if instance <= instance_h
 *          reply oldinstance(instance, instance_value)
 *      else if n > n_h
 *          n_h = n
 *          reply prepare_ok(n_a, v_a)
 *
 *
 *
 * @param src   : proposer id
 * @param a     : proposal received <n,v>
 * @param r     : response
 * @return status
 */
paxos_protocol::status
acceptor::preparereq(std::string src, paxos_protocol::preparearg a,
                     paxos_protocol::prepareres &r)
{
    // handle a preparereq message from proposer
    if(a.instance <= this->instance_h){

        // reply to update old instance
        r.oldinstance = 1;
        r.n_a = this->n_a;
        r.v_a = this->values[a.instance];

    }
    else {
        r.oldinstance = 0;
        r.n_a = this->n_a;
        r.v_a = this->v_a;

        if(a.n > this->n_h){

            // log new value
            this->n_h = a.n;
            this->l->loghigh(a.n);

            // prepare_ok
            r.accept = 1;
            r.oldinstance = 0;
        }
        else{
            // ignore
            r.accept = 0;
        }
    }

    return paxos_protocol::OK;
}

/**
 * @brief acceptor::acceptreq
 *
 * acceptor accept(instance, n, v) handler:
 *      if n >= n_h
 *          n_a = n
 *          v_a = v
 *          reply accept_ok(n)
 *
 *
 * @param src   : src node
 * @param a     : proposal
 * @param r     : int
 * @return
 */
paxos_protocol::status
acceptor::acceptreq(std::string src, paxos_protocol::acceptarg a, int &r)
{
    // handle an acceptreq message from proposer
    if(a.n >= this->n_h){
        this->n_a = a.n;
        this->v_a = a.v;

        r = 1;

        //Log max acepted prop
        this->l->logprop(a.n, a.v);
    }else{
        r = 0;
    }

    return paxos_protocol::OK;
}

/**
 * @brief acceptor::decidereq
 *
 * paxos_commit(instance, v)
 *
 * @param src
 * @param a
 * @param r
 * @return
 */
paxos_protocol::status
acceptor::decidereq(std::string src, paxos_protocol::decidearg a, int &r)
{
    this->commit(a.instance, a.v);
    return paxos_protocol::OK;

}

void
acceptor::commit_wo(unsigned instance, std::string value)
{
    //assume pxs_mutex is held
    printf("acceptor::commit: instance=%d has v= %s\n", instance, value.c_str());
    if (instance > instance_h) {
        printf("commit: highestaccepteinstance = %d\n", instance);
        values[instance] = value;
        l->loginstance(instance, value);
        instance_h = instance;
        n_h.n = 0;
        n_h.m = me;
        n_a.n = 0;
        n_a.m = me;
        v_a.clear();
        if (cfg) {
            pthread_mutex_unlock(&pxs_mutex);
            cfg->paxos_commit(instance, value);
            pthread_mutex_lock(&pxs_mutex);
        }
    }
}

void
acceptor::commit(unsigned instance, std::string value)
{
    pthread_mutex_lock(&pxs_mutex);
    commit_wo(instance, value);
    pthread_mutex_unlock(&pxs_mutex);
}

std::string
acceptor::dump()
{
    return l->dump();
}

void
acceptor::restore(std::string s)
{
    l->restore(s);
    l->logread();
}



// For testing purposes

// Call this from your code between phases prepare and accept of proposer
void
proposer::breakpoint1()
{
    if (break1) {
        printf("Dying at breakpoint 1!\n");
        exit(1);
    }
}

// Call this from your code between phases accept and decide of proposer
void
proposer::breakpoint2()
{
    if (break2) {
        printf("Dying at breakpoint 2!\n");
        exit(1);
    }
}

void
proposer::breakpoint(int b)
{
    if (b == 3) {
        printf("Proposer: breakpoint 1\n");
        break1 = true;
    } else if (b == 4) {
        printf("Proposer: breakpoint 2\n");
        break2 = true;
    }
}
