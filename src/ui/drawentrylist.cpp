#include "ui/drawentrylist.hpp"
#include "fontloader.hpp"
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
        static const char* item_list[] = { "Line", "Text", "Image", "Circle", "Square", "Triangle", "Quad" };

        if (ImGui::Button("Create")) {
            pushNewDrawEntry(L, item_list[item]);
            lua_pop(L, 1);
        }

        ImGui::SameLine();
        ImGui::Combo("ClassName", &item, item_list, IM_ARRAYSIZE(item_list));

        if (ImGui::Button("Clear"))
            DrawEntry::clear(L);
    }

    ImGui::BeginChild("DrawEntry List##chooser", ImVec2{ImGui::GetContentRegionAvail().x * 0.3f, ImGui::GetContentRegionAvail().y}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
    bool chosen_still_exists = false;

    DrawEntry* entry_to_clone = nullptr;
    std::shared_lock draw_list_lock(DrawEntry::draw_list_mutex);
    for (auto& entry : DrawEntry::draw_list) {
        bool is_selected = drawentry_list_chosen && entry == drawentry_list_chosen;

        if (is_selected)
            chosen_still_exists = true;

        ImGui::PushID(entry);

        #define NAME_BUF_SIZE 40

        char buf[NAME_BUF_SIZE];
        if (entry->type == DrawEntry::DrawTypeText) {
            std::string& text = static_cast<DrawEntryText*>(entry)->text;
            if (text.size() > NAME_BUF_SIZE - 20)
                snprintf(buf, NAME_BUF_SIZE, "entry: %s (\"%.*s\"...)", entry->class_name, NAME_BUF_SIZE - 23, text.c_str());
            else
                snprintf(buf, NAME_BUF_SIZE, "entry: %s (\"%s\")", entry->class_name, text.c_str());

            #undef STR_BUF_SIZE
        } else
            snprintf(buf, NAME_BUF_SIZE, "entry: %s", entry->class_name);

        #undef NAME_BUF_SIZE

        if (ImGui::Selectable(buf, is_selected, ImGuiSelectableFlags_None)) {
            chosen_still_exists = true;
            drawentry_list_chosen = entry;
        }
        if (ImGui::BeginPopupContextItem()) {
            ImGui::Checkbox("Visible", &entry->visible);
            if (ImGui::Button("Destroy")) {
                draw_list_lock.unlock();
                entry->destroy(L);
                draw_list_lock.lock();
            } else if (ImGui::Button("Clone"))
                entry_to_clone = entry;
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }
    draw_list_lock.unlock();

    if (entry_to_clone) {
        chosen_still_exists = true;
        drawentry_list_chosen = entry_to_clone->clone(L);
    }

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
            case DrawEntry::DrawTypeLine: {
                DrawEntryLine* entry_line = static_cast<DrawEntryLine*>(entry);

                ImGui::DragScalar("Thickness", ImGuiDataType_Double, &entry_line->thickness);

                ImGui_DragVector2("From", entry_line->from);
                ImGui_DragVector2("To", entry_line->to);

                break;
            }
            case DrawEntry::DrawTypeText: {
                DrawEntryText* entry_text = static_cast<DrawEntryText*>(entry);

                // NOTE: explicitly call updateTextBounds because it is usually lazy evaluated via __newindex, and we currently have no way to tell when the string gets updated
                entry_text->updateTextBounds();

                ImGui_STDString("Text", entry_text->text);
                ImGui::Text("%.f, %.f - TextBounds", entry_text->text_bounds.x, entry_text->text_bounds.y);
                ImGui::DragScalar("TextSize", ImGuiDataType_Double, &entry_text->text_size);

                std::vector<const char*> font_list;
                font_list.reserve(FontLoader::font_name_list.size());

                for (size_t i = 0; i < FontLoader::font_name_list.size(); i++)
                    font_list.push_back(FontLoader::font_name_list[i].c_str());
                if (ImGui::Combo("Font", reinterpret_cast<int*>(&entry_text->font_index), font_list.data(), FontLoader::font_count))
                    entry_text->updateFont();

                ImGui::Checkbox("Centered", &entry_text->centered);
                ImGui::Checkbox("Outlined", &entry_text->outlined);
                ImGui_Color4("Outline Color", entry_text->outline_color);
                ImGui_DragVector2("Position", entry_text->position);

                break;
            }
            case DrawEntry::DrawTypeImage: {
                DrawEntryImage* entry_image = static_cast<DrawEntryImage*>(entry);

                ImGui::Text("%.f, %.f - ImageSize", entry_image->image_size.x, entry_image->image_size.y);
                ImGui_DragVector2("Size", entry_image->size);
                ImGui_DragVector2("Position", entry_image->position);
                ImGui::DragScalar("Rounding", ImGuiDataType_Double, &entry_image->rounding);

                break;
            }
            case DrawEntry::DrawTypeCircle: {
                DrawEntryCircle* entry_circle = static_cast<DrawEntryCircle*>(entry);

                // TODO: thickness (doesn't work yet)
                // ImGui::DragScalar("Thickness", ImGuiDataType_Double, &entry_circle->thickness);
                ImGui::DragScalar("NumSides", ImGuiDataType_S32, &entry_circle->num_sides);
                ImGui::DragScalar("Radius", ImGuiDataType_Double, &entry_circle->radius);
                ImGui::Checkbox("Filled", &entry_circle->filled);
                ImGui_DragVector2("Center", entry_circle->center);

                break;
            }
            case DrawEntry::DrawTypeSquare: {
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
            case DrawEntry::DrawTypeTriangle: {
                DrawEntryTriangle* entry_triangle = static_cast<DrawEntryTriangle*>(entry);

                ImGui::DragScalar("Thickness", ImGuiDataType_Double, &entry_triangle->thickness);
                ImGui_DragVector2("PointA", entry_triangle->pointa);
                ImGui_DragVector2("PointB", entry_triangle->pointb);
                ImGui_DragVector2("PointC", entry_triangle->pointc);
                ImGui::Checkbox("Filled", &entry_triangle->filled);

                break;
            }
            case DrawEntry::DrawTypeQuad: {
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

        if (ImGui::Button("Destroy"))
            entry->destroy(L);
        else if (ImGui::Button("Clone"))
            drawentry_list_chosen = entry->clone(L);

        ImGui::EndChild();
    }
}

}; // namespace fakeroblox
