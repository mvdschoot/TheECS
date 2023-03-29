#ifndef SIZEDB_HEADER_
#define SIZEDB_HEADER_

#include <unordered_map>
#include <typeindex>

#include "Containers.h"
#include "Entity.h"
#include "Steve/Core/UUID.h"

namespace Steve
{
	struct RegistryData
	{
		std::unordered_map<UUID, IHandle> ComponentHandles;
		std::unordered_map<UUID, Entity> Entities;
		ComponentContainer RestComponents;
	};
}

#endif // SIZEDB_HEADER_
