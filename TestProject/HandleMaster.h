#ifndef HANDLE_MASTER_H_
#define HANDLE_MASTER_H_
#include <list>


template <typename T>
class HandleMaster;


// Each handle provides secure access to ONE resource.
template <typename T>
class Handle
{
	friend HandleMaster<T>;

	private:
		HandleMaster<T> *master = nullptr;
		typename std::list<Handle<T>*>::iterator self;	// points self in HandleMaster handleList
		T *resource = nullptr;

		Handle(HandleMaster<T> *master, T *resource);

	public:
		Handle() = default;
		Handle(const Handle &handle);
		Handle(Handle &&handle);
		Handle& operator=(const Handle &handle);
		Handle& operator=(Handle &&handle);
		~Handle();

		template <typename D>
		D* cast();

		T* get();
		const T* get() const;
		operator bool() const;
		T& operator*();
		const T& operator*() const;
		T* operator->();
		const T* operator->() const;
};


template <typename T>
using ConstHandle = Handle<const T>;


// Used to provide several handles to ONE resource
template <typename T>
class HandleMaster
{
	friend Handle<T>;

	private:
		std::list<Handle<T>*> handleList;
		T *resource = nullptr;

	public:

		HandleMaster() = default;
		HandleMaster(T *resource);
		HandleMaster(const HandleMaster&) = delete;
		HandleMaster& operator=(const HandleMaster&) = delete;
		~HandleMaster();

		Handle<T> createHandle();
};


template <typename T>
using ConstHandleMaster = HandleMaster<const T>;


//========================= Handle =========================
template <typename T>
Handle<T>::Handle(HandleMaster<T> *master, T *resource)
	: master(master)
	, resource(resource)
{
	master->handleList.push_back(this);
	self = std::prev(master->handleList.end());
}


template <typename T>
Handle<T>::Handle(const Handle &handle)
	: master(handle.master)
	, resource(handle.resource)
{
	if (master != nullptr)
	{
		master->handleList.push_back(this);
		self = std::prev(master->handleList.end());
	}
}


template <typename T>
Handle<T>::Handle(Handle &&handle)
{
	master = handle.master;
	resource = handle.resource;

	if (master != nullptr)
	{
		self = handle.self;
		*self = this;
	}

	handle.master = nullptr;
	handle.self = std::list<Handle<T>*>::iterator();
	handle.resource = nullptr;
}


template <typename T>
Handle<T>& Handle<T>::operator=(const Handle &handle)
{
	if (this == &handle)
		return *this;

	if (master != nullptr)
		master->handleList.erase(self);

	master = handle.master;
	resource = handle.resource;

	if (master != nullptr)
	{
		master->handleList.push_back(this);
		self = std::prev(master->handleList.end());
	}

	return *this;
}


template <typename T>
Handle<T>& Handle<T>::operator=(Handle &&handle)
{
	if (this == &handle)
		return *this;

	if (master != nullptr)
		master->handleList.erase(self);

	master = handle.master;
	resource = handle.resource;

	if (master != nullptr)
	{
		self = handle.self;
		*self = this;
	}

	handle.master = nullptr;
	handle.self = std::list<Handle<T>*>::iterator();
	handle.resource = nullptr;
	return *this;
}


template <typename T>
Handle<T>::~Handle()
{
	if (master != nullptr)
		master->handleList.erase(self);
}


template <typename T>
T* Handle<T>::get()
{
	return resource;
}


template <typename T>
const T* Handle<T>::get() const
{
	return resource;
}


template <typename T>
Handle<T>::operator bool() const
{
	return resource != nullptr;
}


template <typename T>
T& Handle<T>::operator*()
{
	return *resource;
}


template <typename T>
const T& Handle<T>::operator*() const
{
	return *resource;
}


template <typename T>
template <typename D>
D* Handle<T>::cast()
{
	static_assert(std::is_base_of<T, D>::value, "Handle<T>::cast<D>() T template parameter must be a base class of D.");
	return static_cast<D*>(resource);
}


template <typename T>
T* Handle<T>::operator->()
{
	return resource;
}


template <typename T>
const T* Handle<T>::operator->() const
{
	return resource;
}


//========================= HandleMaster =========================
template <typename T>
HandleMaster<T>::HandleMaster(T *resource)
	: resource(resource)
{}


template <typename T>
HandleMaster<T>::~HandleMaster()
{
	for (auto &i : handleList)
	{
		i->master = nullptr;
		i->self = std::list<Handle<T>*>::iterator();
		i->resource = nullptr;
	}
}


template <typename T>
Handle<T> HandleMaster<T>::createHandle()
{
	return Handle<T>(this, resource);
}


#endif