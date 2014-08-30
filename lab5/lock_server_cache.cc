// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"

pthread_mutex_t lock_server_cache::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lock_server_cache::cond = PTHREAD_COND_INITIALIZER;

lock_server_cache::lock_server_cache():
	nacquire (0)
{
}

/* 	When a client asks for a lock with an acquire RPC, 
	the server grants the lock and responds with OK 
	if the lock is not owned by another client (i.e., the lock is UNLOCKED). 
	If the lock is not UNLOCKED, and there are other 
	clients waiting for the lock, the server responds 
	with a RETRY. Otherwise, the server sends a revoke
	RPC to the owner of the lock, and waits for the lock
	to be released by the owner. Finally, the server sends
	a retry to the next waiting client (if any), grants the
	lock and responds with OK.	
*/
int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, 
                               int &)
{
  pthread_mutex_lock(&mutex);
  lock_protocol::status ret = lock_protocol::OK;
  printf("acquire %lld, the client id is %s.\n", lid, id.c_str());

  bool needRevoke = false;

  // the lock is not exist
  if (lockCache_st_map.find(lid) == lockCache_st_map.end())
  {
  	lockCache_status bufStatus;
  	bufStatus.status = LOCKED_WITHOUT_WAITING;
  	bufStatus.id = id;
  	bufStatus.retry_id = "";
  	lockCache_st_map[lid] = bufStatus;
  	// bufStatus.id_set.clear();
  }

  // the lock exists
  else
  {
  	// the lock is UNLOCKED for all the clients
  	if (lockCache_st_map[lid].status == UNLOCKED)
  	{
  		lockCache_st_map[lid].status = LOCKED_WITHOUT_WAITING;
  		lockCache_st_map[lid].id = id;
  		// lockCache_st_map[lid].id_set.clear();
  	}

  	//the lock is locked but no client has been waiting for it
  	else if (lockCache_st_map[lid].status == LOCKED_WITHOUT_WAITING)
  	{
  		lockCache_st_map[lid].status = LOCKED_WAITING;
  		lockCache_st_map[lid].id_set.insert(id);
  		// next id is this id
  		lockCache_st_map[lid].retry_id = id;
  		ret = lock_protocol::RETRY;
  		needRevoke = true;
  	}

  	// the lock is locked and there has been a little clients waiting for it
  	else if (lockCache_st_map[lid].status == LOCKED_WAITING)
  	{
  		if (lockCache_st_map[lid].id_set.find(lockCache_st_map[lid].retry_id) == lockCache_st_map[lid].id_set.end())
  		{
  			ret = lock_protocol::RETRY;
  			lockCache_st_map[lid].id_set.insert(id);
  		}
  		else
  		{
  			if (lockCache_st_map[lid].retry_id == id)
  			{
  				//~?
  				lockCache_st_map[lid].id = id;
  				lockCache_st_map[lid].id_set.erase(lockCache_st_map[lid].id_set.find(lockCache_st_map[lid].retry_id));
  				if (lockCache_st_map[lid].id_set.empty() == true)
  				{
  					lockCache_st_map[lid].retry_id = "";
  					lockCache_st_map[lid].status = LOCKED_WITHOUT_WAITING;
  				}
  				else
  				{
  					//set bian li~?
  					lockCache_st_map[lid].retry_id = *(lockCache_st_map[lid].id_set.begin());
  					lockCache_st_map[lid].status = LOCKED_WAITING;
  					needRevoke = true;
  				}
  			}
  			else
  			{
  				ret = lock_protocol::RETRY;
  				lockCache_st_map[lid].id_set.insert(id);
  			}
  		}
  	}
  }

  if (needRevoke == true)
  {
  	handle h(lockCache_st_map[lid].id);
  	rpcc *cl = h.safebind();
  	if(cl)
  	{
  		printf("The revoke handle is successful, lid: %lld\n", lid);
  		// Hint: don't hold any mutexes while sending an RPC.
  		pthread_mutex_unlock(&mutex);
  		int r = cl->call(rlock_protocol::revoke, lid, r);
  		pthread_mutex_lock(&mutex);
  		if (r != rlock_protocol::OK)
  		{
  			printf("The r in handle is not OK\n");
  		}
  	}
  	else
  	{
  		printf("Bind faild\n");
  	}
  }
  // don't message retry!
  pthread_mutex_unlock(&mutex);
  return ret;
}

int 
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, 
         int &r)
{
  pthread_mutex_lock(&mutex);
  lock_protocol::status ret = lock_protocol::OK;
  printf("release %lld, the client id is %s.\n", lid, id.c_str());

  bool needRetry = false;

  if (lockCache_st_map.find(lid) == lockCache_st_map.end())
  {
  	printf("release: the lock %lld is not existed\n", lid);
  }

  else
  {
  	if (lockCache_st_map[lid].status == UNLOCKED)
  	{
  		printf("release: the lock %lld is UNLOCKED\n", lid);
  	}

  	else if (lockCache_st_map[lid].status == LOCKED_WITHOUT_WAITING)
  	{
  		lockCache_st_map[lid].status = UNLOCKED;
  		lockCache_st_map[lid].id = "";
  	}

  	else if (lockCache_st_map[lid].status == LOCKED_WAITING)
  	{
  		lockCache_st_map[lid].id = "";
  		needRetry = true;
  	}
  }

  if (needRetry == true)
  {
  	handle h(lockCache_st_map[lid].retry_id);
  	rpcc *cl = h.safebind();
  	if(cl)
  	{
  		printf("The retry handle is successful\n");
  		// Hint: don't hold any mutexes while sending an RPC.
  		pthread_mutex_unlock(&mutex);
  		int r = cl->call(rlock_protocol::retry, lid, r);
  		pthread_mutex_lock(&mutex);
  		if (r != rlock_protocol::OK)
  		{
  			printf("The r in handle is not OK\n");
  		}
  	}
  	else
  	{
  		printf("Bind faild\n");
  	}
  }
  pthread_mutex_unlock(&mutex);
  return ret;
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
  tprintf("stat request\n");
  r = nacquire;
  return lock_protocol::OK;
}