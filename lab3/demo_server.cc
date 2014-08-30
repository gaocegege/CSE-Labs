// demo server

#include <string>
#include <map>
#include <pthread.h>
#include "demo_protocol.h"
#include "rpc.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

class demo_server {
 public:
  demo_server() {};
  ~demo_server() {};
  demo_protocol::status stat(int clt, demo_protocol::demoVar a, int &);
  // for illustration only
      // demo_protocol::status rpcA(int clt, demo_protocol::demoVar a, int &);
      // demo_protocol::status rpcB(int clt, demo_protocol::demoVar a, int &);
};

demo_protocol::status
demo_server::stat(int clt, demo_protocol::demoVar var, int &r)
{
  demo_protocol::status ret = demo_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = 0;
  return ret;
}


// Main loop of demo_server

int main(int argc, char const *argv[])
{
    int count = 0;

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    srandom(getpid());

    if(argc != 2){
      fprintf(stderr, "Usage: %s port\n", argv[0]);
      exit(1);
    }

    char *count_env = getenv("RPC_COUNT");
    if(count_env != NULL){
      count = atoi(count_env);
    }

    demo_server ds;  
    rpcs server(atoi(argv[1]), count);
    server.reg(demo_protocol::stat, &ds, &demo_server::stat);

    while(1)
      sleep(1000);
}
