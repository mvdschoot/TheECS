#ifndef ENTITY_HEADER_
#define ENTITY_HEADER_

#include "Steve/Core/Core.h"
#include "Steve/Core/UUID.h"
#include "Steve/Core/Logger.h"
#include "Steve/Core/Profiling.h"

#include "Registry.h"
#include "Handle.h"

#include <set>
#include <map>
#include <tuple>
#include <typeindex>
#include <type_traits>
#include <utility>

namespace Steve
{

	// Only contains 1 of a component type
	class Entity
	{
		friend class Registry;

	public:

		Entity(Registry* registry) : mRegistry(registry) {}
		~Entity() = default;

		bool operator==(const Entity& other){return Id == other.Id;}

		
		template<typename T>
		[[nodiscard]] Handle<T>& GetComponent()
		{
			CH_PROFILE_FUNCTION();
			const std::type_index type(typeid(T));
			CORE_ASSERT(mComponentIds.contains(type), "Component with this type does not exist")

			return &(Handle<T>*)mComponentHandles.at(mComponentIds[type]);
		}

		template<typename ...Ts>
		[[nodiscard]] std::tuple<Handle<Ts>&...> GetComponents()
		{
			std::tuple<Handle<Ts>&...> res;
			ApplyToTuple(res, [&, this]<typename T>(T& handle)
				{
					handle = GetComponent<typename RemoveHandle<T>::type>();
				});
			return res;
		}

		// Checks if any of the types are here
		template<typename ...T>
		[[nodiscard]] bool ContainsAll() const
		{
			CH_PROFILE_FUNCTION();
			
			([&, this]<typename T>() {
				if (!Contains(std::type_index(typeid(T)))) return false;
			}, ...);

			return true;
		}
		[[nodiscard]] bool ContainsAll(std::vector<std::type_index>& types) const
		{
			for(auto& type : types)
			{
				if (!Contains(type)) return false;
			}
			return true;
		}

		template<typename T>
		[[nodiscard]] bool Contains() const
		{
			return mComponentIds.contains(std::type_index(typeid(T)));
		}
		[[nodiscard]] bool Contains(std::type_index& type) const
		{
			return mComponentIds.contains(type);
		}

		// Shortcut to registry function
		template<typename T>
		Handle<T>& AddComponent(T&& component) { return mRegistry->AddComponent(this, component); }

		// Shortcut to registry function
		template<typename T>
		void DestroyComponent() { mRegistry->DestroyComponent(mComponentHandles[mComponentIds[std::type_index(typeid(T))]]); }


		void AddChildEntity(Entity* entity)
		{
			mChildren.emplace(entity->Id, entity);
		}
		void RemoveChildEntity(Entity* entity)
		{
			mChildren.erase(entity->Id);
		}
		std::map<const UUID&, Entity*>& GetChildren() { return mChildren; }

	private:
		void AddedComponent(IHandle* handle)
		{
			mComponentIds.emplace(handle->Type, handle->Id);
			mComponentHandles.emplace(handle->Id, handle);

			handle->mOwners.push_back(Id);
		}
		
		void RemovedComponent(IHandle* handle)
		{
			CORE_ASSERT(mComponentIds.contains(handle->Type), "component not in mComponentIds")
			CORE_ASSERT(mComponentHandles.contains(handle->Id), "component not in mComponentHandles")

			mComponentIds.erase(handle->Type);
			mComponentHandles.erase(handle->Id);

			handle->mOwners.erase(Id);
		}

	public:
		const UUID Id;
	private:
		std::unordered_map<std::type_index, const UUID&> mComponentIds;
		std::unordered_map<const UUID&, IHandle*> mComponentHandles;
		std::map<const UUID&, Entity*> mChildren;

		Registry* mRegistry;
	};
}


#endif

