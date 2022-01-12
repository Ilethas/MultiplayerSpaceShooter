#ifndef ACTOR_H_
#define ACTOR_H_
#include <SFML/Graphics.hpp>
#include <type_traits>
#include "IDestructible.h"
#include "BehaviourScript.h"
#include "Collider.h"
#include "Tools.h"


class Actor : public IDestructible
{
	private:
		Actor(std::weak_ptr<Actor> root, const std::string &name = "");

		//===== Variables for actor resources management
		std::weak_ptr<Actor> root;
		std::weak_ptr<Actor> parent;
		std::weak_ptr<Actor> handle;
		std::list<std::shared_ptr<Actor>> childrenList;
		std::list<std::shared_ptr<Actor>>::iterator self;		// points self in parent's children list
		std::list<std::shared_ptr<Component>> componentList;
		std::list<Collider*> colliderList;						// helper list for faster access to colliders

		bool toBeDestroyed = false;
		static unsigned long idSeed;
		unsigned long id;

		//===== Variables for the sprite and orientation management
		sf::Sprite sprite;
		float opacity = 255.0f;

		sf::Vector2f localPosition = sf::Vector2f();
		sf::Vector2f localScale = sf::Vector2f(1.0f, 1.0f);
		float localRotation = 0.0f;

		//===== Variables for drawing order and actor identification
		bool enabled = true;	// not displayed when not enabled (coroutines not executed as well)
		int depth = 0;			// determines the order of drawing

		std::list<std::string> tags;
		std::string name;

		//===== Helper private methods
		void updateSprite();
		void updateChildren();
		void removeDestroyedComponents();

	public:
		Actor();
		Actor(const Actor &actor);
		Actor& operator=(const Actor &actor);

		//===== Setter methods
		void setParent(std::weak_ptr<Actor> actor);
		void setRoot(std::weak_ptr<Actor> root);

		void setTexture(const sf::Texture &tex);
		void setOrigin(float x, float y);
		void setOpacity(int val);

		void setLocalPosition(const sf::Vector2f &position);
		void setLocalPosition(float x, float y);
		void setLocalScale(const sf::Vector2f &scale);
		void setLocalScale(float scale);
		void setLocalRotation(float rotation);

		void setEnabled(bool enabled);
		void setDepth(int depth);
		void setName(const std::string &name);

		//===== GetterMethods
		std::weak_ptr<Actor> getParent();
		std::weak_ptr<const Actor> getParent() const;
		std::weak_ptr<Actor> getRoot();
		std::weak_ptr<const Actor> getRoot() const;
		std::weak_ptr<Actor> getHandle();
		std::weak_ptr<const Actor> getHandle() const;

		const sf::Texture& getTexture() const;
		const sf::Vector2f& getOrigin() const;
		int getOpacity() const;

		const sf::Vector2f& getLocalPosition() const;
		const sf::Vector2f& getLocalScale() const;
		float getLocalRotation() const;

		const sf::Vector2f& getGlobalPosition() const;
		const sf::Vector2f& getGlobalScale() const;
		float getGlobalRotation() const;

		bool getEnabled() const;
		int getDepth() const;
		const std::string& getName() const;
		unsigned long getId() const;
		
		//===== Other methods
		void addTag(const std::string &tag);
		void removeTag(const std::string &tag);
		const std::list<std::string>& getTags() const;

		const std::list<Collider*>& getColliderList() const;
		bool isRoot() const;

		sf::Vector2f forward() const;
		sf::Vector2f right() const;

		static std::shared_ptr<Actor> createActor(std::weak_ptr<Actor> root, const std::string &name = "");
		std::weak_ptr<Actor> getChild(const std::string &name);
		std::weak_ptr<Actor> getChildRecursive(const std::string &name);
		const std::list<std::shared_ptr<Actor>>& getChildren() const;

		template <typename T>
		std::weak_ptr<Component> addComponent(std::weak_ptr<Component> component = std::weak_ptr<Component>());
		template <typename T>
		std::weak_ptr<Component> getComponent() const;
		template <typename T>
		std::list<std::weak_ptr<Component>> getComponents() const;
		const std::list<std::shared_ptr<Component>>& getComponents() const;

		void draw(sf::RenderWindow &window, bool drawColliders = false) const;
		void update();
		void notifyScripts(void(BehaviourScript::*notifyMethod)(sf::Event event), sf::Event event);
		void executeCoroutines();

		std::shared_ptr<Actor> clone(const std::string &newActorName = "") const;
		std::shared_ptr<Actor> cloneWithChildren(const std::string &newActorName = "") const;

		void removeDestroyedChildren();
		bool isDestroyed() const override;
		void destroy() override;
};



template <typename T>
std::weak_ptr<Component> Actor::addComponent(std::weak_ptr<Component> component)
{
	static_assert(std::is_base_of<Component, T>::value, "Actor::addComponent<T>() template T parameter must derive from Component class.");

	if (component.expired() == false)
		componentList.push_back(component.lock());
	else
		componentList.push_back(Component::createComponent<T>());
	componentList.back()->ownerActor = handle;

	if (std::is_base_of<BehaviourScript, T>::value)
		static_cast<BehaviourScript*>(componentList.back().get())->awake();
	else if (std::is_base_of<Collider, T>::value)
		colliderList.push_back(static_cast<Collider*>(componentList.back().get()));

	return componentList.back()->handle;
}


template <typename T>
std::weak_ptr<Component> Actor::getComponent() const
{
	static_assert(std::is_base_of<Component, T>::value, "Actor::getComponent<T>() template T parameter must derive from Component class.");

	for (auto &i : componentList)
	{
		T *component = dynamic_cast<T*>(i.get());
		if (component)
			return component->getHandle();
	}

	return std::weak_ptr<Component>();
}


template <typename T>
std::list<std::weak_ptr<Component>> Actor::getComponents() const
{
	static_assert(std::is_base_of<Component, T>::value, "Actor::getComponent<T>() template T parameter must derive from Component class.");

	std::list<std::weak_ptr<Component>> result;
	for (auto &i : componentList)
	{
		T *component = dynamic_cast<T*>(i.get());
		if (component)
			result.push_back(i->getHandle());
	}

	return result;
}


#endif