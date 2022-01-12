#ifndef COROUTINE_H_
#define COROUTINE_H_
#include <memory>
#include <functional>
#include <chrono>


#define coroutineBegin(...) switch(state) {case 0:
#define yieldReturn(...) {state = __LINE__; return __VA_ARGS__; case __LINE__:;}
#define coroutineEnd(...) state = __LINE__; case __LINE__:; return __VA_ARGS__;} return __VA_ARGS__


struct CoroutineResultType
{
	virtual ~CoroutineResultType() = 0 {}
	virtual bool keepWaiting() { return false; }
	virtual bool terminate() { return false; }
};


struct CoroutineFinished : public CoroutineResultType
{
	bool terminate() { return true; }
};


class WaitForSeconds : public CoroutineResultType
{
	private:
		// Point in time when WaitForSeconds was returned
		std::chrono::steady_clock::time_point returnDate = std::chrono::steady_clock::now();
		double duration;

	public:
		WaitForSeconds(double seconds)
			: duration(seconds)
		{}

		bool keepWaiting()
		{
			auto timeWaiting = static_cast<double>((std::chrono::steady_clock::now() - returnDate).count())/std::chrono::steady_clock::period::den;
			return duration > timeWaiting;
		}
};


class WaitUntil : public CoroutineResultType
{
	private:
		std::function<bool()> predicate;

	public:
		template <typename T>
		WaitUntil(T predicate)
			: predicate(predicate)
		{}

		bool keepWaiting() { return !predicate(); }
};


class WaitWhile : public CoroutineResultType
{
	private:
		std::function<bool()> predicate;

	public:
		template <typename T>
		WaitWhile(T predicate)
			: predicate(predicate)
		{}

		bool keepWaiting() { return predicate(); }
};


class CoroutineMaster;
class Coroutine
{
	friend class CoroutineMaster;
	protected:
		int state = 0;
		virtual std::unique_ptr<CoroutineResultType> operator()() = 0;

	public:
		virtual ~Coroutine() = 0 {}
};


#endif