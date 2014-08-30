#   Lab-3: RPC Hands-on
###  Due: 3-4-2014 00:01 (UTC+8)
###  General Lab Info can be found in Lab Information.
#

##  Introduction
*   In this lab, you will:
    1.  use RPC to implement a single client file server
*   In the previous lab, you have finished a filesystem on a single machine. And in lab 3 (and 4), your aim is to extend it to a distributed file server. And from the architectural point of view, it now moves on to the RPC part.
    *   <img src="https://www.evernote.com/shard/s9/res/3e442e35-5f8d-47c6-acb6-90998e6ca134.png">
*   If you have questions about this lab, either in programming environment or requirement, please ask TA: Di Xiao (xiaodi@sjtu.edu.cn).

##  Getting started
*   Please backup your solution to lab1 and lab2 before starting the steps below

    *   At first, please remember to save your lab2 solution:
    <pre>
    % cd lab-cse/lab2
    % git commit -a -m “solution for lab2” </pre>

    *   Then, pull from the repository:
    <pre>
    % git pull
    remote: Counting objects: 43, done.
    …
    [new branch]      lab3      -> origin/lab3
    Already up-to-date
    </pre> 

    *   Then, change to lab3 branch:
    <pre> 
    % git checkout lab3
     </pre>

    *   Merge with lab2, and solve the conflict by yourself (mainly in fuse.cc and yfs_client.cc):
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
    
    *   if there's no error in make, 3 executable files  yfs_client, extent_server, test-lab-3-a will be generated.

*   Note: For Lab2 and beyond, you'll need to use a computer that has the FUSE module, library, and headers installed. You should be able to install these on your own machine by following the instructions at [FUSE: Filesystem in Userspace](http://fuse.sourceforge.net/) (see Lab Information)
*   Note: Both 32-bit and 64-bit librpc are provided, so the lab should be architecture independent.
*   Note: For this lab, you will not have to worry about server failures or client failures. You also need not be concerned about malicious or buggy applications.

##  Distributed FileSystem (Strawman's Approach)
*   In lab2, we have implemented a file system on a single machine. In this lab, we just extend the single machine fils system to a distributed file system.
*   Separating extent service from yfs logic brings us a lot of advantages, such as no fate sharing with yfs client, high availability.
*   Luckily, most of your job has been done in the previous lab. You now can use extent service provided by extent_server through RPC in extent_client. Then a strawman distributed file system has been finished.
*   <i>You had better test your code with the previous test suit before any progress.</i>

##  Detailed Guidance
*   In principle, you can implement whatever design you like as long as it satisfies the requirements in the "Your Job" section and passes the testers. In practice, you should follow the detailed guidance below. 
    *   Using the RPC system:
        *   The RPC library. In this lab, you don't need to care about the implementation of RPC mechanisms, rather you'll use the RPC system to make your local filesystem become a distributed filesystem.
        *   A server uses the RPC library by creating an RPC server object (rpcs) listening on a port and registering various RPC handlers (see <code>main()</code> function in demo_server.cc).
        *   A client creates a RPC client object (rpcc), asks for it to be connected to the demo_server's address and port, and invokes RPC calls (see demo_client.cc).
        *   <b>You can learn how to use the RPC system by studying the stat call implementation.</b> please note it's for illustration purpose only, you won't need to follow the implementation
            *   use <code>make rpcdemo</code> to build the RPC demo
        *   RPC handlers have a standard interface with one to six request arguments and a reply value implemented as a last reference argument. The handler also returns an integer status code; the convention is to return zero for success and to return positive numbers for various errors. If the RPC fails in the RPC library (e.g.timeouts), the RPC client gets a negative return value instead. The various reasons for RPC failures in the RPC library are defined in rpc.h under rpc_const.
        *   The RPC system marshalls objects into a stream of bytes to transmit over the network and unmarshalls them at the other end. Beware: the RPC library does not check that the data in an arriving message have the expected type(s). If a client sends one type and the server is expecting a different type, something bad will happen. You should check that the client's RPC call function sends types that are the same as those expected by the corresponding server handler function.
        *   The RPC library provides marshall/unmarshall methods for standard C++ objects such asstd::string, int, and char. <u>You should be able to complete this lab with existing marshall/unmarshall methods. </u>

##  Testers & Grading
*   The test for this lab is test-lab-3-a. The test take two directories as arguments, issue operations in the two directories, and check that the results are consistent with the operations. Here's a successful execution of the tester:
<pre>
% ./start.sh
starting ./extent_server 29409 > extent_server.log 2>&1 &
starting ./yfs_client /home/cse/cse-2014/yfs1 29409 > yfs_client1.log 2>&1 &
starting ./yfs_client /home/cse/cse-2014/yfs2 29409 > yfs_client2.log 2>&1 &
% ./test-lab-3-a ./yfs1 ./yfs2
Create then read: OK
Unlink: OK
Append: OK
Readdir: OK
Many sequential creates: OK
Write 20000 bytes: OK
test-lab-2-a: Passed all tests.
% ./stop.sh
</pre>
*   To grade this part of lab, a test script <code>grade.sh</code> is provided. It contains are all tests from lab2 (tested on both clients), and test-lab-3-a. Here's a successful grading.
<pre>
    % ./grade.sh
    Passed A
    Passed B
    Passed C
    Passed D
    Passed E
    Passed test-lab-3-a (consistency)
    Passed all tests!
    >> Lab 3 OK
</pre>

##  Tips
*   This is also the first lab that writes null ('\0') characters to files. The std::string(char*)constructor treats '\0' as the end of the string, so if you use that constructor to hold file content or the written data, you will have trouble with this lab. Use the std::string(buf, size) constructor instead. Also, if you use C-style char[] carelessly you may run into trouble!
*   Do notice that a non RPC version may pass the tests, but RPC is checked against in actual grading. So please refrain yourself from doing so ;)

##  Handin procedure
*   After all above done:
<pre>
% make handin
</pre>
*   That should produce a file called lab3.tgz in the directory. Change the file name to your student id:
<pre>
% mv lab3.tgz lab3_[your student id].tgz
</pre>
*   Then upload lab3_[your student id].tgz file to ftp://xiaodi:public@public.sjtu.edu.cn/upload/lab3 before the deadline. You are only given the permission to list and create new file, but no overwrite and read. So make sure your implementation has passed all the tests before final submit.
*   You will receive full credits if your software passes the same tests we gave you when we run your software on our machines.