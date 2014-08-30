// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

pthread_mutex_t extent_client::mutex = PTHREAD_MUTEX_INITIALIZER;

extent_client::extent_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }
}

// a demo to show how to use RPC
extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
           extent_protocol::attr &attr)
{
  extent_protocol::status ret = extent_protocol::OK;
  printf("ec->GETATTR------------------------------------------begin\n");
  pthread_mutex_lock(&mutex);
  // ret = cl->call(extent_protocol::getattr, eid, attr);
    if (extent_cache.find(eid) == extent_cache.end()) 
    {
      extent_cache[eid].validForBuf = false;
      extent_cache[eid].validForAttr = false;

      extent_cache[eid].dirty = false;

      printf("Eid: %llu, extent_cache.find(eid) == extent_cache.end()\n", eid);
    }

  if (extent_cache[eid].validForAttr == true) 
  {
    attr = extent_cache[eid].attr;
  }
  else 
  {
    ret = cl->call(extent_protocol::getattr, eid, attr);

    extent_cache[eid].attr = attr;
    extent_cache[eid].validForAttr = true;

    printf("Eid: %llu, cl->call(getattr)\n", eid);
  }
  printf("ec->GETATTR------------------------------------------end\n");
  pthread_mutex_unlock(&mutex);
  return ret;
}

extent_protocol::status
extent_client::create(uint32_t type, extent_protocol::extentid_t &id)
{
  extent_protocol::status ret = extent_protocol::OK;
  printf("ec->CREATE------------------------------------------begin\n");
  pthread_mutex_lock(&mutex);
  // Your lab3 code goes here
  ret = cl->call(extent_protocol::create, type, id);
  if (extent_cache.find(id) == extent_cache.end()) 
  {
    extent_cache[id].dirty = false;
    extent_cache[id].validForBuf = false;
    extent_cache[id].validForAttr = false;
    printf("extent_cache.find(eid) == extent_cache.end()\n");
  }

  extent_cache[id].buf = "";
  extent_cache[id].attr.size = 0;
  // may have a bug
  // extent_cache[id].validForAttr = true;
  // extent_cache[id].validForBuf = true;

  time_t tm = time(NULL);
  extent_cache[id].attr.type = type;
  extent_cache[id].attr.atime = 0;
  extent_cache[id].attr.mtime = (uint32_t)tm;
  extent_cache[id].attr.ctime = (uint32_t)tm;

  printf("Eid: %llu, type: %d\n", id, type);
  printf("ec->CREATE------------------------------------------end\n");
  pthread_mutex_unlock(&mutex);
  return ret;
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  printf("ec->GET------------------------------------------begin\n");
  pthread_mutex_lock(&mutex);
  // Your lab3 code goes here
  // ret = cl->call(extent_protocol::get, eid, buf);

  if (extent_cache.find(eid) == extent_cache.end()) 
  {
    extent_cache[eid].dirty = false;
    extent_cache[eid].validForBuf = false;
    extent_cache[eid].validForAttr = false;
    printf("extent_cache.find(eid) == extent_cache.end()\n");
  }

  if (!extent_cache[eid].validForBuf) 
  {
    ret = cl->call(extent_protocol::get, eid, buf);
    extent_cache[eid].buf = buf;
    extent_cache[eid].validForBuf = true;
    printf("Eid: %llu, cl->call(get)\n", eid);
  }
  else 
  {
    buf = extent_cache[eid].buf;
    time_t tm = time(NULL);
    extent_cache[eid].attr.atime = (uint32_t)tm;
  }

  printf("Eid: %llu\n", eid);
  printf("Buf-size: %u\nBuf-content: %s\n", buf.size(), buf.c_str());

  printf("ec->GET------------------------------------------end\n");
  pthread_mutex_unlock(&mutex);
  return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  printf("ec->PUT------------------------------------------begin\n");
  pthread_mutex_lock(&mutex);
  // Your lab3 code goes here
  // 1: waste
  // int waste = 0;
  // ret = cl->call(extent_protocol::put, eid, buf, waste);

  if (extent_cache.find(eid) == extent_cache.end()) 
  {
    extent_cache[eid].validForAttr = false;
    printf("extent_cache.find(eid) == extent_cache.end()\n");
  }

  extent_cache[eid].buf = buf;
  extent_cache[eid].dirty = true;
  time_t tm = time(NULL);
  extent_cache[eid].attr.mtime = (uint32_t)tm;
  if (extent_cache[eid].attr.type == extent_protocol::T_DIR)
    extent_cache[eid].attr.ctime = (uint32_t)tm;
  extent_cache[eid].attr.size = buf.size();
  extent_cache[eid].validForBuf = true;

  printf("Eid: %llu\n", eid);
  printf("Buf-size: %u\nBuf-content: %s\n", buf.size(), buf.c_str());

  printf("ec->PUT------------------------------------------end\n");
  pthread_mutex_unlock(&mutex);
  return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  extent_protocol::status ret = extent_protocol::OK;
  printf("ec->REMOVE------------------------------------------begin\n");
  pthread_mutex_lock(&mutex);
  // Your lab3 code goes here
  // int waste = 0;
  // ret = cl->call(extent_protocol::remove, eid, waste);

  int waste;
  extent_cache.erase(eid);
  ret = cl->call(extent_protocol::remove, eid, waste);
  extent_cache[eid].attr.type = 0;
  extent_cache[eid].attr.size = 0;
  extent_cache[eid].attr.atime = 0;
  extent_cache[eid].attr.mtime = 0;
  extent_cache[eid].attr.ctime = 0;

  extent_cache[eid].validForBuf = false;
  extent_cache[eid].validForAttr = false;

  printf("ec->REMOVE------------------------------------------end\n");
  pthread_mutex_unlock(&mutex);
  return ret;
}

void extent_client::flush(extent_protocol::extentid_t eid)
{
  std::string buf;
  int waste;
  pthread_mutex_lock(&mutex);
  printf("ec->FLUSH------------------------------------------begin\n");
  if (extent_cache.find(eid) == extent_cache.end()) 
  {
    pthread_mutex_unlock(&mutex);
    return;
  }

  if (extent_cache[eid].dirty) 
  {
    printf("Lockid: %llu, put to the server\nBuf: %s\n", eid, extent_cache[eid].buf.c_str());
    buf = extent_cache[eid].buf;
    cl->call(extent_protocol::put, eid, buf, waste);
  }

  extent_cache.erase(eid);
  printf("ec->FLUSH------------------------------------------end\n");
  pthread_mutex_unlock(&mutex);
  return;
}
