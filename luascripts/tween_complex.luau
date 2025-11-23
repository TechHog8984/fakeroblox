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

task.wait(1)
print("1 second after calling play")

task.delay(1.5, tween.Cancel, tween)

task.wait(2)
print("2 seconds after start")

print(" ")
task.wait(1)
print("1 second after end time; playing again")
tween:Play()

task.wait(1)
print("1 second after calling play (x2)")

task.wait(2)
print("2 seconds after start")

print(" ")
task.wait(1)
print("1 second after end time; playing again")
tween:Play()

task.wait(1)
print("1 second after calling play (x3)")

task.delay(1.5, tween.Pause, tween)

task.wait(2)
print("2 seconds after start")

task.wait(2)
print("2 seconds after previous update")

tween:Play()

task.wait(0.5)
print("0.5 seconds after resuming")
