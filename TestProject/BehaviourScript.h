#ifndef BEHAVIOUR_SCRIPT_H_
#define BEHAVIOUR_SCRIPT_H_
#include <SFML/Graphics.hpp>
#include "Component.h"
#include "CoroutineMaster.h"


class BehaviourScript : public Component
{
	friend class Actor;

	private:
		bool started = false;

	protected:
		CoroutineMaster coroutineMaster;

	public:
		BehaviourScript() = default;
		BehaviourScript(const BehaviourScript &beh) {}
		virtual ~BehaviourScript() = 0 {}

		virtual void awake() {}
		virtual void start() {}
		virtual void update() {}
		virtual void onKeyboardEvent(sf::Event event) {}
		virtual void onMouseEvent(sf::Event event) {}
		virtual void onCollision(std::weak_ptr<Actor> other) {}

		void executeCoroutines();
};


#endif