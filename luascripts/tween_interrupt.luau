local service = game:GetService("TweenService")
local gui = Instance.new("ScreenGui")

task.delay(10, gui.Destroy, gui)

gui.Parent = game:GetService("CoreGui")

local main_frame = Instance.new("Frame")
main_frame.Parent = gui

main_frame.Size = UDim2.new(1, -300, 1, -350)
main_frame.Position = UDim2.new(0, 150, 0, 300)

local tween = service:Create(main_frame, TweenInfo.new(5), {
	Position = UDim2.fromOffset(300, 400),
	BorderSizePixel = 10
})

tween.Completed:Connect(function()
    print("tween1 status", tween.PlaybackState)
    print("bordersizepixel", main_frame.BorderSizePixel)
end)

tween:Play()

task.wait(2)
print"PLAYING SECOND"

local tween2 = service:Create(main_frame, TweenInfo.new(3, nil, nil, nil, nil, 1), {
	Position = UDim2.fromOffset(200, 100)
})
tween2:Play()
