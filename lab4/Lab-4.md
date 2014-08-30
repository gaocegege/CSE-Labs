#   Lab-4: Lock Server
###  Due: 17-4-2014 23:59 (UTC+8) 
###  General Lab Info can be found in Lab Information.
#

##  Introduction
*   In this lab, you will:
    1. Implememnt a lock server
    1. Add locking to ensure that concurrent operations to the same file/directory from different yfs_clients occur one at a time.
*   In the previous lab, you have finished a filesystem on a single machine. And in this lab, you will extend it to a distributed file server. To work correctly, the yfs servers need a locking service to coordinate updates to the file system structures. In this lab, you'll implement a simple lock server, then use its lock service to coordinate yfs clients.
    *   <img src="http://imgur.qiniudn.com/Screenshot%20from%202014-03-29%2010:10:47.png" width="90%">
    *   Reference: [Distributed Systems (G22.3033-001, Fall 2009, NYU)](http://www.news.cs.nyu.edu/~jinyang/fa09/notes/ds-lec1.ppt)
*   If you have questions about this lab, either in programming environment or requirement, please ask TA: Di Xiao (xiaodi@sjtu.edu.cn).

##  Getting started
*   Please backup your solution to lab3 before starting the steps below

    *   At first, please remember to save your lab3 solution:
    <pre>
    % cd lab-cse/lab2
    % git commit -a -m “solution for lab3” </pre>

    *   Then, pull from the repository:
    <pre>
    % git pull
    remote: Counting objects: 43, done.
    …
    [new branch]      lab4      -> origin/lab4
    Already up-to-date
    </pre> 

    *   Then, change to lab3 branch:
    <pre> 
    % git checkout lab4
     </pre>

    *   Merge with lab3, and solve the conflict by yourself (mainly in fuse.cc, yfs_client.cc and GNUMakefile):
    <pre>% git merge lab2
    Auto-merging fuse.cc
    CONFLICT (content): Merge conflict in yfs_client.cc
    Auto-merging yfs_client.cc
    CONFLICT (content): Merge conflict in ifs_client.cc
    Automatic merge failed; fix conflicts and then commit the result
    …</pre>

    *   After merge all of the conflicts, you should be able to compile successfully:
    <pre>
        % make
    </pre>
    
    *   if there's no error in make, 6 executable files yfs_client, lock_server, lock_demo, lock_tester, test-lab-4-a, test-lab-4-b will be generated.

*   Note: For Lab2 and beyond, you'll need to use a computer that has the FUSE module, library, and headers installed. You should be able to install these on your own machine by following the instructions at [FUSE: Filesystem in Userspace](http://fuse.sourceforge.net/) (see Lab Information)
*   Note: Both 32-bit and 64-bit librpc are provided, so the lab should be architecture independent.
*   Note: For this lab, you will not have to worry about server failures or client failures. You also need not be concerned about malicious or buggy applications.

##  Part 1: Lock Server
*   We provide you with a skeleton RPC-based lock server, a lock client interface, a sample application that uses the lock client interface, and a tester. Now compile and start up the lock server, giving it a port number on which to listen to RPC requests. You'll need to choose a port number that other programs aren't using. For example:
    *   <pre>
            % make
            % ./lock_server 3772
        </pre>
    *   Now open a second terminal on the same machine and run lock_demo, giving it the port number on which the server is listening:
    *   <pre>
            % ./lock_demo 3772
            stat request from clt 1386312245
            stat returned 0
            %
        </pre>
    *   lock_demo asks the server for the number of times a particular lock has been acquired, using the stat RPC that we have provided. In the skeleton code, this will always return 0. You can use it as an example of how to add RPCs. You don't need to fix stat to report the actual number of acquisitions of the given lock in this lab, but you may if you wish.
*   The lock client skeleton does not do anything yet for the acquire and release operations; similarly, the lock server does not implement lock granting or releasing. Your job is to implement this functionality in the server, and to arrange for the client to send RPCs to the server.
### Your Job
*   Your job is to implement a correct lock server assuming a perfect underlying network. Correctness means obeying this invariant: at any point in time, there is at most one client holding a lock with a given identifier.
*   We will use the program lock_tester to check the correctness invariant, i.e. whether the server grants each lock just once at any given time, under a variety of conditions. You run lock_tester with the same arguments as lock_demo. A successful run of lock_tester (with a correct lock server) will look like this:
    *   <pre>
            % ./lock_tester 3772
            simple lock client
            acquire a release a acquire a release a
            acquire a acquire b release b release a
            test2: client 0 acquire a release a
            test2: client 2 acquire a release a
            . . .
            ./lock_tester: passed all tests successfully
        </pre>
*   If your lock server isn't correct, lock_tester will print an error message. For example, if lock_tester complains "error: server granted XXX twice", the problem is probably that lock_tester sent two simultaneous requests for the same lock, and the server granted both requests. A correct server would have granted the lock to just one client, waited for a release, and only then sent granted the lock to the second client.
### Detailed Guidance
*   In principle, you can implement whatever design you like as long as it satisfies the requirements in the "Your Job" section and passes the testers. In practice, you should follow the detailed guidance below. 
    *   Using the RPC system:
        •   A server uses the RPC library by creating an RPC server object (rpcs) listening on a port and registering various RPC handlers (see lock_smain.cc). A client creates a RPC client object (rpcc), asks for it to be connected to the lock_server's address and port, and invokes RPC calls (see lock_client.cc).
        *   Each RPC procedure is identified by a unique procedure number. We have defined the acquire and release RPC numbers you will need in lock_protocol.h. You must register handlers for these RPCs with the RPC server object (see lock_smain.cc).
        *   You can learn how to use the RPC system by studying the stat call implementation in lock_client and lock_server. RPC handlers have a standard interface with one to six request arguments and a reply value implemented as a last reference argument. The handler also returns an integer status code; the convention is to return zero for success and to return positive numbers for various errors. If the RPC fails in the RPC library (e.g.timeouts), the RPC client gets a negative return value instead. The various reasons for RPC failures in the RPC library are defined in rpc.h under rpc_const.
        *   The RPC system marshalls objects into a stream of bytes to transmit over the network and unmarshalls them at the other end. Beware: the RPC library does not check that the data in an arriving message have the expected type(s). If a client sends one type and the server is expecting a different type, something bad will happen. You should check that the client's RPC call function sends types that are the same as those expected by the corresponding server handler function.
        *   The RPC library provides marshall/unmarshall methods for standard C++ objects such asstd::string, int, and char. <u>You should be able to complete this lab with existing marshall/unmarshall methods. </u>
    *   Implementing the lock server:
        *   The lock server can manage many distinct locks. Each lock is identified by an integer of type lock_protocol::lockid_t. The set of locks is open-ended: if a client asks for a lock that the server has never seen before, the server should create the lock and grant it to the client. When multiple clients request the same lock, the lock server must grant the lock to one client at a time.
        *   You will need to modify the lock server skeleton implementation in files lock_server.{cc,h} to accept acquire/release RPCs from the lock client, and to keep track of the state of the locks. Here is our suggested implementation plan.
            *   On the server, a lock can be in one of two states:
                *   free: no clients own the client
                *   locked: some client owns the lock            
                *   The RPC handler for acquire should first check if the lock is locked, and if so, the handler should block until the lock is free. When the lock is free,acquire changes its state tolocked, then returns to the client, which indicates that the client now has the lock. The valuer returned by acquiredoesn't matter. The handler for release should change the lock state to free, and notify any threads that are waiting for the lock. Consider using the C++ STL (Standard Template Library) std::map class to hold the table of lock states.
            *    Implementing the lock client:
                *   The class lock_client is a client-side interface to the lock server (found in files lock_client.{cc,h}). The interface provides acquire() and release() functions that should send and receive RPCs. Multiple threads in the client program can use the same lock_client object and request the same lock. See lock_demo.cc for an example of how an application uses the interface. lock_client::acquire must not return until it has acquired the requested lock.
            *   Handling multi-thread concurrency:
                *   Both lock_client and lock_server's functions will be invoked by multiple threads concurrently. On the lock server side, the RPC library keeps a thread pool and invokes the RPC handler using one of the idle threads in the pool. On the lock client side, many different threads might also call lock_client's acquire() and release() functions concurrently.
                *   You should use pthread mutexes to guard uses of data that is shared among threads. You should use pthread condition variables so that the lock server acquire handler can wait for a lock. The Lab Information contain a link to information about pthreads, mutexes, and condition variables. Threads should wait on a condition variable inside a loop that checks the boolean condition on which the thread is waiting. This protects the thread from spurious wake-ups from the pthread_cond_wait() and pthread_cond_timedwait() functions. 
                *   Use a simple mutex scheme: a single pthreads mutex for all of lock_server. You don't really need (for example) a mutex per lock, though such a setup can be made to work. Using "coarse-granularity" mutexes will simplify your code.
##  Part 2: Locking
*   Next, you are going to ensure the atomicity of file system operations when there are multiple yfs_client processes sharing a file system. Your current implementation does not handle concurrent operations correctly. For example, your yfs_client's create method probably reads the directory's contents from the extent server, makes some changes, and stores the new contents back to the extent server. Suppose two clients issue simultaneous CREATEs for different file names in the same directory via different yfs_client processes. Both yfs_client processes might fetch the old directory contents at the same time and each might insert the newly created file for its client and write back the new directory contents. Only one of the files would be present in the directory in the end. The correct answer, however, is for both files to exist. This is one of many potential races. Others exist: concurrent CREATE and UNLINK, concurrent MKDIR and UNLINK, concurrent WRITEs, etc.
*   You should eliminate YFS races by having yfs_client use your lock server's locks. For example, a yfs_client should acquire a lock on the directory before starting a CREATE, and only release the lock after finishing the write of the new information back to the extent server. If there are concurrent operations, the locks force one of the two operations to delay until the other one has completed. All yfs_clients must acquire locks from the same lock server.
### Your Job
*   Your job is to add locking to yfs_client to ensure the correctness of concurrent operations. The testers for this part of the lab are test-lab-4-a and test-lab-4-b, source in test-lab-4-a.c and test-lab-4-b.c. The testers take two directories as arguments, issue concurrent operations in the two directories, and check that the results are consistent with the operations executing in some sequential order. Here's a successful execution of the testers:
    *   <pre>
            % ./start.sh
            % ./test-lab-4-a ./yfs1 ./yfs2
            Create then read: OK
            Unlink: OK
            Append: OK
            Readdir: OK
            Many sequential creates: OK
            Write 20000 bytes: OK
            Concurrent creates: OK
            Concurrent creates of the same file: OK
            Concurrent create/delete: OK
            Concurrent creates, same file, same server: OK
            test-lab-4-b: Passed all tests.
            % ./stop.sh
        </pre>
    *   <pre>
            % ./start.sh
            % ./test-lab-4-b ./yfs1 ./yfs2
            Create/delete in separate directories: tests completed OK
            % ./stop.sh
        </pre>
*   If you try this before you add locking, it should fail at "Concurrent creates" test in test-lab-3-a. If it fails before "Concurrent creates", your code may have bugs despite having passed previous testers; you should fix them before adding locks.
### Detailed Guidance
*   What to lock? 
    *   At one extreme you could have a single lock for the whole file system, so that operations never proceed in parallel. At the other extreme you could lock each entry in a directory, or each field in the attributes structure. Neither of these is a good idea! A single global lock prevents concurrency that would have been okay, for example CREATEs in different directories. Fine-grained locks have high overhead and make deadlock likely, since you often need to hold more than one fine-grained lock.
    *   You should associate a lock with each inumber. Use the file or directory's inum as the name of the lock (i.e. pass the inum to acquire and release). The convention should be that any yfs_client operation should acquire the lock on the file or directory it uses, perform the operation, finish updating the extent server (if the operation has side-effects), and then release the lock on the inum. Be careful to release locks even for error returns from yfs_client operations.
    *   You'll use your lock server from part 1. yfs_client should create and use a lock_client in the same way that it creates and uses its extent_client. 
    *   <b>(Be warned! Do not use a block/offset based locking protocol! Many adopters of a block-id-as-lock ended up refactoring their code in labs later on.)</b>
*   Things to watch out for
    *   This is the first lab that creates files using two different YFS-mounted directories. If you were not careful in earlier labs, you may find that the components that assign inum for newly-created files and directories choose the same identifiers.
    *   If your inode manager relies on pseudo-randomness to generate unique inode number, one possible way to fix this may be to seed the random number generator differently depending on the process's pid. The provided code has already done such seeding for you in the main function of fuse.cc.

##  Tips
*   This is also the first lab that writes null ('\0') characters to files. The std::string(char*)constructor treats '\0' as the end of the string, so if you use that constructor to hold file content or the written data, you will have trouble with this lab. Use the std::string(buf, size) constructor instead. Also, if you use C-style char[] carelessly you may run into trouble!
*   Do notice that a non RPC version may pass the tests, but RPC is checked against in actual grading. So please refrain yourself from doing so ;)

##  Handin procedure
*   After all above done:
<pre>
% make handin
</pre>
*   That should produce a file called lab4.tgz in the directory. Change the file name to your student id:
<pre>
% mv lab4.tgz lab4_[your student id].tgz
</pre>
*   Then upload lab4_[your student id].tgz file to ftp://xiaodi:public@public.sjtu.edu.cn/upload/lab4 before the deadline. You are only given the permission to list and create new file, but no overwrite and read. So make sure your implementation has passed all the tests before final submit.
*   You will receive full credits if your software passes the same tests we gave you when we run your software on our machines.