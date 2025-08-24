#include "ui/instanceexplorer.hpp"
#include "ui/ui.hpp"
#include "classes/roblox/datatypes/enum.hpp"
#include "classes/roblox/instance.hpp"

#include "imgui.h"

#include <queue>

namespace fakeroblox {

std::shared_ptr<rbxInstance> game;

std::weak_ptr<rbxInstance> selected_instance;

typedef struct {
    enum {
        Destroy,
        Clone
    } type;
    std::shared_ptr<rbxInstance> instance;
} ContextQueueItem;

std::queue<ContextQueueItem> context_queue;

void UI_InstanceExplorer_init(std::shared_ptr<rbxInstance> datamodel) {
    game = datamodel;
}

// options
static bool show_address_near_name = false;
static bool show_input_objects = false; // TODO: we should have a designated filter option where you can search classnames or something

void renderInstance(lua_State* L, std::shared_ptr<rbxInstance>& instance);

static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
void renderNode(lua_State* L, std::shared_ptr<rbxInstance> instance, std::vector<std::shared_ptr<rbxInstance>>& children, std::string name) {
    auto children_count = children.size();

    ImGuiTreeNodeFlags flags = base_flags;
    if (instance && !selected_instance.expired() && instance == selected_instance.lock())
        flags |= ImGuiTreeNodeFlags_Selected;
    if (!children_count)
        flags |= ImGuiTreeNodeFlags_Leaf;

    bool open = ImGui::TreeNodeEx("##node", flags, "%.*s", static_cast<int>(name.size()), name.c_str());
    if (instance && ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        selected_instance = instance;

    if (instance && ImGui::BeginPopupContextItem()) {
        const bool not_creatable = instance->_class->tags & rbxClass::NotCreatable;
        const bool not_archivable = !getValue<bool>(instance, PROP_INSTANCE_ARCHIVABLE);

        if (not_creatable || not_archivable)
            ImGui::BeginDisabled();
        if (ImGui::Button("Clone"))
            context_queue.push({ .type = ContextQueueItem::Clone, .instance = instance });
        if (not_creatable || not_archivable) {
            ImGui::SetItemTooltip(not_creatable ? "not creatable" : "not archivable");
            ImGui::EndDisabled();
        }

        const bool parent_locked = instance->parent_locked;

        if (parent_locked)
            ImGui::BeginDisabled();
        if (ImGui::Button("Destroy"))
            context_queue.push({ .type = ContextQueueItem::Destroy, .instance = instance });
        if (parent_locked) {
            ImGui::SetItemTooltip("parent locked");
            ImGui::EndDisabled();
        }

        ImGui::EndPopup();
    }

    if (open) {
        for (unsigned int ichild = 0; ichild < children_count; ichild++)
            renderInstance(L, children[ichild]);
        ImGui::TreePop();
    }
}
void renderInstance(lua_State* L, std::shared_ptr<rbxInstance>& instance) {
    if (!show_input_objects && instance->isA("InputObject"))
        return;

    std::shared_lock instance_children_lock(instance->children_mutex);

    auto object_name = getValue<std::string>(instance, PROP_INSTANCE_NAME);
    if (show_address_near_name) {
        char buf[50];
        snprintf(buf, 50, " (%p)", instance.get());
        object_name.append(buf);
    }

    ImGui::PushID(instance.get());

    renderNode(L, instance, instance->children, object_name);

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
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Options")) {
            ImGui::MenuItem("Show Address Near Instance Name", nullptr, &show_address_near_name);
            ImGui::MenuItem("Show Input Objects", nullptr, &show_input_objects);

            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::BeginChild("Explorer", ImVec2{0, ImGui::GetContentRegionAvail().y / 2.f}, ImGuiChildFlags_None);

    {
        std::shared_lock game_children_lock(game->children_mutex);

        for (unsigned int ichild = 0; ichild < game->children.size(); ichild++)
            renderInstance(L, game->children[ichild]);

        ImGui::SeparatorText("Psuedo Parents");

        ImGui::PushID("Nil Instances");

        auto nil_instances = getNilInstances();
        std::vector<std::shared_ptr<rbxInstance>> shared_nil_instances;
        shared_nil_instances.reserve(nil_instances.size());

        for (size_t i = 0; i < nil_instances.size(); i++)
            if (auto instance = nil_instances[i].lock())
                shared_nil_instances.push_back(instance);

        renderNode(L, nullptr, shared_nil_instances, "Nil Instances");

        ImGui::PopID();
    }

    while (!context_queue.empty()) {
        auto& item = context_queue.front();
        auto instance = item.instance;

        switch (item.type) {
            case ContextQueueItem::Destroy:
                destroyInstance(L, instance);

                if (auto selected = selected_instance.lock())
                    if (selected == instance)
                        selected_instance.reset();

                break;
            case ContextQueueItem::Clone: {
                auto clone = cloneInstance(L, instance);
                if (clone) {
                    setInstanceParent(L, clone, getValue<std::shared_ptr<rbxInstance>>(instance, PROP_INSTANCE_PARENT), true);
                    selected_instance = clone;
                }
                // TODO: notification system; notify(attempt to clone an instance that's not Archivable!)
                break;
            }
        }

        context_queue.pop();
    }

    if (auto selected = selected_instance.lock())
        if (selected->destroyed)
            selected_instance.reset();

    ImGui::EndChild();

    ImGui::BeginChild("Properties", ImVec2{0, 0}, ImGuiChildFlags_None);

    const auto& rbxinstance_parent_property = rbxClass::class_map["Instance"]->properties["Parent"];

    if (auto selected = selected_instance.lock()) {
        auto selected_name = getValue<std::string>(selected, PROP_INSTANCE_NAME);
        ImGui::Text("%.*s", static_cast<int>(selected_name.size()), selected_name.c_str());

        ImGui::SeparatorText("Properties");
        if (ImGui::BeginTable("Properties##table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            std::lock_guard values_lock(selected->values_mutex);

            for (auto& value_pair : selected->values) {
                auto& property = value_pair.second.property;
                if (property->tags & rbxProperty::Hidden || property->tags & rbxProperty::Deprecated
                    || property->tags & rbxProperty::WriteOnly || property->tags & rbxProperty::NotScriptable
                ) { continue; }

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%.*s", static_cast<int>(value_pair.first.size()), value_pair.first.c_str());
                ImGui::TableNextColumn();

                const bool read_only = property->tags & rbxProperty::ReadOnly;
                const bool parent_locked = property == rbxinstance_parent_property && selected->parent_locked;

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

            ImGui::EndTable();
        }

        ImGui::SeparatorText("Attributes");
        ImGui::Text("WIP");
    }

    ImGui::EndChild();
}

}; // namespace fakeroblox
