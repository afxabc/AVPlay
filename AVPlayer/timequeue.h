#ifndef _BASE_TIME_QUEUE_H__
#define _BASE_TIME_QUEUE_H__

#include <map>
#include "base/mutex.h"
#include "base/log.h"

template <typename QDATA>
class TimeQueue
{
public:

	TimeQueue()
	{
	}

	~TimeQueue()
	{
		clear();
	}


	void insert(const QDATA& data, MicroSecond when)
	{
		Lock lock(mutex_);
		queue_.insert(QPAIR(when, data));
	}

	bool getFront(QDATA& data)
	{
		bool ret = false;
		Lock lock(mutex_);
		QUEUE::iterator it = queue_.begin();
		if (it != queue_.end())
		{
			data = it->second;
			queue_.erase(it);
			ret = true;
		}
		return ret;
	}

	bool peerFront(MicroSecond& when)
	{
		bool ret = false;
		Lock lock(mutex_);
		QUEUE::iterator it = queue_.begin();
		if (it != queue_.end())
		{
			when = it->first;
			ret = true;
		}
		return ret;
	}

	void clear()
	{
		Lock lock(mutex_);
		queue_.clear();
	}

	size_t size()
	{
		Lock lock(mutex_);
		return queue_.size();
	}

private:
	typedef std::multimap<MicroSecond, QDATA> QUEUE;
	typedef std::pair<MicroSecond, QDATA> QPAIR;
	QUEUE queue_;
	mutable Mutex mutex_;
};

#endif
