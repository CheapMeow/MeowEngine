#include "register_all.h"

#include "core/reflect/type_descriptor_builder.hpp"
#include "function/components/camera/camera_3d_component.h"

namespace Meow
{
	void RegisterAll()
	{
		reflect::AddClass<Camera3DComponent>("Camera3DComponent")
			.AddField("near_plane", &Camera3DComponent::near_plane)
			.AddField("far_plane", &Camera3DComponent::far_plane)
			.AddField("field_of_view", &Camera3DComponent::field_of_view)
			.AddField("camera_mode", &Camera3DComponent::camera_mode);

	}
} // namespace Meow
