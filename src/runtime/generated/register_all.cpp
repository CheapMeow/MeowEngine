#include "register_all.h"

#include "core/reflect/type_descriptor_builder.hpp"
#include "function/camera/camera_system.h"

namespace Meow
{
	void RegisterAll()
	{
		reflect::AddClass<Camera3DComponent>("Camera3DComponent")
			.AddField("is_main_camera", &Camera3DComponent::is_main_camera)
			.AddField("near_plane", &Camera3DComponent::near_plane)
			.AddField("far_plane", &Camera3DComponent::far_plane)
			.AddField("field_of_view", &Camera3DComponent::field_of_view);

	}
} // namespace Meow
