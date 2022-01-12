#include "Button.h"


Button::Button(const sf::Texture &texture, float posX, float posY, float originX, float originY, float width, float height, sf::RenderWindow &window, std::function<void()> onClick)
	: dimensions(width, height)
	, window(&window)
{
	auto textureSize = texture.getSize();

	setPosition(posX, posY);
	setTexture(texture);
	setSize(width, height);
	setOrigin(originX, originY);
	this->onClick = onClick;
}


void Button::setTexture(const sf::Texture &texture)
{
	sprite.setTexture(texture);
}


void Button::setPosition(sf::Vector2f position)
{
	sprite.setPosition(position);
}


void Button::setPosition(float x, float y)
{
	sprite.setPosition(x, y);
}


void Button::setOrigin(sf::Vector2f origin)
{
	sprite.setOrigin(origin);
}


void Button::setOrigin(float originX, float originY)
{
	sprite.setOrigin(originX, originY);
}


void Button::setWidth(float width)
{
	setSize(sf::Vector2f(width, dimensions.y));
}


void Button::setHeight(float height)
{
	setSize(sf::Vector2f(dimensions.x, height));
}


void Button::setSize(sf::Vector2f size)
{
	dimensions = size;

	const sf::Texture *texture = sprite.getTexture();
	if (texture != nullptr)
	{
		auto textureSize = texture->getSize();

		if (size.x == 0)
			dimensions.x = static_cast<float>(textureSize.x);
		if (size.y == 0)
			dimensions.y = static_cast<float>(textureSize.y);

		sprite.setScale(dimensions.x / static_cast<float>(textureSize.x), dimensions.y / static_cast<float>(textureSize.y));
	}
}


void Button::setSize(float width, float height)
{
	setSize(sf::Vector2f(width, height));
}


void Button::setOnClick(std::function<void()> onClick)
{
	this->onClick = onClick;
}


void Button::setWindow(sf::RenderWindow &window)
{
	this->window = &window;
}


const sf::Texture* Button::getTexture() const
{
	return sprite.getTexture();
}


sf::Vector2f Button::getPosition() const
{
	return sprite.getPosition();
}


sf::Vector2f Button::getOrigin() const
{
	return sprite.getOrigin();
}


float Button::getWidth() const
{
	return dimensions.x;
}


float Button::getHeight() const
{
	return dimensions.y;
}


sf::Vector2f Button::getSize() const
{
	return dimensions;
}


std::function<void()> Button::getOnClick() const
{
	return onClick;
}


sf::RenderWindow* Button::getWindow()
{
	return window;
}


bool Button::isClicked(sf::Vector2i clickPoint) const
{
	if (window == nullptr)
		return false;

	sf::Vector2f offsetedPosition = sf::Vector2f(sprite.getPosition() - Tools::ScaleVector(sprite.getOrigin(), sprite.getScale()));
	sf::Vector2f clickPointFloat(clickPoint);

	if (clickPointFloat.x > offsetedPosition.x && clickPointFloat.x < offsetedPosition.x + dimensions.x &&
		clickPointFloat.y > offsetedPosition.y && clickPointFloat.y < offsetedPosition.y + dimensions.y)
		return true;
	else
		return false;
}


bool Button::isClicked(int clickPointX, int clickPointY) const
{
	return isClicked(sf::Vector2i(clickPointX, clickPointY));
}


void Button::click()
{
	if (onClick != nullptr)
		onClick();
}


void Button::draw()
{
	if (window == nullptr)
		return;

	sf::View previousView = window->getView();
	window->setView(sf::View(sf::FloatRect(sf::Vector2f(), sf::Vector2f(window->getSize().x, window->getSize().y))));

	window->draw(sprite);

	window->setView(previousView);
}