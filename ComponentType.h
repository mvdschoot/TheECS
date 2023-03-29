#ifndef __CORE_COMPONENT_HEADER__
#define __CORE_COMPONENT_HEADER__

#include <typeindex>

#include "Steve/Core/UUID.h"

#include "Steve/Renderer/Texture.h" 

#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Steve
{
	struct ComponentType 
	{
		virtual ~ComponentType() = default;
		UUID p_Id;

		virtual bool operator==(const ComponentType& x) const
		{
			return p_Id == x.p_Id;
		}

	protected:
		ComponentType() = default;
	};
	
	struct EmptyComponent : ComponentType{};
	const std::type_index EmptyComponentType(typeid(EmptyComponent));



}

#endif

