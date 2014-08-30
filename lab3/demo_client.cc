// demo client

#include <string>
#include "demo_protocol.h"
#include "rpc.h"
#include <vector>
#include <arpa/inet.h>
#include <sstream>
#include <iostream>
#include <stdio.h>

// Client interface to the demo server
class demo_client {
 protected:
  rpcc *cl;
 public:
  demo_client(std::string d);
  virtual ~demo_client() {};
  virtual demo_protocol::status stat(demo_protocol::demoVar);
};

demo_client::demo_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() < 0) {
    printf("demo_client: call bind\n");
  }
}

int
demo_client::stat(demo_protocol::demoVar var)
{
  int r;
  demo_protocol::status ret = cl->call(demo_protocol::stat, cl->id(), var, r);
  VERIFY (ret == demo_protocol::OK);
  return r;
}


#include "demo_protocol.h"
#include "rpc.h"
#include <arpa/inet.h>
#include <vector>
#include <stdlib.h>
#include <stdio.h>

std::string dst;
demo_client *dc;

int
main(int argc, char *argv[])
{
  int r;

  if(argc != 2){
    fprintf(stderr, "Usage: %s [host:]port\n", argv[0]);
    exit(1);
  }

  dst = argv[1];
  dc = new demo_client(dst);
  r = dc->stat(1);
  printf ("stat returned %d\n", r);
}
