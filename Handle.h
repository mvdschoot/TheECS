#ifndef __HANDLE_HEADER__ 
#define __HANDLE_HEADER__ 

#include "ComponentType.h"

#include <typeindex>

namespace Steve
{

	class IHandle {
		friend class Entity;
	public:
		// Copy constructor
		IHandle(const IHandle& handle) = default;

		// Move constructor
		IHandle(IHandle&& comp) = default;

		// Constructor
		IHandle(std::type_index type, usize size, const UUID& id = UUID()) : Type(type), Id(id), Size(size) { mOwners.reserve(3); }

		// Destructor IMPORTANT!
		virtual ~IHandle() = default;

		// Less operator for indexing in hash table
		friend bool operator<(const IHandle& l, const IHandle& r)
		{
			return l.Component->p_Id < r.Component->p_Id;
		}

		// Deref
		virtual ComponentType& operator->() {
			return *Component;
		}

		virtual ComponentType& operator*() {
			return *Component;
		}

		[[nodiscard]] virtual ComponentType* GetComponent() { return Component; }

		void Move(u8* location)
		{
			if (Component != nullptr)
			{
				memcpy_s(location, Size, Component, Size);
				Component = (ComponentType*)location;
			}
		}
		void SetLocation(u8* new_location)
		{
			Component = (ComponentType*)new_location;
		}

	public:
		const std::type_index Type;
		const UUID Id;
		ComponentType* Component = nullptr;
		const usize Size;


	// Private stuff for Registry
	private:
		// UUID's of entities
		std::vector<const UUID&> mOwners;
	};

	template<typename T>
	class Handle final : public IHandle
	{
	public:
		T& operator->() override
		{
			return *(T*)Component;
		}
	
		T& operator*() override
		{
			return *(T*)Component;
		}

		~Handle() override
		{
		}

		// Initializes base class
		Handle(const UUID id = UUID()) : IHandle(std::type_index(typeid(T)), sizeof(T), id) {}

		Handle(T& e) : Handle()
        {
            Component = e;
        }

		Handle(T* e) : Handle()
		{
			Component = e;
		}

		Handle(void* location, T&& e, const UUID id = UUID()) : Handle(id)
		{
			Component = new(location) T(e);
		}

		template<typename ...Args>
		Handle(void* location, Args&& ...args) : Handle()
		{
			Component = new(location) T(std::forward<Args>(args)...);
		}

		T* Get()
		{
			return (T*)Component;
		}
	};

	template<typename I>
	struct RemoveHandle
	{
		using type = I;
	};

	template<typename I>
	struct RemoveHandle<Handle<I>>
	{
		using type = typename RemoveHandle<I>::type;
	};
}


#endif
