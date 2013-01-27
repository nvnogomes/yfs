yfs
===

Practical work of the Distributed Systems and Algorithms class 2012.

Lab overview

In this sequence of labs, you'll build a multi-server file system called Yet-Another File System (yfs) in the spirit of Frangipani. At the end of all the labs, your file server architecture will look like this:

You'll write a file server process, labeled yfs above, using the FUSE toolkit. Each client host will run a copy of yfs. yfs will appear to local applications on the same machine by registering via FUSE to receive file system events from the operating system. The yfs extent server will store all the file system data on an extent server on the network, instead of on a local disk. yfs servers on multiple client hosts can share the file system by sharing a single extent server.

This architecture is appealing because (in principle) it shouldn't slow down very much as you add client hosts. Most of the complexity is in the per-client yfs program, so new clients make use of their own CPUs rather than competing with existing clients for the server's CPU. The extent server is shared, but hopefully it's simple and fast enough to handle a large number of clients. In contrast, a conventional NFS server is pretty complex (it has a complete file system implementation) so it's more likely to be a bottleneck when shared by many NFS clients.
Lab assignments
Lab 1 - Lock Server
Lab 2 - Basic File Server
Lab 3 - File Server: Reading, Writing and Sharing Files
Lab 4 - MKDIR, REMOVE, and Locking
Lab 5 - Paxos
Lab 6 - Replicated lock server
