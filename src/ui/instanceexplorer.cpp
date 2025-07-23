#include "ui/instanceexplorer.hpp"
#include "ui/ui.hpp"
#include "classes/roblox/datatypes/enum.hpp"
#include "classes/roblox/instance.hpp"

#include "imgui.h"

#include <queue>

namespace fakeroblox {

std::shared_ptr<rbxInstance> game;

std::shared_ptr<rbxInstance> selected_instance;
std::queue<std::shared_ptr<rbxInstance>> destroy_queue;

void UI_InstanceExplorer_init(std::shared_ptr<rbxInstance> datamodel) {
    game = datamodel;
}

static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
void renderInstance(lua_State* L, std::shared_ptr<rbxInstance>& instance) {
    std::shared_lock instance_children_lock(instance->children_mutex);

    auto& object_name = instance->getValue<std::string>(PROP_INSTANCE_NAME);
    auto children_count = instance->children.size();

    ImGui::PushID(instance.get());

    ImGuiTreeNodeFlags flags = base_flags;
    if (selected_instance && instance == selected_instance)
        flags |= ImGuiTreeNodeFlags_Selected;
    if (!children_count)
        flags |= ImGuiTreeNodeFlags_Leaf;

    bool open = ImGui::TreeNodeEx("##node", flags, "%.*s", static_cast<int>(object_name.size()), object_name.c_str());
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        selected_instance = instance;

    if (ImGui::BeginPopupContextItem()) {
        const bool parent_locked = instance->parent_locked;

        if (parent_locked)
            ImGui::BeginDisabled();
        if (ImGui::Button("Destroy"))
            destroy_queue.push(instance);
        if (parent_locked) {
            ImGui::SetItemTooltip("parent locked");
            ImGui::EndDisabled();
        }

        ImGui::EndPopup();
    }

    if (open) {
        for (unsigned int ichild = 0; ichild < children_count; ichild++)
            renderInstance(L, instance->children[ichild]);
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void renderPropertyValue(rbxProperty* property, rbxValueVariant& value) {
    ImGui::PushID(property);
    static const char* label = "##value";

    // FIXME: reportChanged
    switch (property->type_category) {
        case Primitive:
            if (std::holds_alternative<bool>(value))
                ImGui::Checkbox(label, &std::get<bool>(value));
            else if (std::holds_alternative<int32_t>(value))
                ImGui::DragScalar(label, ImGuiDataType_S32, &std::get<int32_t>(value));
            else if (std::holds_alternative<int64_t>(value))
                ImGui::DragScalar(label, ImGuiDataType_S64, &std::get<int64_t>(value));
            else if (std::holds_alternative<float>(value))
                ImGui::DragScalar(label, ImGuiDataType_Float, &std::get<float>(value));
            else if (std::holds_alternative<double>(value))
                ImGui::DragScalar(label, ImGuiDataType_Double, &std::get<double>(value));
            else if (std::holds_alternative<std::string>(value))
                ImGui_STDString(label, std::get<std::string>(value));
            break;
        case DataType:
            if (std::holds_alternative<EnumItemWrapper>(value)) {
                auto& wrapper = std::get<EnumItemWrapper>(value);

                int selected = -1;
                std::vector<const char*> item_list;
                auto& item_map = Enum::enum_map[wrapper.enum_name].item_map;

                const size_t count = item_map.size();
                if (count) {
                    item_list.reserve(count);
                    int index = 0;
                    for (auto it = item_map.begin(); it != item_map.end(); it++, index++) {
                        if (wrapper.name == it->first)
                            selected = index;
                        item_list.push_back(it->first.c_str());
                    }

                    ImGui::Combo(label, &selected, item_list.data(), item_list.size());

                    if (selected > -1)
                        wrapper.name = item_list[selected];
                }
            } else if (std::holds_alternative<Color>(value))
                ImGui_Color3(label, std::get<Color>(value));
            else if (std::holds_alternative<Vector2>(value))
                ImGui_DragVector2(label, std::get<Vector2>(value));
            else if (std::holds_alternative<Vector3>(value))
                ImGui_DragVector3(label, std::get<Vector3>(value));
            else if (std::holds_alternative<UDim>(value))
                ImGui_DragUDim(label, std::get<UDim>(value));
            else if (std::holds_alternative<UDim2>(value))
                ImGui_DragUDim2(label, std::get<UDim2>(value));
            else
                ImGui::Text("TODO: DataType");
            break;
        case Instance:
            ImGui::Text("TODO: Instance");
            break;
    }

    ImGui::PopID();
}

// TODO: lua api to get/set selected instance, focus instance, etc
void UI_InstanceExplorer_render(lua_State *L) {
    ImGui::BeginChild("Explorer", ImVec2{0, ImGui::GetContentRegionAvail().y / 2.f}, ImGuiChildFlags_None);

    std::shared_lock game_children_lock(game->children_mutex);

    for (unsigned int ichild = 0; ichild < game->children.size(); ichild++)
        renderInstance(L, game->children[ichild]);

    game_children_lock.unlock();

    while (!destroy_queue.empty()) {
        auto& instance = destroy_queue.front();

        if (selected_instance && selected_instance == instance)
            selected_instance = nullptr;

        destroyInstance(L, instance);

        destroy_queue.pop();
    }

    ImGui::EndChild();

    ImGui::BeginChild("Properties", ImVec2{0, 0}, ImGuiChildFlags_None);

    const auto& rbxinstance_parent_property = rbxClass::class_map["Instance"]->properties["Parent"];

    if (selected_instance) {
        auto selected_instance_name = selected_instance->getValue<std::string>(PROP_INSTANCE_NAME);
        ImGui::Text("%.*s", static_cast<int>(selected_instance_name.size()), selected_instance_name.c_str());

        ImGui::SeparatorText("Properties");
        if (ImGui::BeginTable("Properties##table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            std::shared_lock values_lock(selected_instance->values_mutex);

            for (auto& value_pair : selected_instance->values) {
                auto& property = value_pair.second.property;
                if (property->tags & rbxProperty::Hidden || property->tags & rbxProperty::Deprecated
                    || property->tags & rbxProperty::WriteOnly || property->tags & rbxProperty::NotScriptable
                ) { continue; }

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%.*s", static_cast<int>(value_pair.first.size()), value_pair.first.c_str());
                ImGui::TableNextColumn();

                const bool read_only = property->tags & rbxProperty::ReadOnly;
                const bool parent_locked = property == rbxinstance_parent_property && selected_instance->parent_locked;

                const bool disabled = read_only || parent_locked;
                if (disabled)
                    ImGui::BeginDisabled();

                renderPropertyValue(property.get(), value_pair.second.value);

                if (read_only)
                    ImGui::SetItemTooltip("read-only");
                else if (parent_locked)
                    ImGui::SetItemTooltip("parent locked");

                if (disabled)
                    ImGui::EndDisabled();
            }

            values_lock.unlock();

            ImGui::EndTable();
        }

        ImGui::SeparatorText("Attributes");
        ImGui::Text("WIP");
    }

    ImGui::EndChild();
}

}; // namespace fakeroblox
