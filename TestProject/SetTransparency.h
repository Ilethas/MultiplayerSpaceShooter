#ifndef SET_TRANSPARENCY_H_
#define SET_TRANSPARENCY_H_
#include "Actor.h"
#include "Coroutine.h"


class SetTransparency : public Coroutine
{
	int state = 0;
	std::weak_ptr<Actor> whatActor;
	float intensity;
	float currentValue;
	float delay;
	float factor;

	std::unique_ptr<CoroutineResultType> operator()();

	public:
		SetTransparency(std::weak_ptr<Actor> whatActor, float intensity, float delay, float factor);
};


#endif