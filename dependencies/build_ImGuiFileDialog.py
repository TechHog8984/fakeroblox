from buildscripts.utils import *

from pathlib import Path

if __name__ == "__main__":
    if not Path("./rlImGui/imgui-master").exists():
        fail_exit("run ./build_rlImGui.sh first (or ensure ./rlImGui/imgui-master)")

    change_directory("ImGuiFileDialog")

    replace_in_file("CMakeLists.txt", "target_include_directories(ImGuiFileDialog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})", "target_include_directories(ImGuiFileDialog PUBLIC\n ${CMAKE_CURRENT_SOURCE_DIR}\n ${CMAKE_CURRENT_SOURCE_DIR}/../rlImGui/imgui-master\n)")

    ensure_directory("cmake")
    change_directory("cmake")

    run_command(["cmake", ".."])
    run_command(["cmake", "--build", ".", "--config", "Release"])
