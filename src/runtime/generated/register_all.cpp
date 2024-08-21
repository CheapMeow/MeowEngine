#include "register_all.h"

#include "core/reflect/type_descriptor_builder.hpp"
#include "function/components/camera/camera_3d_component.hpp"

namespace Meow
{
	void RegisterAll()
	{
		reflect::AddClass<Transform3DComponent>("Transform3DComponent")
			.AddField("position", "int", &Transform3DComponent::position)
			.AddField("rotation", "int", &Transform3DComponent::rotation)
			.AddField("scale", "int", &Transform3DComponent::scale);

		reflect::AddClass<Camera3DComponent>("Camera3DComponent")
			.AddField("near_plane", "float", &Camera3DComponent::near_plane)
			.AddField("far_plane", "float", &Camera3DComponent::far_plane)
			.AddField("field_of_view", "float", &Camera3DComponent::field_of_view)
			.AddField("camera_mode", "CameraMode", &Camera3DComponent::camera_mode);

	}
} // namespace Meow
