#include "Actor.h"


unsigned long Actor::idSeed = 0;


void Actor::updateSprite()
{
	auto parentShared = parent.lock();
	if (!parentShared)
	{
		sprite.setPosition(localPosition);
		sprite.setScale(localScale);
		sprite.setRotation(localRotation);
		return;
	}

	sprite.setPosition(
		parentShared->sprite.getPosition() +
		Tools::ScaleVector(
			Tools::rotate(
				localPosition,
				parentShared->sprite.getRotation()),
			parentShared->sprite.getScale()));

	sprite.setScale(Tools::ScaleVector(parentShared->sprite.getScale(), localScale));
	sprite.setRotation(parentShared->sprite.getRotation() + localRotation);
}


void Actor::updateChildren()
{
	for (auto &i : childrenList)
	{
		i->updateSprite();
		i->updateChildren();
	}
}


void Actor::removeDestroyedComponents()
{
	for (auto i = componentList.begin(); i != componentList.end();)
	{
		if ((*i)->isDestroyed())
		{
			Collider *col = dynamic_cast<Collider*>(i->get());
			if (col)
				colliderList.remove(col);

			componentList.erase(i++);
		}
		else
			i++;
	}
}


Actor::Actor(std::weak_ptr<Actor> root, const std::string &name)
	: root(root)
	, name(name)
	, id(idSeed++)
{}


Actor::Actor()
	: id(idSeed++)
{}


Actor::Actor(const Actor &actor)
	: id(idSeed++)
{
	*this = actor;
}


Actor& Actor::operator=(const Actor &actor)
{
	if (this == &actor)
		return *this;

	sprite = actor.sprite;
	setLocalPosition(actor.getLocalPosition());
	setLocalScale(actor.getLocalScale());
	setLocalRotation(actor.getLocalRotation());
	setOpacity(actor.getOpacity());
	setDepth(actor.getDepth());
	setEnabled(actor.getEnabled());
	tags = actor.tags;

	// Copy components
	colliderList.clear();
	componentList.clear();

	for (auto &i : actor.componentList)
	{
		componentList.push_back(i->clone().lock());

		Collider *coll = dynamic_cast<Collider*>(i.get());
		if (coll)
			colliderList.push_back(coll);
	}

	// Execute awake() method on cloned actor scripts
	for (auto &i : componentList)
	{
		BehaviourScript *beh = dynamic_cast<BehaviourScript*>(i.get());
		if (beh)
			beh->awake();
	}

	return *this;
}


void Actor::setParent(std::weak_ptr<Actor> actor)
{
	if (isRoot())
		return;

	// If actor exists
	auto actorShared = actor.lock();
	if (actorShared)
	{
		auto parentShared = parent.lock();

		// Stop if we attempt to attach actor to its parent or we try to attach it to itself
		if (parentShared == actorShared || actorShared.get() == this)
			return;

		// If parent exists
		if (parentShared)
			actorShared->childrenList.splice(actorShared->childrenList.begin(), parentShared->childrenList, self);
		else
			actorShared->childrenList.push_front(handle.lock());

		self = actorShared->childrenList.begin();
		parent = actor;
	}
	// If actor doesn't exist
	else
		setParent(root);

	updateSprite();
}


void Actor::setRoot(std::weak_ptr<Actor> root)
{
	auto parentShared = parent.lock();
	auto rootShared = root.lock();
	auto newRootShared = root.lock();

	if (newRootShared->isRoot() == false)
		return;

	if (rootShared != newRootShared)
	{
		this->root = root;
		setParent(std::weak_ptr<Actor>());
	}
}


void Actor::setTexture(const sf::Texture &tex)
{
	sprite.setTexture(tex);
}


void Actor::setOrigin(float x, float y)
{
	sprite.setOrigin(x, y);
}


void Actor::setOpacity(int val)
{
	sprite.setColor(sf::Color(((sprite.getColor().toInteger() >> 8) << 8) | val));
}


void Actor::setLocalPosition(const sf::Vector2f &position)
{
	localPosition = position;
	if (parent.expired() == false)
		updateSprite();
	else
		sprite.setPosition(position);
	updateChildren();
}


void Actor::setLocalPosition(float x, float y)
{
	localPosition = sf::Vector2f(x, y);
	if (parent.expired() == false)
		updateSprite();
	else
		sprite.setPosition(sf::Vector2f(x, y));
	updateChildren();
}


void Actor::setLocalScale(const sf::Vector2f &scale)
{
	localScale = scale;
	if (parent.expired() == false)
		updateSprite();
	else
		sprite.setScale(scale);
	updateChildren();
}


void Actor::setLocalScale(float scale)
{
	localScale = sf::Vector2f(scale, scale);
	if (parent.expired() == false)
		updateSprite();
	else
		sprite.setScale(localScale);
	updateChildren();
}


void Actor::setLocalRotation(float rotation)
{
	localRotation = rotation;
	if (parent.expired() == false)
		updateSprite();
	else
		sprite.setRotation(rotation);
	updateChildren();
}


void Actor::setEnabled(bool enabled)
{
	this->enabled = enabled;
}



void Actor::setDepth(int depth)
{
	this->depth = depth;
}



void Actor::setName(const std::string &name)
{
	this->name = name;
}


std::weak_ptr<Actor> Actor::getParent()
{
	return parent;
}


std::weak_ptr<const Actor> Actor::getParent() const
{
	return parent;
}


std::weak_ptr<Actor> Actor::getRoot()
{
	return root;
}


std::weak_ptr<const Actor> Actor::getRoot() const
{
	return root;
}


std::weak_ptr<Actor> Actor::getHandle()
{
	return handle;
}


std::weak_ptr<const Actor> Actor::getHandle() const
{
	return handle;
}


const sf::Texture& Actor::getTexture() const
{
	return *sprite.getTexture();
}



const sf::Vector2f& Actor::getOrigin() const
{
	return sprite.getOrigin();
}



int Actor::getOpacity() const
{
	return (sprite.getColor().toInteger() << 24) >> 24;
}



const sf::Vector2f& Actor::getLocalPosition() const
{
	return localPosition;
}



const sf::Vector2f& Actor::getLocalScale() const
{
	return localScale;
}



float Actor::getLocalRotation() const
{
	return localRotation;
}



const sf::Vector2f& Actor::getGlobalPosition() const
{
	return sprite.getPosition();
}



const sf::Vector2f& Actor::getGlobalScale() const
{
	return sprite.getScale();
}



float Actor::getGlobalRotation() const
{
	return sprite.getRotation();
}



bool Actor::getEnabled() const
{
	return enabled;
}



int Actor::getDepth() const
{
	return depth;
}



const std::string& Actor::getName() const
{
	return name;
}

unsigned long Actor::getId() const
{
	return id;
}


void Actor::addTag(const std::string &tag)
{
	tags.push_back(tag);
}



void Actor::removeTag(const std::string &tag)
{
	tags.remove(tag);
}



const std::list<std::string>& Actor::getTags() const
{
	return tags;
}


const std::list<Collider*>& Actor::getColliderList() const
{
	return colliderList;
}



bool Actor::isRoot() const
{
	return root.expired();
}


sf::Vector2f Actor::forward() const
{
	return Tools::rotate(sf::Vector2f(1.0f, 0.0f), sprite.getRotation());
}


sf::Vector2f Actor::right() const
{
	return Tools::rotate(sf::Vector2f(0.0f, 1.0f), sprite.getRotation());
}


std::shared_ptr<Actor> Actor::createActor(std::weak_ptr<Actor> root, const std::string &name)
{
	std::shared_ptr<Actor> newActor = std::shared_ptr<Actor>(new Actor{ root, name });
	newActor->handle = newActor;

	if (root.expired() == false)
		newActor->setParent(root);

	return newActor;
}


std::weak_ptr<Actor> Actor::getChild(const std::string &name)
{
	for (auto &i : childrenList)
	{
		if (i->name == name)
			return std::weak_ptr<Actor>(i);
	}
	return std::weak_ptr<Actor>();
}


std::weak_ptr<Actor> Actor::getChildRecursive(const std::string &name)
{
	for (auto &i : childrenList)
	{
		if (i->name == name)
			return std::weak_ptr<Actor>(i);
		else
		{
			std::weak_ptr<Actor> result = i->getChildRecursive(name);
			if (result.expired() == false)
				return result;
		}
	}
	return std::weak_ptr<Actor>();
}


const std::list<std::shared_ptr<Actor>>& Actor::getChildren() const
{
	return childrenList;
}



const std::list<std::shared_ptr<Component>>& Actor::getComponents() const
{
	return componentList;
}



void Actor::draw(sf::RenderWindow &window, bool drawColliders) const
{
	if (!enabled)
		return;

	// Lists of drawable actors and colliders
	std::list<const Actor*> drawList;
	std::list<const Collider*> collList;

	// Pushes all children colliders of an actor (including the parent) to the collList
	std::function<void(const Actor*)> getColliders = [&](const Actor* parent)
	{
		// Stop if the actor is not enabled
		if (parent->enabled == false)
			return;

		for (auto &i : parent->colliderList)
			collList.push_back(i);

		for (auto &i : parent->childrenList)
			getColliders(i.get());
	};


	// Pushes all children of an actor (including the parent) to the drawList
	std::function<void(const Actor*)> getDrawables = [&](const Actor* parent)
	{
		// Stop if the actor is not enabled
		if (parent->enabled == false)
			return;

		drawList.push_back(parent);
		for (auto &i : parent->childrenList)
			getDrawables(i.get());
	};

	// Fill the draw list and sort it by depth
	getDrawables(this);
	drawList.sort([](auto &a, auto &b) { return a->depth < b->depth; });

	// If actor has texture and is enabled, draw it
	for (auto &i : drawList)
		if (i->sprite.getTexture() != nullptr && i->enabled)
			window.draw(i->sprite);

	// Draw all colliders of the actor tree
	if (drawColliders == true)
	{
		getColliders(this);
		for (auto &i : collList)
			i->draw(window);
	}
}


void Actor::update()
{
	std::function<void(Actor*)> updateRecursive = [&](Actor *actor)
	{
		if (actor->getEnabled() == false)
			return;

		// Call start method on all scripts that are enabled and
		// and for which the method hasn't been called yet.
		// Call update method on all scripts.
		for (auto &i : actor->getComponents())
		{
			BehaviourScript *beh = dynamic_cast<BehaviourScript*>(i.get());
			if (beh)
			{
				if (beh->started == false)
				{
					beh->started = true;
					beh->start();
				}

				beh->update();
			}
		}

		// Perform the update for the children
		for (auto &i : actor->getChildren())
			updateRecursive(i.get());
	};

	updateRecursive(this);
}


void Actor::notifyScripts(void(BehaviourScript::*notifyMethod)(sf::Event event), sf::Event event)
{
	std::function<void(Actor*)> updateRecursive = [&](Actor *actor)
	{
		if (actor->enabled == false)
			return;

		// Call the input device handler method for
		// behaviour scripts
		for (auto &i : actor->getComponents())
		{
			BehaviourScript *beh = dynamic_cast<BehaviourScript*>(i.get());
			if (beh)
				(beh->*notifyMethod)(event);
		}

		for (auto &i : actor->getChildren())
			updateRecursive(i.get());
	};

	updateRecursive(this);
}


void Actor::executeCoroutines()
{
	for (auto &i : componentList)
	{
		BehaviourScript *beh = dynamic_cast<BehaviourScript*>(i.get());
		if (beh)
			beh->executeCoroutines();
	}

	for (auto &i : childrenList)
		i->executeCoroutines();
}


std::shared_ptr<Actor> Actor::clone(const std::string &newActorName) const
{
	if (isRoot())
		return nullptr;

	std::shared_ptr<Actor> newActor = std::shared_ptr<Actor>(new Actor{ root });
	newActor->handle = newActor;
	*newActor = *this;
	newActor->name = newActorName;

	return newActor;
}


std::shared_ptr<Actor> Actor::cloneWithChildren(const std::string &newActorName) const
{
	if (isRoot())
		return nullptr;

	// Clone yourself
	std::shared_ptr<Actor> newActor = std::shared_ptr<Actor>(new Actor{ root });
	newActor->handle = newActor;
	*newActor = *this;
	newActor->name = newActorName;
	
	// Clone all childrenList and attach them to yourself
	for (auto &i : childrenList)
	{
		std::shared_ptr<Actor> child = i->cloneWithChildren();
		child->name = i->name;
		child->setParent(newActor);
	}

	return newActor;
}


void Actor::removeDestroyedChildren()
{
	std::function<void(Actor*)> destroyRecursive = [&](Actor *actor)
	{
		// Physically destroy all actors marked as destroyed
		for (auto i = actor->childrenList.begin(); i != actor->childrenList.end();)
		{
			// If marked, destroy actor, otherwise check
			// whether has any components to be destroyed
			if ((*i)->toBeDestroyed)
				actor->childrenList.erase(i++);
			else
			{
				(*i)->removeDestroyedComponents();
				i++;
			}
		}

		for (auto &i : actor->childrenList)
			destroyRecursive(i.get());
	};

	destroyRecursive(this);
}


bool Actor::isDestroyed() const
{
	return toBeDestroyed;
}


void Actor::destroy()
{
	if (!isRoot())
		toBeDestroyed = true;
}