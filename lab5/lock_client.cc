// RPC stubs for clients to talk to lock_server

#include "lock_client.h"
#include "rpc.h"
#include <arpa/inet.h>

#include <sstream>
#include <iostream>
#include <stdio.h>

lock_client::lock_client(std::string dst)
{
  // printf("hehe\n");
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  // printf("hehe\n");
  cl = new rpcc(dstsock);
  // printf("hehe\n");
  if (cl->bind() < 0) {
    printf("lock_client: call bind\n");
  }
  // printf("hehe\n");
}

int
lock_client::stat(lock_protocol::lockid_t lid)
{
  int r;
  lock_protocol::status ret = cl->call(lock_protocol::stat, cl->id(), lid, r);
  VERIFY (ret == lock_protocol::OK);
  return r;
}

lock_protocol::status
lock_client::acquire(lock_protocol::lockid_t lid)
{
  // Your lab4 code goes here
  int r;
  // printf("acquire for %d, %d\n", cl->id(),lid);
  lock_protocol::status ret = cl->call(lock_protocol::acquire, cl->id(), lid, r);
  VERIFY (ret == lock_protocol::OK);
  return r;
}

lock_protocol::status
lock_client::release(lock_protocol::lockid_t lid)
{
  // Your lab4 code goes here
  int r;
  lock_protocol::status ret = cl->call(lock_protocol::release, cl->id(), lid, r);
  VERIFY (ret == lock_protocol::OK);
  return r;
}

