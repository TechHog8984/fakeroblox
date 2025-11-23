local inst = Instance.new("NumberValue")

inst.Value = 1

inst.Changed:Connect(function(prop)
    warn("INST", prop, inst[prop])
end)

local tween = game:GetService("TweenService"):Create(inst, TweenInfo.new(2, nil, nil, nil, nil, 1), {Value = 3})

print("initial", tween.PlaybackState)

tween.Changed:Connect(function(prop)
    warn("TWEEN", prop, tween[prop])
end)
tween.Completed:Connect(warn)

print("calling play")
tween:Play()
