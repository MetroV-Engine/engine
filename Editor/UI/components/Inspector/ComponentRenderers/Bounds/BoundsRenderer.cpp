#include "BoundsRenderer.hpp"
#include <imgui.h>
#include "ValueRenderer/ValueRenderer.hpp"
#include "Reflection/FieldDescriptor.hpp"

namespace Editor::UI::Components::Renderers {
    void BoundsRenderer::render(IComponent &component, bool &preserveRatio, float &storedRatio)
    {
        if (!ImGui::CollapsingHeader("Bounds", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        auto fields = component.getFields();
        float *width = nullptr;
        float *height = nullptr;
        Proto::Color *color = nullptr;

        for (auto &field : fields) {
            if (field.name == "width")
                width = std::get<float*>(field.valuePtr);
            else if (field.name == "height")
                height = std::get<float*>(field.valuePtr);
            else if (field.name == "color")
                color = std::get<Proto::Color*>(field.valuePtr);
        }

        if (width && height) {
            if (ImGui::Checkbox("Preserve Ratio", &preserveRatio)) {
                if (preserveRatio && *height > 0.f)
                    storedRatio = *width / *height;
            }
            
            if (ImGui::DragFloat("width", width, 0.1f)) {
                if (preserveRatio && storedRatio > 0.f)
                    *height = *width / storedRatio;
            }
            
            if (ImGui::DragFloat("height", height, 0.1f)) {
                if (preserveRatio && storedRatio > 0.f)
                    *width = *height * storedRatio;
            }

            if (preserveRatio && *height > 0.f)
                storedRatio = *width / *height;
        }

        if (color) {
            auto &valueRenderer = Editor::ValueRenderer::getInstance();
            valueRenderer.renderField("color", ECS::Reflection::FieldType::Color, color);
        }
    }
}
