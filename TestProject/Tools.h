#ifndef TOOLS_H_
#define TOOLS_H_
#include <SFML/Graphics.hpp>
#include "Coroutine.h"


namespace Tools
{
	constexpr float pi = 3.141592653589793f;
	float rad2deg(float radians);
	float deg2rad(float degrees);

	template <typename T>
	T length(const sf::Vector2<T> &vector);

	template <typename T>
	T dot(const sf::Vector2<T> &v1, const sf::Vector2<T> &v2);

	template <typename T>
	sf::Vector2<T> rotate(const sf::Vector2<T> &vector, float degrees);

	template <typename T>
	sf::Vector2<T> rotateAroundPoint(const sf::Vector2<T> &vector, const sf::Vector2<T> &point, float degrees);

	template <typename T>
	sf::Vector2<T> ScaleVector(const sf::Vector2<T> &vector, const sf::Vector2<T> &scale);

	template <typename T>
	sf::Vector2<T> componentsReciprocal(const sf::Vector2<T> &vector);

	template <typename T>
	std::string toString(const sf::Vector2<T> &vector);

	template <typename T>
	class Lerp : public Coroutine
	{
		private:
			int state = 0;
			T &whatValue;
			T intensity = 0.0;
			T fadingSpeed = 1.0;
			float delay = 0.0f;

		public:
			Lerp(T &whatValue, T intensity, T fadingSpeed, float delay);

		protected:
			std::unique_ptr<CoroutineResultType> operator()() override;
	};



	template <typename T>
	T length(const sf::Vector2<T> &vector)
	{
		return std::sqrt(vector.x*vector.x + vector.y*vector.y);
	}


	template <typename T>
	T dot(const sf::Vector2<T> &v1, const sf::Vector2<T> &v2)
	{
		return v1.x*v2.x + v1.y*v2.y;
	}


	template <typename T>
	sf::Vector2<T> rotate(const sf::Vector2<T> &vector, float degrees)
	{
		// Rotates vector around the point (0, 0)
		float radAngle = deg2rad(degrees);
		return sf::Vector2<T>(
			vector.x*std::cos(radAngle) - vector.y*std::sin(radAngle),
			vector.x*std::sin(radAngle) + vector.y*std::cos(radAngle));
	}


	template <typename T>
	sf::Vector2<T> rotateAroundPoint(const sf::Vector2<T> &vector, const sf::Vector2<T> &point, float degrees)
	{
		// Rotates vector around an arbitrary point
		return rotate(vector - point, degrees) + point;
	}


	template <typename T>
	sf::Vector2<T> ScaleVector(const sf::Vector2<T> &vector, const sf::Vector2<T> &scale)
	{
		return sf::Vector2<T>(vector.x*scale.x, vector.y*scale.y);
	}



	template <typename T>
	sf::Vector2<T> componentsReciprocal(const sf::Vector2<T> &vector)
	{
		return sf::Vector2<T>(1 / vector.x, 1 / vector.y);
	}



	template <typename T>
	std::string toString(const sf::Vector2<T> &vector)
	{
		return "sf::Vector(" + std::to_string(vector.x) + ", " + std::to_string(vector.y) + ")";
	}


	template <typename T>
	Lerp<T>::Lerp(T &whatValue, T intensity, T fadingSpeed, float delay)
		: whatValue(whatValue)
		, intensity(intensity)
		, fadingSpeed(fadingSpeed)
		, delay(delay)
	{}


	template <typename T>
	std::unique_ptr<CoroutineResultType> Lerp<T>::operator()()
	{
		coroutineBegin();
		while (std::abs(whatValue - intensity) > 0.01)
		{
			whatValue = static_cast<T>(whatValue*(1.0f - fadingSpeed) + intensity*fadingSpeed);
			yieldReturn(mkUniq(WaitForSeconds(delay)));
		}
		std::cout << "Fading finished" << std::endl;
		coroutineEnd(mkUniq(CoroutineFinished()));
	}
};


#endif