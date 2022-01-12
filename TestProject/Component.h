#ifndef COMPONENT_H_
#define COMPONENT_H_
#include <memory>
#include "IDestructible.h"


#define CLONEABLE_COMPONENT(...) \
std::weak_ptr<Component> clone() const override \
{ \
	return std::shared_ptr<Component>(new std::decay<decltype(*this)>::type(*this)); \
}


class Actor;


class Component : public IDestructible
{
	friend class Actor;

	private:
		bool toBeDestroyed = false;
		std::weak_ptr<Actor> ownerActor;
		std::weak_ptr<Component> handle;

	public:
		Component() = default;
		Component(const Component &component);
		virtual ~Component() = 0 {}

		template <typename T>
		static std::shared_ptr<Component> createComponent();

		std::weak_ptr<Actor> getOwnerActor();
		std::weak_ptr<const Actor> getOwnerActor() const;
		std::weak_ptr<Component> getHandle();
		std::weak_ptr<const Component> getHandle() const;

		//virtual void copy(const Component &other) const = 0;
		virtual std::weak_ptr<Component> clone() const = 0;

		bool isDestroyed() const override;
		void destroy() override;
};


template <typename T>
std::shared_ptr<Component> Component::createComponent()
{
	static_assert(std::is_base_of<Component, T>::value, "Component::createComponent<T>() template T parameter must derive from Component class.");

	std::shared_ptr<Component> newComponent = std::shared_ptr<Component>(new T);
	newComponent->handle = newComponent;
	return newComponent;
}


#endif