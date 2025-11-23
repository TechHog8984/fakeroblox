local TIME = 2
local DELAY = 0
local REPEAT = 1

local SECOND = {
    time = 1,
    delay = 1.2,
    rep = 2
}

local service = game:GetService("TweenService")
local gui = Instance.new("ScreenGui")

local duration = (DELAY + TIME) * (REPEAT + 1) + 1
duration += ((SECOND.time * 2) + SECOND.delay) * (SECOND.rep + 1)

task.delay(duration, gui.Destroy, gui)

gui.Parent = game:GetService("CoreGui")

local main_frame = Instance.new("Frame")
main_frame.Parent = gui

main_frame.Size = UDim2.new(1, -300, 1, -350)
main_frame.Position = UDim2.new(0, 150, 0, 300)

local info = TweenInfo.new(TIME, nil, nil, REPEAT, nil, DELAY)
local tween = service:Create(main_frame, info, {
    BackgroundColor3 = Color3.fromRGB(200, 20, 255),
    Position = main_frame.Position + UDim2.new(0, 50, 0, 20)
})

main_frame:GetPropertyChangedSignal("BackgroundColor3"):Connect(function()
    print("color", main_frame.BackgroundColor3)
end)
tween:GetPropertyChangedSignal("PlaybackState"):Connect(function()
    print("state", tween.PlaybackState)
end)

tween:Play()

tween.Completed:Wait()
print("first tween done")

tween = service:Create(main_frame, TweenInfo.new(SECOND.time, nil, nil, SECOND.rep, true, SECOND.delay), {
    Position = main_frame.Position - UDim2.new(0, 60, 0, 30)
})
tween:Play()

tween.Completed:Wait()
print("second tween done")
