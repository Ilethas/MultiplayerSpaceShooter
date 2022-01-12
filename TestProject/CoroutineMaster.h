#ifndef COROUTINE_MASTER_H_
#define COROUTINE_MASTER_H_
#include <list>
#include <string>
#include "Coroutine.h"


template <typename T>
std::unique_ptr<T> mkUniq(T &&val) { return std::make_unique<T>(val); }


class CoroutineMaster
{
	private:
		struct CoroutineRecord
		{
			std::unique_ptr<Coroutine> coroutine;
			std::unique_ptr<CoroutineResultType> yieldResult;
			std::string name;
		};

		std::list<CoroutineRecord> coroutineList;

	public:
		template <typename T>
		Coroutine* startCoroutine(T coroutine, const std::string &name = std::string());
		bool isRunning(Coroutine *coroutineAddress);
		bool isRunning(const std::string &name);
		bool stopCoroutine(Coroutine *coroutineAddress);
		bool stopCoroutine(const std::string &name);
		void stopAllCoroutines();
		void executeCoroutines();
};


template <typename T>
Coroutine* CoroutineMaster::startCoroutine(T coroutine, const std::string &name)
{
	static_assert(std::is_base_of<Coroutine, T>::value, "CoroutineMaster::startCoroutine(T coroutine) parameter must derive from Coroutine class");

	coroutineList.push_back(CoroutineRecord{ std::make_unique<T>(std::forward<T>(coroutine)), nullptr, name });
	return coroutineList.back().coroutine.get();
}


#endif;