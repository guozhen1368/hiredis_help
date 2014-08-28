#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "redis_help.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>

/* Put event loop in the global scope, so it can be explicitly stopped */

static long count_g = 0;
static time_t s;


void getCallback(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = (redisReply *)r;
	count_g++;
	if (count_g%100000 == 0)
	{
		long tv = time(NULL)-s;
		printf("time = %u    count = %u   speed =%0.3f\n", tv, count_g, (float)count_g/tv );
	}
    if (reply == NULL) 
	{
		//printf("getCallback reply =null\n");
		return;
	}
	/*if (reply->type == REDIS_REPLY_ERROR)
	{
	printf("argv[error]: %s\n", reply->str);
	} 
	else if(reply->type == REDIS_REPLY_STRING)
	{
	printf("argv[%s]: %s\n", (char*)privdata, reply->str);
	}
	else if (reply->type == REDIS_REPLY_INTEGER)
	{
	printf("argv[%s]: %llu\n", (char*)privdata, reply->integer);
	}
	else
	{
	printf("argv[other]: type=%d str=%s\n", reply->type, reply->str);
	}*/

}

void *proc(void *ca)
{
	RedisHelp *loop = (RedisHelp*)ca;
    loop->init();
	loop->start();
	
}

int main (int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);

    RedisHelp redis("127.0.0.1", 6379, 10);
	

	pthread_t ph;
	pthread_create(&ph, NULL, proc, (void*)&redis);

    //sleep(2);
    
   
	printf("start time %llu\n", s);
	int count = 1000000;
   int ret = 0;
   while (1)
   {
	    s = time(NULL);
		count_g = 0;
	for (int i=0; i<count; i++)
	{
		ret = redis.AsyncCommand(getCallback, (char*)"set", "SET %b %b", "some", 4, "ane", 3);
		if (ret<0)
		{
			printf("some ret =%s\n", redis.getRedisError());
			usleep(10*1000);
		}
		ret = redis.AsyncCommand(getCallback, (char*)"get", "GET %b", "some", 4);
		if (ret<0)
		{
			printf("some ret =%s\n", redis.getRedisError());
			usleep(10*1000);
		}
		const char * aaa = "ssdsdf";
		ret = redis.AsyncCommand(getCallback, (char*)"incr", "INCRBY %b 10", aaa, strlen(aaa));
		if (ret<0)
		{
			printf("some ret =%s\n", redis.getRedisError());
			usleep(10*1000);
		}
	}
	sleep(10);
   }
	//printf("time = %u    count = %u \n", time(NULL)-s, count_g);

	while (1)
	{
	sleep(10);
	}

	redis.stop();
	sleep(2);
    return 0;
}

