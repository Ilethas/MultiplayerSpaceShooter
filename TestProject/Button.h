#ifndef BUTTON_H_
#define BUTTON_H_
#include <SFML/Graphics.hpp>
#include <functional>
#include "Tools.h"


class Button
{
	private:
		sf::Sprite sprite;
		sf::Vector2f dimensions;
		sf::RenderWindow *window;
		std::function<void()> onClick;

	public:
		Button() = default;
		Button(const sf::Texture &texture, float posX, float posY, float originX, float originY, float width, float height, sf::RenderWindow &window, std::function<void()> onClick = nullptr);

		void setTexture(const sf::Texture &texture);
		void setPosition(sf::Vector2f position);
		void setPosition(float x, float y);
		void setOrigin(sf::Vector2f origin);
		void setOrigin(float originX, float originY);
		void setWidth(float width);
		void setHeight(float height);
		void setSize(sf::Vector2f size);
		void setSize(float width, float height);
		void setOnClick(std::function<void()> onClick);
		void setWindow(sf::RenderWindow &window);

		const sf::Texture* getTexture() const;
		sf::Vector2f getPosition() const;
		sf::Vector2f getOrigin() const;
		float getWidth() const;
		float getHeight() const;
		sf::Vector2f getSize() const;
		std::function<void()> getOnClick() const;
		sf::RenderWindow* getWindow();

		bool isClicked(sf::Vector2i clickPoint) const;
		bool isClicked(int clickPointX, int clickPointY) const;
		void click();
		void draw();
};


#endif