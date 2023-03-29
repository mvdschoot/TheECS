#include "Registry.h"

#include "Entity.h"
#include "Steve/Core/KeyCodes.h"

namespace Steve
{

	UUID Registry::CreateEntity(const char* name)
	{
		Entity ent(name);
		mData.Entities.emplace(ent.Id, ent);
		return ent.Id;
	}

	Entity& Registry::GetEntity(const UUID& uuid)
	{
		return mData.Entities[uuid];
	}

	/**
	 * \brief Checks if type is already in group
	 * \param type Type_index object
	 * \return Returns an iterator to the std::pair of the found type, if it
	 * hasnt found it, it will point to mGrouptypes end()
	 */
	std::optional<const UUID&> Registry::IsInGroup(const std::type_index type)
	{
		auto it = mGroupTypes.begin();
		for(; it != mGroupTypes.end(); ++it)
		{
			auto it2 = it->second.begin();
			for(; it2 != it->second.end(); ++it2)
			{
				if (*it2 == type)
					return it->first;
			}
		}
		return {};
	}

	// Takes in temp handle and returns a definite one
	IHandle* Registry::AddComponent(Entity* entity, IHandle* component_handle)
	{
		CH_PROFILE_FUNCTION();
		CORE_ASSERT(mData.Entities.contains(entity->Id), "Entity does not exists so component cannot be added")

		// Inserts handle and gets pointer to it
		IHandle* new_handle = &mData.ComponentHandles.emplace(component_handle->Id, IHandle(component_handle->Type, component_handle->Size)).first->second;

		Entity& ent = mData.Entities[entity->Id];
		ent.AddedComponent(new_handle);

		const std::optional<const UUID&> group_id = IsInGroup(new_handle->Type);

		// If type not in a group or the entity does not have all types for the group
		if (!group_id.has_value() || !ent.ContainsAll(mGroupTypes.at(*group_id))) {
			mData.RestComponents.Insert(new_handle);
		} else {
			mGroups.at(*group_id)->InsertNew(*entity);
		}

		return new_handle;
	}

	void Registry::DestroyComponent(IHandle* component_handle)
	{
		// Not in group
		if (mData.RestComponents.Contains(component_handle.Id))
		{
			mData.RestComponents.Remove(component_handle.Id);
		}
		else  // Group
		{
			auto opt = IsInGroup(component_handle->Type);
			CORE_ASSERT(opt.has_value(), "Component is nowhere")

			mData.RestComponents.Insert();
		}

		mData.ComponentHandles.erase(component_handle.Id);
	}

}
