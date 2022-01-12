#include "SetTransparency.h"


std::unique_ptr<CoroutineResultType> SetTransparency::operator()()
{
	auto actor = whatActor.lock();

	coroutineBegin();
	if (!actor)
		yieldReturn(mkUniq(CoroutineFinished()));

	currentValue = static_cast<float>(actor->getOpacity());

	while (std::abs(currentValue - intensity) > 0.01f)
	{
		currentValue = currentValue*(1.0f - factor) + intensity*factor;

		if (!actor)
			yieldReturn(mkUniq(CoroutineFinished()));
		actor->setOpacity(static_cast<int>(currentValue));

		yieldReturn(mkUniq(WaitForSeconds(delay)));
	}

	coroutineEnd(mkUniq(CoroutineFinished()));
}


// Linearily interpolates whatActor's transparency from the value
// of 'currentValue' to 'intensity' by 'factor' every 'delay'
// seconds.
SetTransparency::SetTransparency(std::weak_ptr<Actor> whatActor, float intensity, float delay, float factor)
	: whatActor(whatActor)
	, intensity(intensity)
	, delay(delay)
	, factor(factor)
{}