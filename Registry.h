#ifndef __REGISTRY_HEADER__
#define __REGISTRY_HEADER__

#include "Steve/Core/Core.h"
#include "Steve/Core/Logger.h"
#include "Steve/Core/Profiling.h"

#include "Group.h"
#include "Handle.h"
#include "Containers.h"
#include "RegistryData.h"
#include "View.h"

#include <string>
#include <set>
#include <vector>
#include <map>
#include <typeinfo>
#include <typeindex>
#include <utility>


namespace Steve
{
    class Entity;

	class Registry
	{
	#define GROUPTYPES_IT std::map<const UUID&, std::vector<std::type_index>>::iterator
	#define GROUPTYPES_END std::end(mGroupTypes)

        friend class Scene;
	private:
		Registry() {}
        Registry(RegistryData&& reg_data) : mData(reg_data) {}
		~Registry() {}
        
        std::optional<const UUID&> IsInGroup(std::type_index type);

		template<typename ...Ts>
        void GroupComponents()
        {
            CH_PROFILE_FUNCTION();

            // Check if any component already in a group
            bool isInGroup = false;
            (void(isInGroup ? isInGroup : IsInGroup(std::type_index(typeid(Ts))).has_value()), ...);
            CORE_ASSERT(!isInGroup, "Component already in group")


            Group<Ts...> group(&mData);
            group.SetAll();

            // At this point, group has copy's of the object
            // Next remove them from the old containers

            CH_PROFILE_FUNCTION();
            View<Ts...> view;
            for (std::tuple<Handle<Ts>&...>& tup : view) {
                std::apply(mData.RestComponents.Remove, tup.Id);
            }
        }

		template<typename T>
		Handle<T>& AddComponent(Entity* entity, T&& component)
        {
            CORE_ASSERT(mData.Entities.contains(entity->Id), "Entity does not exists so component cannot be added")
                
            Handle<T> handle(component);
            IHandle* new_handle = AddComponent(entity, (IHandle*)&handle);
            entity->AddedComponent(new_handle);

            return (Handle<T>&)*new_handle;
        }

        // Invalidates all handle references
        template<typename T>
        void DestroyComponent(Handle<T>& component_handle)
		{
            DestroyComponent((IHandle*)&component_handle);
		}
        
        Entity& GetEntity(const UUID& id)
		{
            return mData.Entities[id];
		}

        template<typename ...Ts>
        [[nodiscard]] View<Ts...> GetView()
		{
            return View<Ts...>(mData);
		}

	private:
        // Untemplated logic implementation, so Entity can also access this;
        IHandle* AddComponent(Entity* entity, IHandle* component_handle);
        void DestroyComponent(IHandle* component_handle);

	private:
		std::map<const UUID&, std::vector<std::type_index>> mGroupTypes;
		std::map<const UUID&, IGroup*> mGroups;

        RegistryData mData;
	};
}


#endif

