#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>
#include <map>
#include <set>

#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"

class lock_server_cache {
 private:
  int nacquire;
  // status in the server:
  // LOCKED_WITHOUT_WAITING:	locked but no clients waiting for the lock
  // LOCKED_WAITING:			locked but multiple clients waiting for it
  enum lock_server_cache_status {UNLOCKED, LOCKED_WITHOUT_WAITING, LOCKED_WAITING};

  /* 
  The server's per-lock state should include whether it is held by some client,
  the ID (host name and port number) of that client, and the set of other clients
  waiting for that lock.
  */
  struct lockCache_status
  {
	public:
	lockCache_status(){};
	~lockCache_status(){};
	/* data */
	// the ID (host name and port number) of that client
	std::string id;
	// the id to send retry to
	std::string retry_id;
	// the set of other clients waiting for that lock
	std::set<std::string> id_set;
	// status of the lock
	lock_server_cache_status status;
  };

  std::map<lock_protocol::lockid_t, lockCache_status> lockCache_st_map;

  static pthread_mutex_t mutex;
  static pthread_cond_t cond;
 public:

  lock_server_cache();
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  int acquire(lock_protocol::lockid_t, std::string id, int &);
  int release(lock_protocol::lockid_t, std::string id, int &);
};

#endif
