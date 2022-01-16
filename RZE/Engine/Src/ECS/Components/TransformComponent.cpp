#include <StdAfx.h>

#include <ECS/Components/TransformComponent.h>

#include <ImGui/imgui.h>

//void TransformComponent::OnEditorInspect(Apollo::EntityID entityID)
//{
//	float* positionValues = const_cast<float*>(&Position.GetInternalVec().x);
//	float* rotationValues = const_cast<float*>(&Rotation.GetInternalVec().x);
//	float* scaleValues = const_cast<float*>(&Scale.GetInternalVec().x);
//
//
//	// #TODO(Modify DragFloat3 to only register on mouse movement to avoid falloff when dragging)
//
//
//	ImGui::Text("Position");
//	ImGui::DragFloat3("##transfcomp_position", positionValues, 0.005f);
//	ImGui::Text("Rotation");
//	ImGui::DragFloat3("##transfcomp_rotation", rotationValues, 0.05f);
//	ImGui::Text("Scale");
//	ImGui::DragFloat3("##transfcomp_scale", scaleValues, 0.005f);
//}

//void TransformComponent::Load(const rapidjson::Value& data)
//{
//	Position = Vector3D(data["Position"][0].GetFloat(), data["Position"][1].GetFloat(), data["Position"][2].GetFloat());
//	Scale = Vector3D(data["Scale"][0].GetFloat(), data["Scale"][1].GetFloat(), data["Scale"][2].GetFloat());
//	Rotation = Vector3D(data["Rotation"][0].GetFloat(), data["Rotation"][1].GetFloat(), data["Rotation"][2].GetFloat());
//}

//void TransformComponent::Save(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
//{
//	writer.String("TransformComponent");
//	writer.StartObject();
//	{
//		{
//			writer.Key("Position");
//			writer.StartArray();
//			for (int i = 0; i < 3; ++i)
//			{
//				writer.Double(Position[i]);
//			}
//			writer.EndArray();
//		}
//		{
//			writer.Key("Scale");
//			writer.StartArray();
//			for (int i = 0; i < 3; ++i)
//			{
//				writer.Double(Scale[i]);
//			}
//			writer.EndArray();
//		}
//		{
//			writer.Key("Rotation");
//			writer.StartArray();
//			for (int i = 0; i < 3; ++i)
//			{
//				writer.Double(Rotation[i]);
//			}
//			writer.EndArray();
//		}
//	}
//	writer.EndObject();
//}
