local coregui = game:GetService("CoreGui")
coregui:ClearAllChildren()

local gui = Instance.new("ScreenGui")
gui.Parent = coregui

local main_frame = Instance.new("Frame")
main_frame.Parent = gui

main_frame.Size = UDim2.new(1, -300, 1, -350)
main_frame.Position = UDim2.new(0, 150, 0, 300)

local play_button = Instance.new("TextButton")
play_button.Parent = main_frame

play_button.Size = UDim2.new(0, 220, 0, 100)
play_button.Position = UDim2.new(0.5, -110, 0.5, -50)
