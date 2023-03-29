#ifndef _CUSTOMSTORAGE_HEADER__
#define _CUSTOMSTORAGE_HEADER__

#include <map>
#include <ranges>
#include <cstdarg>

#include "Steve/Core/UUID.h"
#include "Handle.h"

namespace Steve
{
	class ArenaContainer
	{
	public:
		ArenaContainer(size_t initial_size = 1024, float hole_threshold = 0.1f) : mStorageSize(initial_size), mFragThreshold(hole_threshold), mFragHoleSize(0)
		{
			mStorage = (u8*)calloc(mStorageSize, 1);
			mStorageFreePtr = mStorage;
			mStorageContent.reserve(mStorageSize);
		}

		~ArenaContainer()
		{
			free(mStorage);
		}

		// Gets ownership of element
		template<typename T>
		T* Insert(const UUID& uuid, T&& element)
		{
			CH_PROFILE_FUNCTION();
			if (mStorageFreePtr + sizeof(T) >= mStorage + mStorageSize)
				Resize();

			size_t size = sizeof(T);
			T* ptr = new(mStorageFreePtr) T(element);

			mStorageContent.insert({ uuid, {(u8*)ptr, size } });
			mStorageFreePtr += size;

			return ptr;
		}

		// Gets ownership of object
		u8* InsertExternal(const UUID& uuid, usize size)
		{
			CH_PROFILE_FUNCTION();
			if (mStorageFreePtr + size >= mStorage + mStorageSize)
				Resize();

			mStorageContent.insert({ uuid, { mStorageFreePtr, size } });
			mStorageFreePtr += size;

			return mStorageFreePtr - size;
		}

		// Calling function takes ownership
		template<typename T>
		T&& Remove(const UUID& element)
		{
			CH_PROFILE_FUNCTION();
			const auto it = mStorageContent.find(element);
			T&& to_return = std::move(*static_cast<T*>(it->second.first));
			mStorageContent.erase(it);

			mFragHoleSize += sizeof(T);

			return to_return;
		}

		void Remove(const UUID& element)
		{
			CH_PROFILE_FUNCTION();
			const auto it = mStorageContent.find(element);

			if (it->second.first + it->second.second == mStorageFreePtr)
				mStorageFreePtr -= it->second.second;
			else
				mFragHoleSize += it->second.second;
			
			mStorageContent.erase(it);
		}

		// Returns new location of each component
		// Memory has already been moved
		std::unordered_map<UUID, u8*> Defragment()
		{
			std::unordered_map<UUID, u8*> res;

			u8* new_storage = (u8*)calloc(mStorageSize, 1);
			u8* new_storage_ptr = new_storage;

			for (auto& data : mStorageContent)
			{
				memcpy_s(new_storage_ptr, data.second.second, data.second.first, data.second.second);
				data.second.first = new_storage_ptr;
				new_storage_ptr += data.second.second;

				res.emplace(data.first, data.second.first);
			}

			mFragHoleSize = 0;

			free(mStorage);
			mStorage = new_storage;
			mStorageFreePtr = new_storage_ptr;

			return res;
		}
		std::optional<std::unordered_map<UUID, u8*>> DefragmentIfNeeded()
		{
			if ((float)mFragHoleSize / (float)mStorageSize > mFragThreshold)
			{
				return { Defragment() };
			}
			return {};
		}

		[[nodiscard]] const u8* GetRaw() const { return mStorage; }
		[[nodiscard]] std::unordered_map<UUID, std::pair<u8*, usize>> GetContent() const { return mStorageContent; }
		[[nodiscard]] usize GetSize() const
		{
			usize total_size = 0;
			for(auto& [ptr, size] : mStorageContent | std::ranges::views::values)
			{
				total_size += size;
			}
			return total_size;
		}

	protected:
		void Resize()
		{
			mStorageSize *= 2;

			u8* new_storage = (u8*)realloc(mStorage, mStorageSize);
			const long long diff = new_storage - mStorage;
			mStorage = new_storage;

			mStorageFreePtr += diff;
			for (auto& data : mStorageContent | std::views::values)
			{
				data.first += diff;
			}

			mStorageContent.reserve(mStorageSize / 2);
		}

		u8* mStorage;
		u8* mStorageFreePtr;
		size_t mStorageSize;

		std::unordered_map<const UUID&, std::pair<u8*, usize>> mStorageContent;

		float mFragThreshold;
		size_t mFragHoleSize;
	};

	// ArenaContainer + Component handles
	class ComponentContainer : protected ArenaContainer
	{
	public:
		ComponentContainer(size_t initial_size = 1024, float hole_threshold = 0.10f) : ArenaContainer(initial_size, hole_threshold) {}

		template<typename T> Handle<T>&& Insert(T&& component)
		{
			UUID uuid;
			return Handle<T>(InsertExternal(uuid, sizeof(T)), component, uuid);
		}

		template<typename ...T>
		void Insert(T&& ...handles)
		{
			([&](auto&& handle)
				{
					using HandleType = std::decay_t<decltype(handle)>;
					static_assert(std::is_base_of_v<IHandle, HandleType>, "All inputs should be IHandle's");
					mHandles.emplace(handle->Id, std::forward<HandleType>(handle));
					handle->Move(InsertExternal(handle->Id, handle->Size));
				}(std::forward<T>(handles)), ...);
		}

		template<typename T> void Insert(Handle<T>& component_handle)
		{
			Insert((IHandle*)&component_handle);
		}

		void Remove(const UUID& component_uuid)
		{
			DefragmentIfNeeded()
			mHandles.erase(component_uuid);
			ArenaContainer::Remove(component_uuid);
		}
		void Remove(const IHandle* component_handle)
		{
			mHandles.erase(component_handle->Id);
			ArenaContainer::Remove(component_handle->Id);
		}
		template<typename T> T&& Remove(UUID& component_uuid)
		{
			mHandles.erase(component_uuid);
			return ArenaContainer::Remove<T>(component_uuid);
		}
		template<typename T> T&& Remove(Handle<T>& component_handle)
		{
			mHandles.erase(component_handle.Id);
			return ArenaContainer::Remove(component_handle.Id);
		}

		[[nodiscard]] bool Contains(const UUID& component_uuid) const { return mHandles.contains(component_uuid); }

		void Defragment()
		{
			std::unordered_map<UUID, u8*> defrag = ArenaContainer::Defragment();
			for (const auto& [id, loc] : defrag)
			{
				mHandles.at(id)->SetLocation(loc);
			}
		}
		void DefragmentIfNeeded()
		{
			if(mFragHoleSize > mStorageSize * mFragThreshold)
			{
				Defragment();
			}
		}
	private:
		// uuid is member of IHandle
		std::unordered_map<const UUID&, IHandle*> mHandles;
	};

	// Sequential storage for sets of types
	template<typename ...Ts>
	class TupleComponentContainer : public ArenaContainer
	{
	public:
		using TypeInTuple = std::tuple<Ts...>;
		size_t TotalSize = 0;

		// initial_size in #Tuples
		TupleComponentContainer(size_t initial_size = 10, float hole_threshold = 0.1f)
		{
			(void(TotalSize += sizeof(Ts)), ...);
			ArenaContainer(initial_size * TotalSize, hole_threshold);
		}

		UUID Insert(Handle<Ts>&... handles)
		{
			UUID id;
			mEntities.emplace(id, std::make_tuple(handles...));
			(void(handles.Move(InsertExternal(handles.Id, handles.Size))), ...);
			return id;
		}

		void Remove(UUID& entity_uuid)
		{
			CORE_ASSERT(mEntities.contains(entity_uuid), "Does not have this entity")

			std::tuple<Handle<Ts>&...>& handles = mEntities[entity_uuid];
			ApplyToTuple(handles, [&](auto handle)
				{
					ArenaContainer::Remove(handle.Id);
				});
			mEntities.erase(entity_uuid);
		}

		void Move(UUID& entity_uuid, u8* location)
		{
			CORE_ASSERT(mEntities.contains(entity_uuid), "Does not have this entity")

			std::tuple<Handle<Ts>&...>& handles = mEntities[entity_uuid];
			ApplyToTuple(handles, [&](auto& handle)
				{
					handle.Move(location);
					ArenaContainer::Remove(handle.Id);
				});
			mEntities.erase(entity_uuid);
		}
		
	private:
		std::unordered_map<UUID, std::tuple<Handle<Ts>&...>> mEntities;
	};

	// Not completely efficient
	// It could be more array focused
	template<typename T>
	class VectorComponentContainer : protected ComponentContainer
	{
	public:
		// initial_size in #elements
		VectorComponentContainer(usize initial_size) : ComponentContainer(initial_size * sizeof(T)) {}

		void Insert(Handle<T>& handle)
		{
			CH_PROFILE_FUNCTION();
			mHandles.emplace(handle.Id, handle);
			handle.Move(InsertExternal(handle.Id, handle.Size));
		}

		T&& Remove(const UUID& element)
		{
			CH_PROFILE_FUNCTION();
			mHandles.erase(element);
			return ArenaContainer::Remove<T>(element);
		}

		T&& Remove(Handle<T>& handle)
		{
			CH_PROFILE_FUNCTION();
			mHandles.erase(handle.Id);
			return ArenaContainer::Remove<T>(handle.Id);
		}

		[[nodiscard]] inline T* GetData() const { return (T*)GetRaw(); }
		 
		// Returns the number of T's in storage
		[[nodiscard]] inline usize GetSize() const { return mStorageContent.size(); }

	private:
		std::unordered_map<const UUID&, Handle<T>&> mHandles;
	};

	/// class GroupContainer in Group.h
}

#endif
