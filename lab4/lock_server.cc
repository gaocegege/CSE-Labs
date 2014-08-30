// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

pthread_mutex_t lock_server::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lock_server::cond = PTHREAD_COND_INITIALIZER;

lock_server::lock_server():
  nacquire (0)
{
  lock_st_map.clear();
  // printf("hehe\n");
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  // printf("stat request from clt %d\n", clt);
  r = nacquire;
  // printf("Fxxk you!!\n");
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
  // printf("a\n");
  lock_protocol::status ret = lock_protocol::OK;
	// Your lab4 code goes here
  // printf("acquire request from clt %d\n", clt);

  pthread_mutex_lock(&mutex);
  // new lid
  if (lock_st_map.find(lid) == lock_st_map.end()) 
  {
    // printf("hehe\n");
    lock_server_status lockState = LOCKED;
    lock_st_map[lid] = lockState;
  } 
  else 
  {
    // if the lid is locked ,still wait.
    while(lock_st_map[lid] == LOCKED)
      // reference:  http://blog.sina.com.cn/s/blog_6ffd3b5c0100mc3n.html
      pthread_cond_wait(&cond, &mutex);

    lock_st_map[lid] = LOCKED;
  }
  pthread_mutex_unlock(&mutex);
  return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
	// Your lab4 code goes here
  // printf("release request from clt %d.\n", clt);
  pthread_mutex_lock(&mutex);
  if (lock_st_map.find(lid) == lock_st_map.end())
    ret = lock_protocol::NOENT;
  else 
  {
    lock_st_map[lid] = UNLOCKED;
    pthread_cond_signal(&cond);
  }
  pthread_mutex_unlock(&mutex);
  return ret;
}