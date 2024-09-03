#include "register_all.h"

#include "core/reflect/type_descriptor_builder.hpp"
#include "function/components/camera/camera_3d_component.hpp"
#include "function/components/model/model_component.h"

namespace Meow
{
	void RegisterAll()
	{
		reflect::AddClass<Transform3DComponent>("Transform3DComponent")
			.AddField("position", "glm::vec3", &Transform3DComponent::position)
			.AddField("rotation", "glm::quat", &Transform3DComponent::rotation)
			.AddField("scale", "glm::vec3", &Transform3DComponent::scale);

		reflect::AddClass<Camera3DComponent>("Camera3DComponent")
			.AddField("field_of_view", "float", &Camera3DComponent::field_of_view)
			.AddField("aspect_ratio", "float", &Camera3DComponent::aspect_ratio)
			.AddField("near_plane", "float", &Camera3DComponent::near_plane)
			.AddField("far_plane", "float", &Camera3DComponent::far_plane)
			.AddField("camera_mode", "CameraMode", &Camera3DComponent::camera_mode);

		reflect::AddClass<ModelComponent>("ModelComponent")
			.AddField("m_image_paths", "std::vector<std::string>", &ModelComponent::m_image_paths);
	}

	VertexAttributeBit to_enum(const std::string& str)
	{
		if (str == "None")
			return VertexAttributeBit::None;
		if (str == "Position")
			return VertexAttributeBit::Position;
		if (str == "UV0")
			return VertexAttributeBit::UV0;
		if (str == "UV1")
			return VertexAttributeBit::UV1;
		if (str == "Normal")
			return VertexAttributeBit::Normal;
		if (str == "Tangent")
			return VertexAttributeBit::Tangent;
		if (str == "Color")
			return VertexAttributeBit::Color;
		if (str == "SkinWeight")
			return VertexAttributeBit::SkinWeight;
		if (str == "SkinIndex")
			return VertexAttributeBit::SkinIndex;
		if (str == "SkinPack")
			return VertexAttributeBit::SkinPack;
		if (str == "InstanceFloat1")
			return VertexAttributeBit::InstanceFloat1;
		if (str == "InstanceFloat2")
			return VertexAttributeBit::InstanceFloat2;
		if (str == "InstanceFloat3")
			return VertexAttributeBit::InstanceFloat3;
		if (str == "InstanceFloat4")
			return VertexAttributeBit::InstanceFloat4;
		if (str == "Custom0")
			return VertexAttributeBit::Custom0;
		if (str == "Custom1")
			return VertexAttributeBit::Custom1;
		if (str == "Custom2")
			return VertexAttributeBit::Custom2;
		if (str == "Custom3")
			return VertexAttributeBit::Custom3;
		if (str == "ALL")
			return VertexAttributeBit::ALL;

		return VertexAttributeBit::None;
	}

	const std::string to_string(VertexAttributeBit enum_val)
	{
		switch (enum_val)
		{
			case VertexAttributeBit::None:
				return "None";
			case VertexAttributeBit::Position:
				return "Position";
			case VertexAttributeBit::UV0:
				return "UV0";
			case VertexAttributeBit::UV1:
				return "UV1";
			case VertexAttributeBit::Normal:
				return "Normal";
			case VertexAttributeBit::Tangent:
				return "Tangent";
			case VertexAttributeBit::Color:
				return "Color";
			case VertexAttributeBit::SkinWeight:
				return "SkinWeight";
			case VertexAttributeBit::SkinIndex:
				return "SkinIndex";
			case VertexAttributeBit::SkinPack:
				return "SkinPack";
			case VertexAttributeBit::InstanceFloat1:
				return "InstanceFloat1";
			case VertexAttributeBit::InstanceFloat2:
				return "InstanceFloat2";
			case VertexAttributeBit::InstanceFloat3:
				return "InstanceFloat3";
			case VertexAttributeBit::InstanceFloat4:
				return "InstanceFloat4";
			case VertexAttributeBit::Custom0:
				return "Custom0";
			case VertexAttributeBit::Custom1:
				return "Custom1";
			case VertexAttributeBit::Custom2:
				return "Custom2";
			case VertexAttributeBit::Custom3:
				return "Custom3";
			case VertexAttributeBit::ALL:
				return "ALL";
			default:
				return "Unknown";
		}
	}

} // namespace Meow
