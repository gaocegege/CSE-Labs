LAB=4
SOL=0
RPC=./rpc
LAB1GE=$(shell expr $(LAB) \>\= 1)
LAB2GE=$(shell expr $(LAB) \>\= 2)
LAB3GE=$(shell expr $(LAB) \>\= 3)
LAB4GE=$(shell expr $(LAB) \>\= 4)
LAB5GE=$(shell expr $(LAB) \>\= 5)
LAB6GE=$(shell expr $(LAB) \>\= 6)
LAB7GE=$(shell expr $(LAB) \>\= 7)
CXXFLAGS =  -g -MMD -Wall -I. -I$(RPC) -DLAB=$(LAB) -DSOL=$(SOL) -D_FILE_OFFSET_BITS=64
FUSEFLAGS= -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=25 -I/usr/local/include/fuse -I/usr/include/fuse

# choose librpc based on architecture
ifeq ($(shell getconf LONG_BIT),64)
	RPCLIB= librpc64.a
else
	RPCLIB= librpc86.a
endif

ifeq ($(shell uname -s),Darwin)
  MACFLAGS= -D__FreeBSD__=10
else
  MACFLAGS=
endif
LDFLAGS = -L. -L/usr/local/lib
LDLIBS = -lpthread 
ifeq ($(LAB1GE),1)
  ifeq ($(shell uname -s),Darwin)
    ifeq ($(shell sw_vers -productVersion | sed -e "s/.*\(10\.[0-9]\).*/\1/"),10.6)
      LDLIBS += -lfuse_ino64
    else
      LDLIBS += -lfuse
    endif
  else
    LDLIBS += -lfuse
  endif
endif
LDLIBS += $(shell test -f `gcc -print-file-name=librt.so` && echo -lrt)
LDLIBS += $(shell test -f `gcc -print-file-name=libdl.so` && echo -ldl)
CC = g++
CXX = g++

lab:  lab$(LAB)
lab1: lab1_tester
lab2: yfs_client 
lab3: yfs_client extent_server test-lab-3-g
lab4: lock_server lock_tester lock_demo yfs_client extent_server test-lab-4-a test-lab-4-b
lab5: yfs_client extent_server lock_server lock_tester test-lab-4-b\
	 test-lab-4-c
lab6: yfs_client extent_server lock_server test-lab-4-b test-lab-4-c
lab7: lock_server rsm_tester
lab8: lock_tester lock_server rsm_tester

hfiles1=rpc/fifo.h rpc/connection.h rpc/rpc.h rpc/marshall.h rpc/method_thread.h\
	rpc/thr_pool.h rpc/pollmgr.h rpc/jsl_log.h rpc/slock.h rpc/rpctest.cc\
	lock_protocol.h lock_server.h lock_client.h gettime.h gettime.cc lang/verify.h \
        lang/algorithm.h
hfiles2=yfs_client.h extent_client.h extent_protocol.h extent_server.h
hfiles3=lock_client_cache.h lock_server_cache.h handle.h tprintf.h
hfiles4=log.h rsm.h rsm_protocol.h config.h paxos.h paxos_protocol.h rsm_state_transfer.h rsmtest_client.h tprintf.h
hfiles5=rsm_state_transfer.h rsm_client.h
rsm_files = rsm.cc paxos.cc config.cc log.cc handle.cc

rpc/rpctest=rpc/rpctest.cc
rpc/rpctest: $(patsubst %.cc,%.o,$(rpctest)) rpc/$(RPCLIB)

lock_demo=lock_demo.cc lock_client.cc
lock_demo : $(patsubst %.cc,%.o,$(lock_demo)) rpc/$(RPCLIB)

lock_tester=lock_tester.cc lock_client.cc
ifeq ($(LAB5GE),1)
  lock_tester += lock_client_cache.cc
endif
ifeq ($(LAB7GE),1)
  lock_tester+=rsm_client.cc handle.cc lock_client_cache_rsm.cc
endif
lock_tester : $(patsubst %.cc,%.o,$(lock_tester)) rpc/$(RPCLIB)

lock_server=lock_server.cc lock_smain.cc
ifeq ($(LAB5GE),1)
  lock_server+=lock_server_cache.cc handle.cc
endif
ifeq ($(LAB6GE),1)
  lock_server+= $(rsm_files)
endif
ifeq ($(LAB7GE),1)
  lock_server+= lock_server_cache_rsm.cc
endif

lock_server : $(patsubst %.cc,%.o,$(lock_server)) rpc/$(RPCLIB)

lab1_tester=lab1_tester.cc extent_client.cc extent_server.cc inode_manager.cc
lab1_tester : $(patsubst %.cc,%.o,$(lab1_tester))
yfs_client=yfs_client.cc extent_client.cc fuse.cc extent_server.cc inode_manager.cc
ifeq ($(LAB3GE),1)
  yfs_client += lock_client.cc
endif
ifeq ($(LAB7GE),1)
  yfs_client += rsm_client.cc lock_client_cache_rsm.cc
endif
ifeq ($(LAB5GE),1)
  yfs_client += lock_client_cache.cc
endif
yfs_client : $(patsubst %.cc,%.o,$(yfs_client)) rpc/$(RPCLIB)

extent_server=extent_server.cc extent_smain.cc inode_manager.cc
extent_server : $(patsubst %.cc,%.o,$(extent_server)) rpc/$(RPCLIB)

test-lab-3-b=test-lab-3-b.c
test-lab-3-b:  $(patsubst %.c,%.o,$(test_lab_4-b)) rpc/$(RPCLIB)

test-lab-3-c=test-lab-3-c.c
test-lab-4-c:  $(patsubst %.c,%.o,$(test_lab_4-c)) rpc/$(RPCLIB)

rsm_tester=rsm_tester.cc rsmtest_client.cc
rsm_tester:  $(patsubst %.cc,%.o,$(rsm_tester)) rpc/$(RPCLIB)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

fuse.o: fuse.cc
	$(CXX) -c $(CXXFLAGS) $(FUSEFLAGS) $(MACFLAGS) $<

# mklab.inc is needed by 6.824 staff only. Just ignore it.
-include mklab.inc

-include *.d
-include rpc/*.d

clean_files=rpc/rpctest rpc/*.o rpc/*.d *.o *.d yfs_client extent_server lock_server lock_tester lock_demo rpctest test-lab-3-a test-lab-3-b test-lab-3-c test-lab-4-a test-lab-4-b rsm_tester lab1_tester
.PHONY: clean handin
clean: 
	rm $(clean_files) -rf 

handin_ignore=$(clean_files) core* *log
handin_file=lab$(LAB).tgz
labdir=$(shell basename $(PWD))
handin: 
	@bash -c "cd ../; tar -X <(tr ' ' '\n' < <(echo '$(handin_ignore)')) -czvf $(handin_file) $(labdir); mv $(handin_file) $(labdir); cd $(labdir)"
	@echo Please modify lab4.tgz to lab4_[your student id].tgz and upload it to ftp://xiaodi:public@public.sjtu.edu.cn/upload/lab4	
	@echo Thanks!
