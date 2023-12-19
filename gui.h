#pragma once

#include <imgui.h>

#include <string>

class Gui {
   public:
    Gui(const std::string& name) : name(name) {}
    virtual ~Gui() = default;

    void gui_render() {
        ImGui::Begin(name.c_str());
        gui_draw();
        ImGui::End();
    }

   protected:
    virtual void gui_draw() = 0;

   private:
    std::string name;
};
