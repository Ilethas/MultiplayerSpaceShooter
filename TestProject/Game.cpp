#include "Game.h"


Game::Game()
	: initialWindowWidth(1200)
	, initialWindowHeight(800)
	, initialGameViewWidth(1800)
	, initialGameViewHeight(1200)
	, window(sf::VideoMode(initialWindowWidth, initialWindowHeight), "SFML works!")
{
	window.setKeyRepeatEnabled(false);
	initCollision();

	// Initialize the game viewing area
	setView(sf::Vector2f(0.0f, 0.0f), initialGameViewWidth, initialGameViewHeight);

	// Load all needed textures here
	loadTextures(textures.starsSmall, "Assets\\starsSmall.png");
	loadTextures(textures.starsMedium, "Assets\\starsMedium.png");
	loadTextures(textures.starsLarge, "Assets\\starsLarge.png");

	loadTextures(textures.dust, "Assets\\dust.png");
	loadTextures(textures.blueDust, "Assets\\blueDust.png");
	loadTextures(textures.violetDust, "Assets\\violetDust.png");
	loadTextures(textures.yellowDust, "Assets\\yellowDust.png");

	loadTextures(textures.hotNebula, "Assets\\hotNebula.png");
	loadTextures(textures.coldNebula, "Assets\\coldNebula.png");

	loadTextures(textures.exoplanet, "Assets\\exoplanet.png");
	loadTextures(textures.exoplanet2, "Assets\\exoplanet2.png");
	loadTextures(textures.exoplanet3, "Assets\\exoplanet3.png");
	loadTextures(textures.iceGiant, "Assets\\iceGiant.png");
	loadTextures(textures.gasGiant, "Assets\\gasGiant.png");

	loadTextures(textures.playerShip1, "Assets\\playerShip1.png");
	loadTextures(textures.engineJet1, "Assets\\engineJet1.png");
	loadTextures(textures.playerShipEngine, "Assets\\playerShipEngine.png");

	loadTextures(textures.plasmaFire, "Assets\\plasmaFire.png");
	loadTextures(textures.plasmaProjectile, "Assets\\plasmaProjectile.png");
	loadTextures(textures.plasmaSplash, "Assets\\plasmaSplash.png");
}


Game::CollThreadData::CollThreadData()
{}


Game::CollThreadData::CollThreadData(const CollThreadData &collThreadData)
{
	dataReady = collThreadData.dataReady;
	resultList = collThreadData.resultList;
	firstActor = collThreadData.firstActor;
	actorsToTest = collThreadData.actorsToTest;
}


Game::~Game()
{
	std::unique_lock<std::mutex> lck(stopConditionBlocade);
	stopCollisionThreads = true;
	lck.unlock();

	for (auto &i : perThreadCollisions)
		i.dataAvailability.notify_one();

	for (auto &thread : threads)
		thread.join();
}


Game& Game::get()
{
	static Game game;
	return game;
}


void Game::loadTextures(sf::Texture &texture, const std::string &path)
{
	static unsigned long texId = 0;
	if (!texture.loadFromFile(path.c_str()))
	{
		//MessageBoxA(0, ("Nie udalo sie wczytac tekstury " + path).c_str(), "Blad", MB_ICONWARNING);
	}
}


// Creates a view with specified dimensions with the point 'center'
// in the middle. Next, the view is scaled to the dimensions
// of the game window (however it's ratio remains unchanged).
void Game::setView(const sf::Vector2f &center, unsigned int newWidth, unsigned int newHeight)
{
	sf::FloatRect visibleArea(newWidth / -2.0f + center.x, newHeight / -2.0f + center.y, newWidth + center.x, newHeight + center.y);

	sf::View gameView = fitViewIn(sf::View(visibleArea), static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
	window.setView(gameView);
}


// Scales view into a rectangle with newWidth and newHeight
// dimensions and creates a proper viewport to keep view's
// ratio unchanged.
sf::View Game::fitViewIn(const sf::View &view, float newWidth, float newHeight)
{
	sf::View newView = view;
	float scale = std::min(newWidth / newView.getSize().x, newHeight / newView.getSize().y);

	float scaledWidth = newView.getSize().x * scale;
	float scaledHeight = newView.getSize().y * scale;

	float ratioX = scaledWidth / newWidth;
	float ratioY = scaledHeight / newHeight;

	newView.setViewport(sf::FloatRect((1 - ratioX) / 2, (1 - ratioY) / 2, ratioX, ratioY));
	return newView;
}


void Game::update()
{
	actorRoot->update();
}


void Game::notifyScripts(void(BehaviourScript::*notifyMethod)(sf::Event event), sf::Event event)
{
	actorRoot->notifyScripts(notifyMethod, event);
}


void Game::drawActors()
{
	std::list<Actor*> drawables;
	for (auto &i : actorRoot->getChildren())
		drawables.push_back(i.get());

	drawables.sort([](auto &a, auto &b) { return a->getDepth() < b->getDepth(); });
	for (auto &i : drawables)
		i->draw(window, drawColliders);
}


void Game::executeActorCoroutines()
{
	actorRoot->executeCoroutines();
}


std::weak_ptr<Actor> Game::getRootActor()
{
	return actorRoot;
}


void Game::removeDestroyedActors()
{
	actorRoot->removeDestroyedChildren();
}


// Runs a number of threads which purpose is to test if a number of
// actors collide with each other when notified
void Game::initCollision()
{
	unsigned int threadsCount = std::thread::hardware_concurrency();
	if (threadsCount == 0)
		threadsCount = 2;

	perThreadCollisions.resize(threadsCount);
	threads.reserve(threadsCount);

	for (int i = 0; i < static_cast<int>(threadsCount); i++)
		threads.push_back(std::thread(&Game::perThreadTest, this, std::ref<CollThreadData>(perThreadCollisions[i])));
}


// Each thread tests if a number of actors collide with each other upon notification
void Game::perThreadTest(Game::CollThreadData &collThreadData)
{
	while (true)
	{
		// Check if the thread should finish
		std::unique_lock<std::mutex> stopConditionLock(stopConditionBlocade);
		if (stopCollisionThreads == true)
			return;
		stopConditionLock.unlock();

		// Wait until the next portion of collision data is available
		std::unique_lock<std::mutex> dataBlockadeLock(collThreadData.dataBlockade);
		while (true)
		{
			if (stopCollisionThreads == true)
				return;
			else if (collThreadData.dataReady == false)
				collThreadData.dataAvailability.wait(dataBlockadeLock);
			else break;
		}

		std::pair<Actor*, Actor*> whoCollided;

		// Tests actor with its children against another actor with their children
		std::function<bool(Actor &a, Actor &b)> testActorCollision = [&](Actor &a, Actor &b)
		{
			// Test a with b
			for (auto &aCollider : a.getColliderList())
			{
				for (auto &bCollider : b.getColliderList())
				{
					if (aCollider->collisionTest(*bCollider))
					{
						whoCollided.first = &a;
						whoCollided.second = &b;
						return true;
					}
				}
			}

			// Test a with all children of b and b with children of a
			auto testWithChildren = [&](Actor &a, Actor &b)
			{
				for (auto &i : b.getChildren())
				{
					if (testActorCollision(a, *i))
					{
						whoCollided.first = &a;
						whoCollided.second = &(*i);
						return true;
					}
				}

				return false;
			};

			if (testWithChildren(a, b) || testWithChildren(b, a))
				return true;

			return false;
		};

		// Perform collision test for 'actorsToTest' number of actors
		for (int i = 0; i < collThreadData.actorsToTest; i++, collThreadData.firstActor++)
		{
			for (auto other = std::next(collThreadData.firstActor); other != actorRoot->getChildren().end(); other++)
			{
				if (testActorCollision(*(*collThreadData.firstActor), *(*other)))
					collThreadData.resultList.push_back(whoCollided);
			}
		}

		collThreadData.dataReady = false;
	}
}


// Very slow without compiler optimizations (on debug).
// TODO: improve on n^2 complexity and rewrite to reduce
// the constant factor
void Game::testCollisions()
{
	using Collision = std::pair<Actor*, Actor*>;

	int actorListsize = static_cast<int>(actorRoot->getChildren().size());
	if (actorListsize <= 1)
		return;

	// Get the total amount of collision checks and determine
	// the suitable amount of tests per thread
	int totalCollisionTests = (actorListsize*(actorListsize - 1)) / 2;
	int testsPerThread = totalCollisionTests / static_cast<int>(threads.size());
	if (testsPerThread == 0)
		testsPerThread = 1;

	std::list<std::shared_ptr<Actor>>::const_iterator currentActor = actorRoot->getChildren().cbegin();
	int actorIndex = 0;
	int threadIndex = 0;
	for (int i = 0; i < totalCollisionTests && actorIndex != actorListsize - 1;)
	{
		int actorsToTest = 0;	// how many actors to test per thread
		for (int j = 0; j < testsPerThread && i < totalCollisionTests; actorIndex++)
		{
			int testsAtIndex = actorListsize - actorIndex - 1;	// tests done for actor at index

			j += testsAtIndex;
			i += testsAtIndex;
			actorsToTest++;
		}

		// Update information for collision threads and order them to do the collision tests
		std::unique_lock<std::mutex> lck(perThreadCollisions[threadIndex].dataBlockade);
		perThreadCollisions[threadIndex].actorsToTest = actorsToTest;
		perThreadCollisions[threadIndex].firstActor = currentActor;
		perThreadCollisions[threadIndex].resultList.clear();
		perThreadCollisions[threadIndex].dataReady = true;
		lck.unlock();
		perThreadCollisions[threadIndex].dataAvailability.notify_all();

		std::advance(currentActor, actorsToTest);
		threadIndex++;
	}

	std::vector<std::unique_lock<std::mutex>> locks;
	locks.reserve(threads.size());

	// Wait for all thread to finish collision checks (each blockade unlocks when the thread
	// finished processing its data)
	for (int i = 0; i < static_cast<int>(threads.size()); i++)
		locks.push_back(std::unique_lock<std::mutex>(perThreadCollisions[i].dataBlockade));

	for (auto &collisionData : perThreadCollisions)
	{
		for (auto &collision : collisionData.resultList)
		{
			// Notify all scripts of actor
			for (auto &script : collision.first->getComponents<BehaviourScript>())
				static_cast<BehaviourScript*>(script.lock().get())->onCollision(collision.second->getHandle());

			// Notify all scripts of other
			for (auto &script : collision.second->getComponents<BehaviourScript>())
				static_cast<BehaviourScript*>(script.lock().get())->onCollision(collision.first->getHandle());
		}
	}
}


// Very slow without compiler optimizations (on debug).
// TODO: improve on n^2 complexity and rewrite to reduce
//void Game::testCollisions()
//{
//	std::pair<Actor*, Actor*> whoCollided;
//
//	// Tests actor with its children against another actor with their children
//	std::function<bool(Actor &a, Actor &b)> testActorCollision = [&](Actor &a, Actor &b)
//	{
//		// Test a with b
//		for (auto &aCollider : a.colliderList)
//		{
//			for (auto &bCollider : b.colliderList)
//			{
//				if (aCollider->collisionTest(*bCollider))
//				{
//					whoCollided.first = &a;
//					whoCollided.second = &b;
//					return true;
//				}
//			}
//		}
//
//		// Test a with all children of b
//		for (auto &i : b.getChildren())
//		{
//			if (testActorCollision(a, *i))
//			{
//				whoCollided.first = &a;
//				whoCollided.second = &(*i);
//				return true;
//			}
//		}
//
//		// Test b with all children of a
//		for (auto &i : a.getChildren())
//		{
//			if (testActorCollision(b, *i))
//			{
//				whoCollided.first = &a;
//				whoCollided.second = &(*i);
//				return true;
//			}
//		}
//
//		return false;
//	};
//
//
//	// Do the shitty very slow n^2 collision test (I'm going to regret it))
//	for (auto actor = actorRoot->getChildren().begin(); actor != actorRoot->getChildren().end(); actor++)
//	{
//		for (auto other = std::next(actor); other != actorRoot->getChildren().end(); other++)
//		{
//			if (testActorCollision(*(*actor), *(*other)))
//			{
//				// Notify all scripts of actor
//				for (auto &script : whoCollided.first->getComponents<BehaviourScript>())
//					script.cast<BehaviourScript>()->onCollision(whoCollided.second->getHandle());
//
//				// Notify all scripts of other
//				for (auto &script : whoCollided.second->getComponents<BehaviourScript>())
//					script.cast<BehaviourScript>()->onCollision(whoCollided.first->getHandle());
//			}
//		}
//	}
//}