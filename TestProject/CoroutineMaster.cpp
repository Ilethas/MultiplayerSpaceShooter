#include "CoroutineMaster.h"


// If coroutine pointed by coroutineAddress is
// still in the coroutineList, returns true.
// Returns false otherwise.
bool CoroutineMaster::isRunning(Coroutine *coroutineAddress)
{
	for (auto i = coroutineList.begin(); i != coroutineList.end();)
	{
		if (i->coroutine.get() == coroutineAddress)
			return true;
		else
			i++;
	}
	return false;
}


// If coroutine pointed by coroutineAddress is
// still in the coroutineList, returns true.
// Returns false otherwise.
bool CoroutineMaster::isRunning(const std::string &name)
{
	for (auto i = coroutineList.begin(); i != coroutineList.end();)
	{
		if (i->name == name)
			return true;
		else
			i++;
	}
	return false;
}


// If coroutine pointed by coroutineAddress is
// still in the coroutineList, it's removed.
// Returns false if the coroutine was not
// in the list.
bool CoroutineMaster::stopCoroutine(Coroutine *coroutineAddress)
{
	for (auto i = coroutineList.begin(); i != coroutineList.end();)
	{
		if (i->coroutine.get() == coroutineAddress)
		{
			coroutineList.erase(i);
			return true;
		}
		else
			i++;
	}
	return false;
}


// If coroutine with specified name is
// still in the coroutineList, it's removed.
// Returns false if the coroutine was not
// in the list.
bool CoroutineMaster::stopCoroutine(const std::string &name)
{
	for (auto i = coroutineList.begin(); i != coroutineList.end();)
	{
		if (i->name == name)
		{
			coroutineList.erase(i);
			return true;
		}
		else
			i++;
	}
	return false;
}


void CoroutineMaster::stopAllCoroutines()
{
	coroutineList.clear();
}


void CoroutineMaster::executeCoroutines()
{
	for (auto i = coroutineList.begin(); i != coroutineList.end();)
	{
		// Coroutine returns an object which tells us if
		// further execution should be suspended or not.
		// We check if last returned result (if any)
		// says that we should wait.
		if (i->yieldResult.get() != nullptr && i->yieldResult->keepWaiting())
		{
			i++;
			continue;
		}

		// We resume the coroutine after we are sure
		// we are allowed to do it. Afterwards, we store
		// the result in the record as the last obtained
		// result.
		std::unique_ptr<CoroutineResultType> result = (*(i->coroutine))();
		i->yieldResult = std::move(result);

		// After obtaining another yield result through
		// coroutine execution, we ask if it tells us
		// that we should terminate the coroutine.
		if (i->yieldResult.get() != nullptr && i->yieldResult->terminate())
			coroutineList.erase(i++);
		else
			i++;
	}
}