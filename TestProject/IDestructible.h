#ifndef IDESTRUCTIBLE_H_
#define IDESTRUCTIBLE_H_


class IDestructible
{
	public:
		virtual ~IDestructible() = 0 {}
		virtual bool isDestroyed() const = 0;
		virtual void destroy() = 0;
};


#endif