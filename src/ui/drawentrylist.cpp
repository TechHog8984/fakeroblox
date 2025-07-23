#include "ui/drawentrylist.hpp"
#include "imgui.h"
#include "ui/ui.hpp"

#include "console.hpp"

#include "libraries/drawentrylib.hpp"

namespace fakeroblox {

DrawEntry* drawentry_list_chosen = nullptr;

void UI_DrawEntryList_render(lua_State *L) {
    // "Create" button
    {
        static int item = 0;
        const char* item_list[] = { "Line", "Text", "Image", "Circle", "Square", "Triangle", "Quad" };

        if (ImGui::Button("Create")) {
            pushNewDrawEntry(L, item_list[item]);
            lua_pop(L, 1);
        }

        ImGui::SameLine();
        ImGui::Combo("ClassName", &item, item_list, IM_ARRAYSIZE(item_list));
    }

    ImGui::BeginChild("DrawEntry List##chooser", ImVec2{ImGui::GetContentRegionAvail().x * 0.25f, ImGui::GetContentRegionAvail().y}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
    bool chosen_still_exists = false;

    std::shared_lock draw_list_lock(DrawEntry::draw_list_mutex);
    for (auto& entry : DrawEntry::draw_list) {
        bool is_selected = drawentry_list_chosen && entry == drawentry_list_chosen;

        if (is_selected)
            chosen_still_exists = true;

        ImGui::PushID(entry);

        char buf[20];
        snprintf(buf, 20, "entry: %s", entry->class_name);
        if (ImGui::Selectable(buf, is_selected, ImGuiSelectableFlags_None)) {
            chosen_still_exists = true;
            drawentry_list_chosen = entry;
        }
        if (ImGui::BeginPopupContextItem()) {
            ImGui::Checkbox("Visible", &entry->visible);
            // TODO: DrawEntry clone
            // if (ImGui::Button("Clone"))
            //     entry->clone();
            if (ImGui::Button("Destroy")) {
                draw_list_lock.unlock();
                entry->destroy(L);
                draw_list_lock.lock();
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }
    draw_list_lock.unlock();

    if (!chosen_still_exists)
        drawentry_list_chosen = nullptr;

    ImGui::EndChild();

    if (drawentry_list_chosen) {
        std::lock_guard members_lock(drawentry_list_chosen->members_mutex);

        ImGui::SameLine();

        auto& entry = drawentry_list_chosen;
        ImGui::BeginChild("DrawEntry List##display", ImVec2{0, 0}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::Text("%s (%p)", entry->class_name, entry);

        ImGui::SeparatorText("Properties");

        ImGui::Checkbox("Visible", &entry->visible);
        if (ImGui::DragScalar("ZIndex", ImGuiDataType_U32, &entry->zindex))
            entry->onZIndexUpdate();
        ImGui_Color4("Color", entry->color);

        switch (entry->type) {
            case DrawEntry::Line: {
                DrawEntryLine* entry_line = static_cast<DrawEntryLine*>(entry);

                ImGui::DragScalar("Thickness", ImGuiDataType_Double, &entry_line->thickness);

                ImGui_DragVector2("From", entry_line->from);
                ImGui_DragVector2("To", entry_line->to);

                break;
            }
            case DrawEntry::Text: {
                DrawEntryText* entry_text = static_cast<DrawEntryText*>(entry);

                // NOTE: explicitly call updateTextBounds because it is usually lazy evaluated via __newindex
                entry_text->updateTextBounds();

                ImGui_STDString("Text", entry_text->text);
                ImGui::Text("%.f, %.f - TextBounds", entry_text->text_bounds.x, entry_text->text_bounds.y);
                ImGui::DragScalar("TextSize", ImGuiDataType_Double, &entry_text->text_size);
                // TODO: fonts (don't work yet)
                ImGui::Checkbox("Centered", &entry_text->centered);
                ImGui::Checkbox("Outlined", &entry_text->outlined);
                ImGui_Color4("Outline Color", entry_text->outline_color);
                ImGui_DragVector2("Position", entry_text->position);

                break;
            }
            case DrawEntry::Circle: {
                DrawEntryCircle* entry_circle = static_cast<DrawEntryCircle*>(entry);

                // TODO: thickness (doesn't work yet)
                // ImGui::DragScalar("Thickness", ImGuiDataType_Double, &entry_circle->thickness);
                ImGui::DragScalar("NumSides", ImGuiDataType_S32, &entry_circle->num_sides);
                ImGui::DragScalar("Radius", ImGuiDataType_Double, &entry_circle->radius);
                ImGui::Checkbox("Filled", &entry_circle->filled);
                ImGui_DragVector2("Center", entry_circle->center);

                break;
            }
            case DrawEntry::Square: {
                DrawEntrySquare* entry_square = static_cast<DrawEntrySquare*>(entry);

                ImGui::DragScalar("Thickness", ImGuiDataType_Double, &entry_square->thickness);
                auto& rect = entry_square->rect;
                Vector2 size{rect.width, rect.height};
                Vector2 position{rect.x, rect.y};
                ImGui_DragVector2("Size", size);
                ImGui_DragVector2("Position", position);

                rect.width = size.x;
                rect.height = size.y;
                rect.x = position.x;
                rect.y = position.y;

                ImGui::Checkbox("Filled", &entry_square->filled);

                break;
            }
            case DrawEntry::Triangle: {
                DrawEntryTriangle* entry_triangle = static_cast<DrawEntryTriangle*>(entry);

                ImGui::DragScalar("Thickness", ImGuiDataType_Double, &entry_triangle->thickness);
                ImGui_DragVector2("PointA", entry_triangle->pointa);
                ImGui_DragVector2("PointB", entry_triangle->pointb);
                ImGui_DragVector2("PointC", entry_triangle->pointc);
                ImGui::Checkbox("Filled", &entry_triangle->filled);

                break;
            }
            case DrawEntry::Quad: {
                DrawEntryQuad* entry_quad = static_cast<DrawEntryQuad*>(entry);

                ImGui::DragScalar("Thickness", ImGuiDataType_Double, &entry_quad->thickness);
                ImGui_DragVector2("PointA", entry_quad->pointa);
                ImGui_DragVector2("PointB", entry_quad->pointb);
                ImGui_DragVector2("PointC", entry_quad->pointc);
                ImGui_DragVector2("PointD", entry_quad->pointd);
                ImGui::Checkbox("Filled", &entry_quad->filled);

                break;
            }
            default: {
                Console::ScriptConsole.debugf("INTERNAL TODO: all DrawEntry types (%s)", entry->class_name);
                break;
            }
        }

        ImGui::SeparatorText("Methods");

        if (ImGui::Button("Destroy")) {
            draw_list_lock.unlock();
            entry->destroy(L);
            // lock.lock(); // commented out because there's no code after this
        }

        ImGui::EndChild();
    }
}

}; // namespace fakeroblox
