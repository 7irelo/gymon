#include "gypch.h"
#include "Gymon/Editor/SceneHierarchyPanel.h"

#include <imgui.h>

namespace Gymon {

	namespace {

		const char* TypeLabel(EntityType type)
		{
			switch (type)
			{
				case EntityType::Empty:            return "Empty";
				case EntityType::Mesh:             return "Mesh";
				case EntityType::Camera:           return "Camera";
				case EntityType::DirectionalLight: return "Directional Light";
			}
			return "Unknown";
		}

		// A labelled row of three drag floats, with a reset button per axis.
		// Kept local to the panel because it is editor chrome, not engine API.
		void DrawVec3Control(const char* label, glm::vec3& values, float resetValue = 0.0f, float speed = 0.1f)
		{
			ImGui::PushID(label);

			ImGui::Columns(2, nullptr, false);
			ImGui::SetColumnWidth(0, 90.0f);
			ImGui::Text("%s", label);
			ImGui::NextColumn();

			const float width = ImGui::GetContentRegionAvail().x / 3.0f;
			const char* axes[] = { "X", "Y", "Z" };
			float* components[] = { &values.x, &values.y, &values.z };

			for (int i = 0; i < 3; i++)
			{
				ImGui::PushID(i);
				ImGui::SetNextItemWidth(width - 24.0f);
				ImGui::DragFloat("##v", components[i], speed, 0.0f, 0.0f, "%.2f");
				ImGui::SameLine();
				if (ImGui::SmallButton(axes[i]))
					*components[i] = resetValue;
				if (i < 2)
					ImGui::SameLine();
				ImGui::PopID();
			}

			ImGui::Columns(1);
			ImGui::PopID();
		}
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& scene)
	{
		m_Scene = scene;
		m_SelectedID = 0;
	}

	Ref<Entity> SceneHierarchyPanel::GetSelected() const
	{
		if (!m_Scene || m_SelectedID == 0)
			return nullptr;
		return m_Scene->FindEntity(m_SelectedID);
	}

	void SceneHierarchyPanel::SetSelected(const Ref<Entity>& entity)
	{
		m_SelectedID = entity ? entity->GetID() : 0;
	}

	void SceneHierarchyPanel::DrawEntityNode(const Ref<Entity>& entity)
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_Leaf;

		if (entity->GetID() == m_SelectedID)
			flags |= ImGuiTreeNodeFlags_Selected;

		const bool opened = ImGui::TreeNodeEx(
			reinterpret_cast<void*>(static_cast<uintptr_t>(entity->GetID())),
			flags, "%s", entity->Name.c_str());

		if (ImGui::IsItemClicked())
			m_SelectedID = entity->GetID();

		bool destroy = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete Entity"))
				destroy = true;
			ImGui::EndPopup();
		}

		if (opened)
			ImGui::TreePop();

		// Deferred: destroying inside the tree walk would invalidate the
		// iteration in the caller.
		if (destroy)
		{
			if (m_SelectedID == entity->GetID())
				m_SelectedID = 0;
			m_Scene->DestroyEntity(entity->GetID());
		}
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		// Laid out like an editor on first run: hierarchy left, inspector
		// right. FirstUseEver so a user's own arrangement survives restarts.
		ImGui::SetNextWindowPos(ImVec2(20.0f, 40.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(260.0f, 320.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Scene Hierarchy");

		if (m_Scene)
		{
			ImGui::TextDisabled("%s", m_Scene->GetName().c_str());
			ImGui::Separator();

			// Copy so that a deletion during the walk cannot invalidate it.
			auto entities = m_Scene->GetEntities();
			for (const auto& entity : entities)
				DrawEntityNode(entity);

			// Clicking empty space clears the selection.
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
				m_SelectedID = 0;

			if (ImGui::BeginPopupContextWindow("hierarchy_ctx", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
			{
				if (ImGui::MenuItem("Create Empty"))
					m_Scene->CreateEntity("Empty", EntityType::Empty);
				if (ImGui::MenuItem("Create Cube"))
				{
					auto e = m_Scene->CreateEntity("Cube", EntityType::Mesh);
					e->Mesh = Mesh::CreateCube();
				}
				if (ImGui::MenuItem("Create Plane"))
				{
					auto e = m_Scene->CreateEntity("Plane", EntityType::Mesh);
					e->Mesh = Mesh::CreatePlane();
				}
				ImGui::EndPopup();
			}
		}
		else
		{
			ImGui::TextDisabled("No scene bound.");
		}

		ImGui::End();
	}

	void PropertiesPanel::OnImGuiRender(const Ref<Scene>& scene, const Ref<Entity>& entity)
	{
		ImGui::SetNextWindowPos(ImVec2(980.0f, 40.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(280.0f, 460.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Inspector");

		if (!entity)
		{
			ImGui::TextDisabled("Nothing selected.");
			ImGui::End();
			return;
		}

		// Name
		{
			char buffer[128];
			std::memset(buffer, 0, sizeof(buffer));
			std::strncpy(buffer, entity->Name.c_str(), sizeof(buffer) - 1);
			if (ImGui::InputText("Name", buffer, sizeof(buffer)))
				entity->Name = buffer;
		}

		ImGui::SameLine();
		ImGui::Checkbox("Visible", &entity->Visible);

		ImGui::TextDisabled("%s  (id %u)", TypeLabel(entity->Type), entity->GetID());
		ImGui::Separator();

		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			DrawVec3Control("Position", entity->Transform.Translation);
			DrawVec3Control("Rotation", entity->Transform.Rotation, 0.0f, 1.0f);
			DrawVec3Control("Scale", entity->Transform.Scale, 1.0f);
		}

		if (entity->Type == EntityType::Mesh && ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
		{
			const char* current = entity->Mesh ? "Mesh" : "None";
			if (ImGui::BeginCombo("Primitive", current))
			{
				if (ImGui::Selectable("Cube"))
					entity->Mesh = Mesh::CreateCube();
				if (ImGui::Selectable("Plane"))
					entity->Mesh = Mesh::CreatePlane();
				if (ImGui::Selectable("Sphere"))
					entity->Mesh = Mesh::CreateSphere();
				if (ImGui::Selectable("None"))
					entity->Mesh = nullptr;
				ImGui::EndCombo();
			}

			if (entity->Mesh)
				ImGui::TextDisabled("%u indices, %u triangles",
					entity->Mesh->GetIndexCount(), entity->Mesh->GetIndexCount() / 3);
		}

		if (entity->Type == EntityType::Mesh && ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (entity->Material)
			{
				ImGui::TextDisabled("Shader: %s",
					entity->Material->GetShader() ? entity->Material->GetShader()->GetName().c_str() : "none");
				ImGui::ColorEdit4("Albedo", &entity->Material->Albedo.x);
				ImGui::SliderFloat("Metallic", &entity->Material->Metallic, 0.0f, 1.0f);
				ImGui::SliderFloat("Roughness", &entity->Material->Roughness, 0.0f, 1.0f);

				auto describeMap = [](const char* label, const Gymon::Ref<Gymon::Texture2D>& map)
				{
					if (map)
						ImGui::TextDisabled("%s: %ux%u, %u mip%s", label,
							map->GetWidth(), map->GetHeight(),
							map->GetMipLevels(), map->GetMipLevels() == 1 ? "" : "s");
					else
						ImGui::TextDisabled("%s: none", label);
				};
				describeMap("Albedo map", entity->Material->AlbedoMap);
				describeMap("Normal map", entity->Material->NormalMap);
				describeMap("Metal/Rough", entity->Material->MetallicRoughnessMap);
			}
			else
			{
				ImGui::TextDisabled("No material assigned.");
			}
		}

		if (entity->Type == EntityType::DirectionalLight && ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::ColorEdit3("Color", &entity->LightColor.x);
			ImGui::DragFloat("Intensity", &entity->LightIntensity, 0.05f, 0.0f, 10.0f);
		}

		ImGui::End();
	}
}
