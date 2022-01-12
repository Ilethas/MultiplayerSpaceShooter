#ifndef MESSAGE_HEADER_H_
#define MESSAGE_HEADER_H_


enum class MessageHeader : int
{
	THIS_PLAYER_ID,				// unsigned long id of the receiver player
	OTHER_PLAYER_ID,			// unsigned long id of a player who is not a receiver

	CREATE_MAP,					// unsigned int map seed
	CREATE_PLAYER_SHIP,			// float x, float y, float rotation, unsigned long playerId, null terminated name
	PLAYER_INPUT_EVENT,			// sf::Event event, unsigned long player ID
	SET_ROTATION,				// float angle, unsigned long actor ID
	SET_POSITION,				// sf::Vector2f position, unsigned long actor ID
	SET_VELOCITY,				// sf::Vector2f position, unsigned long actor ID
	DESTROY_ACTOR				// unsigned long actor ID
};


#endif
