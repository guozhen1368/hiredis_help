#include "redis_help.h"

#include <stdio.h>
#include "ae.h"
#include <sys/eventfd.h>
#include <unistd.h>

static void wakeup(int fd)
{
	uint64_t one = 1;
	ssize_t n = ::write(fd, &one, sizeof one);
	if (n != sizeof one)
	{
		
	}
}

static void readWakeupEvent(aeEventLoop *el, int fd, void *privdata, int mask) {
	((void)el); ((void)privdata); ((void)mask);

	uint64_t one = 1;
	ssize_t n = ::read(fd, &one, sizeof one);
	if (n != sizeof one)
	{

	}
}


static int redis_time_cb(struct aeEventLoop *eventLoop, long long id, void *clientData)
{
	RedisHelp *r = (RedisHelp *)clientData;
	if ( r && r->init())
	{
		r->cb_("redis_time_cb : redis connect");
		return AE_NOMORE;
	}

	return AE_NONE;
}

static void connectCallback(const redisAsyncContext *c, int status) {
	RedisHelp *r = (RedisHelp*)c->data;
	if (NULL == r)
	{
		r->cb_("RedisHelp: connectCallback RedisHelp is null");
		return;
	}

	if (status != REDIS_OK) {
		r->cb_("RedisHelp: connectCallback error %s", c->errstr);
		r->setConnTimer();
		return;
	}

	r->setConnected(true);
	r->cb_("RedisHelp: connectCallback Connected...");
}

static void disconnectCallback(const redisAsyncContext *c, int status) {
	RedisHelp *r = (RedisHelp*)c->data;
	if (NULL == r)
	{
		r->cb_("RedisHelp: disconnectCallback RedisHelp is null");
		return;
	}
	
	r->setConnected(false);
	if ( !r->init())
	{
		r->setConnTimer();
	}

	if (status != REDIS_OK) {
		r->cb_("RedisHelp: disconnectCallback error %s", c->errstr);
		return;
	}

	r->cb_("RedisHelp: disconnectCallback Disconnected...");
}


RedisHelp::RedisHelp(char *ip, int port, long reconnSecond)
	: context_(NULL),
	  port_(port),
	  ip_(ip),
	  connected_(false),
	  reconnSecond_(reconnSecond),
	  cb_(printf),
	  wakeupFd_(0)
{
	loop_ = aeCreateEventLoop(64);
	if (loop_ == NULL) {
		cb_("RedisHelp : Create loop_ fail.");
	}

	wakeupFd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (wakeupFd_ != -1)
	{
		aeCreateFileEvent(loop_, wakeupFd_, AE_READABLE, readWakeupEvent, NULL);
	}

	int ret = pthread_mutex_init(&mutex_, NULL);
	if (0 != ret){
		cb_("pthread_mutex_init failed error [%d]\n", ret);
	}
}

RedisHelp::~RedisHelp()
{
	if (loop_) {
		aeDeleteEventLoop(loop_);
		loop_ = NULL;
	}

	if (context_ && connected_)
	{
		redisAsyncFree(context_);
		context_ = NULL;
	}

	::close(wakeupFd_);

	pthread_mutex_destroy(&mutex_);
}

bool RedisHelp::init()
{
	context_ = redisAsyncConnect(ip_.c_str(), port_);      //("127.0.0.1", 6379);
	if (context_->err) {
		/* Let *c leak for now... */
		cb_("RedisHelp: context_ error %s", context_->errstr);
		return false;
	}

	context_->data = this;

	redisAeAttach(loop_, context_);
	redisAsyncSetConnectCallback(context_,connectCallback);
	redisAsyncSetDisconnectCallback(context_,disconnectCallback);
	return true;
}

void RedisHelp::start()
{
	//aeMain(loop_);
	loop_->stop = 0;
	while (!loop_->stop) {
		execCommand();
		//aeProcessEvents(loop_, AE_ALL_EVENTS|AE_DONT_WAIT);
		aeProcessEvents(loop_, AE_ALL_EVENTS);
	}
}

void RedisHelp::stop()
{
	aeStop(loop_);
}

void RedisHelp::setConnTimer()
{
	aeCreateTimeEvent(loop_, reconnSecond_*1000, redis_time_cb, (void*)this, NULL);
};

void RedisHelp::execCommand()
{
	int status = -1;
	struct cmd_t cmd;

	lock();
	if (cmdQueue_.empty()) {
		unlock();
		return;
	}
	cmd = cmdQueue_.front();
	unlock();

	size_t len = cmd.len;
	if (connected_)
	{
		status = redisAsyncCommandArgv(context_, cmd.cbk, cmd.param, 1, (const char **)&cmd.cmd, &len);
		//status = redisAsyncCommand(context_, cmd.cbk, cmd.param, "GET %s", "foo");
	}
	 
	if (status == REDIS_OK)
	{
		lock();
		cmdQueue_.pop();
		unlock();
		free(cmd.cmd);
	}
}

int RedisHelp::AsyncCommand(redisCallbackFn *fn, void *privdata, const char *format, ...) 
{
	cmd_t cmd;

	cmd.cbk = fn;
	cmd.param = privdata;

	lock();
	if (cmdQueue_.size() >= QUEUE_MAX_SIZE) {
		unlock();
		return -1;
	}
	unlock();

	va_list ap;
	va_start(ap,format);
	cmd.len = redisvFormatCommand(&cmd.cmd,format,ap);
	va_end(ap);

	lock();
	if (cmdQueue_.empty())
	{
		cmdQueue_.push(cmd);
		wakeup(wakeupFd_);
	} 
	else
	{
		cmdQueue_.push(cmd);
	}
	
	unlock();
	
	return 0;
}

//int RedisHelp::AsyncCommandGet(redisCallbackFn *fn, void *privdata, const char *key, size_t keylen)
//{
//	int ret = -1;
//
//	lock();
//	if (connected_)
//	{
//		ret = redisAsyncCommand(context_, fn, privdata, "GET %b", key, keylen);
//	}
//	unlock();
//
//	return ret;
//}
//
//int RedisHelp::AsyncCommandSet(const char *key, size_t keylen, const char *value, size_t valueLen)
//{
//	int ret = -1;
//
//	lock();
//	if (connected_)
//	{
//		ret = redisAsyncCommand(context_, NULL, NULL, "SET %b %b", key, keylen, value, valueLen);
//	}
//	unlock();
//
//	return ret;
//}
//
//int RedisHelp::AsyncIncrGet(redisCallbackFn *fn, void *privdata, const char *key, size_t keylen)
//{
//	int ret = -1;
//
//	lock();
//	if (connected_)
//	{
//		ret = redisAsyncCommand(context_, fn, privdata, "INCRBY %b 100", key, keylen);
//		//ret = redisAsyncCommand(context_, fn, privdata, "INCR %b", key, keylen);
//	}
//	unlock();
//
//	return ret;
//	
//}
