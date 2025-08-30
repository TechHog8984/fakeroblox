local ImGui = game:GetService("ImGuiService")

local window_active = Instance.new("BoolValue")
window_active.Value = true

local toggle1 = Instance.new("BoolValue")
local combo1_value = Instance.new("IntValue")
local combo1 = { "value1", "value2" }

local str = Instance.new("StringValue")
str.Value = "hello world"

local color = Instance.new("Color3Value")

local render_connection
render_connection = ImGui.Render:Connect(function()
    if ImGui.Begin("Lua Window", window_active) then
        ImGui.Text(("Hello, %s"):format("world!"))

        if ImGui.Button("Destroy") then
            window_active.Value = false
        end

        if ImGui.Checkbox("Toggle1", toggle1) then
            print("changed toggle1;", toggle1.Value)
        end

        ImGui.Bullet()
        ImGui.Text("Bullet Point 1")

        if ImGui.Combo("Combo1", combo1_value, combo1) then
            print("changed combo1;", combo1[combo1_value.Value + 1])
        end

        if ImGui.InputText("String", str) then
            print("str changed;", str.Value)
        end

        if ImGui.ColorEdit("color", color) then
            print("color changed;", color.Value)
        end
    end
    ImGui.End()

    -- disconnecting the connection should go after the button press because
    -- we don't want to omit the above End midway through
    if not window_active.Value then
        print("disconnecting...")
        render_connection:Disconnect()
    end
end)
