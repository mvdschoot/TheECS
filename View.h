#ifndef __VIEW_HEADER__
#define __VIEW_HEADER__

#include "Steve/Core/Core.h"
#include "Steve/Core/Logger.h"
#include "Steve/Core/Profiling.h"

#include "Entity.h"
#include "Handle.h"
#include "RegistryData.h"

#include <vector>
#include <set>
#include <unordered_map>

namespace Steve
{
	template<typename ...Ts>
	class View
	{
	public:
		using EntityIterator = std::unordered_map<UUID, Entity>::iterator;

		struct Iterator		// ITERATOR
		{
			friend class View;

			[[nodiscard]] Entity* GetEntity() const
			{
				return &p->second;
			}

			//Gets components on the fly, so addr's are always good
			template<typename Indices = std::make_index_sequence<sizeof...(Ts)>>
			std::tuple<Handle<Ts>&...> operator*()
			{
				CH_PROFILE_FUNCTION();
				return p->second.GetComponents<Ts...>();
			}
			std::tuple<Handle<Ts>&...> operator->() { return operator*(); }

			bool operator != (const Iterator& rhs) const {
				return p != rhs.p;
			}
			bool operator != (const EntityIterator& rhs) const
			{
				return p != rhs;
			}

			Iterator& operator++()
			{
				++p;
				while (p != mRegData->Entities.end() && !(
															p->second.ContainsAll<Ts...>() &&
															(!mFilter.has_value() || (*mFilter)(&p->second)))
					) ++p;
				return *this;
			}

			Iterator operator++(int)
			{
				Iterator tmp = *this;
				operator++();
				return tmp;
			}

			Iterator end()
			{
				auto ret = mRegData->Entities.end();
				return Iterator(ret);
			}

		// private constructor
		private:
			Iterator(RegistryData* reg_data, EntityIterator start = reg_data->Entities.begin(), std::function<bool(Entity*)>&& filter = nullptr)
				: mRegData(reg_data), p(start), mFilter(filter) {}
			
		private:
			RegistryData* mRegData;
			EntityIterator p;
			std::optional<std::function<bool(Entity*)>> mFilter;
		};	// ITERATOR

		View(RegistryData* reg_data)
		{
			mRegData = reg_data;
			if (mTypes.empty())
				(void(mTypes.push_back(std::type_index(typeid(Ts)))), ...);
		}

		inline Iterator begin()
		{
			return Iterator();
		}

		inline Iterator end()
		{
			return Iterator(mRegData->Entities.end());
		}

		inline Iterator FilterBy(std::function<bool(Entity*)>&& filter_function)
		{
			return Iterator(std::forward< std::function<bool(Entity*)> && >(filter_function));
		}
	private:
		std::vector<std::type_index> mTypes;
		RegistryData* mRegData;
	};
}

#endif