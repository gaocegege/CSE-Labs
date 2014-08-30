// demo protocol

#ifndef demo_protocol_h
#define demo_protocol_h

#include "rpc.h"

class demo_protocol {
 public:
  enum xxstatus { OK, RETRY, RPCERR, NOENT, IOERR };
  typedef int status;
  typedef unsigned long long demoVar;
  enum rpc_numbers {
    stat = 0x7001,
    rpcA,
    rpcB
  };
};

#endif 