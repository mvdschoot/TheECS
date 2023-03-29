#ifndef GROUP_HEADER_
#define GROUP_HEADER_

#include "Steve/Core/Core.h"
#include "Steve/Core/Logger.h"
#include "Steve/Core/Profiling.h"
#include "Steve/Core/UUID.h"

#include "Entity.h"
#include "Handle.h"
#include "Containers.h"
#include "RegistryData.h"
#include "View.h"

#include <tuple>
#include <typeindex>
#include <vector>

namespace Steve
{
	class IGroup
	{
	public:
		virtual void SetAll() = 0;
		virtual void InsertNew(Entity& entity) = 0;
		virtual void Move(Entity& entity, u8* location) = 0;
	};


	template<typename ...Ts>
	class Group : public IGroup
	{
		using TupleType = std::tuple<Ts...>;

	public:
		Group(RegistryData* reg_data)
		{
			CH_PROFILE_FUNCTION();

			mRegData = reg_data;
			// pType = std::type_index(typeid(Group<Ts...>));

			int i = 0;
			([&]()
				{
					std::type_index type = std::type_index(typeid(Ts));
					for (int x = 0; x < i; x++)
					{
						CORE_ASSERT(mComponentTypes[x] != type, "Group cannot have multiple of the same type")
					}
					mComponentTypes[i++] = std::type_index(typeid(Ts));
				}, ...);
		}
		
		~Group() 
		{
		}

		void SetAll() override
		{
			CH_PROFILE_FUNCTION();

			View<Ts...> view;
			for (std::tuple<Handle<Ts>&...>& tup : view) {
				std::apply(mStorage.Insert, tup);
			}
		}

		void InsertNew(Entity& entity) override
		{
			CH_PROFILE_FUNCTION();

			CORE_ASSERT(entity.ContainsAll<Ts...>(), "Entity does not contain all elements")
			std::apply(mStorage.Insert, entity.GetComponents<Ts...>());
		}

		void Move(Entity& entity, u8* location) override
		{
			u8* ptr = location;
			ApplyTuple(entity.GetComponents<Ts...>(), [&](auto& handle)
				{
					handle.Move(ptr);
					ptr += handle.Size;
					mStorage.Remove(handle.Id);
				});
		}

		[[nodiscard]] const u8* GetRaw() const 
		{
			return mStorage.GetRaw();
		}
	private:
		RegistryData* mRegData;

		const std::type_index mComponentTypes[sizeof...(Ts)];
		TupleComponentContainer<Ts...> mStorage;
	};


}

#endif
