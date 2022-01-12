#include "Component.h"


Component::Component(const Component &component)
{
	ownerActor = component.ownerActor;
}


std::weak_ptr<Actor> Component::getOwnerActor()
{
	return ownerActor;
}


std::weak_ptr<const Actor> Component::getOwnerActor() const
{
	return ownerActor;
}


std::weak_ptr<Component> Component::getHandle()
{
	return handle;
}


std::weak_ptr<const Component> Component::getHandle() const
{
	return handle;
}


bool Component::isDestroyed() const
{
	return toBeDestroyed;
}


void Component::destroy()
{
	toBeDestroyed = true;
}