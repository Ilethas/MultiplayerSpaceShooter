#ifndef THREADSAFE_DATA_QUEUE
#define THREADSAFE_DATA_QUEUE
#include <queue>
#include <vector>
#include <mutex>


struct ThreadsafeDataQueue
{
	std::mutex blockade;
	std::queue<std::pair<std::vector<char>, unsigned long>> dataQueue;
};


#endif