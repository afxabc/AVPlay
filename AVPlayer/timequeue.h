#ifndef _BASE_TIME_QUEUE_H__
#define _BASE_TIME_QUEUE_H__

#include <map>
#include "base/mutex.h"

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

	int64_t insert(const QDATA& data, int64_t when)
	{
		Lock lock(mutex_);
		queue_.insert(QPAIR(when, data));
		QUEUE::iterator it = queue_.begin();
		assert(it != queue_.end());
		return it->first;
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

	bool peerFront(int64_t& when)
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
		return queue_.size();
	}

private:
	typedef std::multimap<int64_t, QDATA> QUEUE;
	typedef std::pair<int64_t, QDATA> QPAIR;
	QUEUE queue_;
	mutable Mutex mutex_;
};

#endif
