// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"

pthread_mutex_t lock_client_cache::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lock_client_cache::cond = PTHREAD_COND_INITIALIZER;

int lock_client_cache::last_port = 0;

lock_client_cache::lock_client_cache(std::string xdst, 
				     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
  srand(time(NULL)^last_port);
  rlock_port = ((rand()%32000) | (0x1 << 10));
  const char *hname;
  // VERIFY(gethostname(hname, 100) == 0);
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlock_port;
  id = host.str();
  last_port = rlock_port;
  rpcs *rlsrpc = new rpcs(rlock_port);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
  pthread_mutex_lock(&mutex);
  int ret = lock_protocol::OK;
  bool acquire = false;
  // the lock is not exist
  if (lockCache_st_map.find(lid) == lockCache_st_map.end())
  {
    // printf("acquire a new lock: %lld\n", lid);
    acquire = true;
    lockCache_status buf;
    buf.status = NONE;
    buf.revoke = false;
    buf.retry = false;
    buf.cond = PTHREAD_COND_INITIALIZER;
    buf.condForRetry = PTHREAD_COND_INITIALIZER;

    lockCache_st_map[lid] = buf;

    // if (lockCache_st_map[lid].status == ACQUIRING)
    //   printf("Yeah\n");
  }

  // if
  {
    // if it is none, acquire it.
    if (lockCache_st_map[lid].status == NONE)
    {
      acquire = true;
      lockCache_st_map[lid].status = ACQUIRING;
    }

    // if it is free, lock it
    else if (lockCache_st_map[lid].status == FREE)
    {
      lockCache_st_map[lid].status = LOCKED;
    }

    //wait
    else if (lockCache_st_map[lid].status == LOCKED || lockCache_st_map[lid].status == ACQUIRING || lockCache_st_map[lid].status == RELEASING)
    {
      printf("acquire: locked->%lld\n", lid);
      while (lockCache_st_map[lid].status == LOCKED || lockCache_st_map[lid].status == ACQUIRING || lockCache_st_map[lid].status == RELEASING)
        pthread_cond_wait(&lockCache_st_map[lid].cond, &mutex);

      // if it is none, acquire it.
      if (lockCache_st_map[lid].status == NONE)
      {
        acquire = true;
        lockCache_st_map[lid].status = ACQUIRING;
      }

      // if it is free, lock it
      else if (lockCache_st_map[lid].status == FREE)
      {
        lockCache_st_map[lid].status = LOCKED;
      }
    }
  }

  if (acquire == true)
  {
    /*
    When a client asks for a lock with an acquire RPC,
    the server grants the lock and responds with OK 
    if the lock is not owned by another client 
    (i.e., the lock is free). If the lock is not free,
    and there are other clients waiting for the lock, 
    the server responds with a RETRY. 
    */
    // wait until the ret is OK
    while (lockCache_st_map[lid].retry == false)
    {
      int r = -1;
      pthread_mutex_unlock(&mutex);
      ret = cl->call(lock_protocol::acquire, lid, id, r);
      pthread_mutex_lock(&mutex);
      if (ret == lock_protocol::RETRY)
      {
        while(lockCache_st_map[lid].retry == false)
        {
          // wait for retry
          pthread_cond_wait(&lockCache_st_map[lid].condForRetry, &mutex);
        }
        lockCache_st_map[lid].retry = false;
      }
      //end of the loop
      else if (ret == lock_protocol::OK)
      {
        printf("Yeah for retry \n");
        lockCache_st_map[lid].status = LOCKED;
        break;
      }
    }
  }
  pthread_mutex_unlock(&mutex);
  return ret;
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
  pthread_mutex_lock(&mutex);
  printf("release %lld\n", lid);
  // printf("hehe\n");
  int ret = lock_protocol::OK;
  bool release = false;

  // printf("hehe\n");
  // the lock is not exist
  if (lockCache_st_map.find(lid) == lockCache_st_map.end())
  {
    printf("Err\n");
  }

  else
  {
    printf("%lld's revoke is %d\n", lid, lockCache_st_map[lid].revoke);
    // revoke means that you must release the lock
    if (lockCache_st_map[lid].revoke == true)
    {
      // printf("%lld's revoke is %d\n", lid, lockCache_st_map[lid].revoke);
      printf("enter the if\n");
      lockCache_st_map[lid].revoke = false;
      int r = -1;
      pthread_mutex_unlock(&mutex);
      ret = cl->call(lock_protocol::release, lid, id, r);
      pthread_mutex_lock(&mutex);
      lockCache_st_map[lid].status = NONE;
      pthread_cond_signal(&lockCache_st_map[lid].cond);
    }
    else
    {
      printf("release: revoke is false, status is \n");
      if (lockCache_st_map[lid].status == LOCKED)
      {
        printf(" \tLOCKED\n");
        lockCache_st_map[lid].status = FREE;
        pthread_cond_signal(&lockCache_st_map[lid].cond);
      }
      else if (lockCache_st_map[lid].status == FREE)
      {
        printf("I don't know how to deal with\n");
      }
      else if (lockCache_st_map[lid].status == RELEASING)
      {
        printf("RELEASING\n");
        int r = -1;
        pthread_mutex_unlock(&mutex);
        ret = cl->call(lock_protocol::release, lid, id, r);
        pthread_mutex_lock(&mutex);
        lockCache_st_map[lid].status = NONE;
        pthread_cond_signal(&lockCache_st_map[lid].cond);
      }
    }
  }
  pthread_mutex_unlock(&mutex);
  return ret;
}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, 
                                  int &)
{
  pthread_mutex_lock(&mutex);
  printf("revoke_handler: %lld\n", lid);
  int ret = rlock_protocol::OK;
  if (lockCache_st_map.find(lid) == lockCache_st_map.end())
    printf("error: revoke_handler -> find nothing\n");

  if (lockCache_st_map[lid].status == NONE || lockCache_st_map[lid].status == ACQUIRING || lockCache_st_map[lid].status == RELEASING)
  {
    lockCache_st_map[lid].revoke = true;
    printf("%lld's revoke is %d\n", lid, lockCache_st_map[lid].revoke);
  }
  else if (lockCache_st_map[lid].status == FREE)
  {
    lockCache_st_map[lid].status = RELEASING;
    int r = -1;
    pthread_mutex_unlock(&mutex);      
    ret = cl->call(lock_protocol::release, lid, id, r);
    pthread_mutex_lock(&mutex);  
    lockCache_st_map[lid].status = NONE;
    pthread_cond_signal(&lockCache_st_map[lid].cond);
  }
  else if (lockCache_st_map[lid].status == LOCKED)
  {
    lockCache_st_map[lid].status = RELEASING;
  }
  // printf("hehe\n");
  pthread_mutex_unlock(&mutex);
  return ret;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, 
                                 int &)
{
  pthread_mutex_lock(&mutex);
  printf("retry_handler: %lld\n", lid);
  int ret = rlock_protocol::OK;
  pthread_cond_signal(&(lockCache_st_map[lid].condForRetry));
  lockCache_st_map[lid].retry = true;
  pthread_mutex_unlock(&mutex);
  return ret;
}