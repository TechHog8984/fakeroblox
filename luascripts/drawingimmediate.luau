local connections = {}

connections[#connections + 1] = DrawingImmediate.GetPaint(0):Connect(function()
    DrawingImmediate.Line(Vector2.new(20, 20), Vector2.new(300, 500), Color3.fromRGB(100, 20, 0), 1, 4)

    DrawingImmediate.Circle(Vector2.new(300, 500), 64, Color3.fromRGB(160, 50, 30), 0.7, 0, 0)
    DrawingImmediate.Circle(Vector2.new(400, 500), 64, Color3.fromRGB(160, 50, 30), 0.7, 64, 10)
    DrawingImmediate.FilledCircle(Vector2.new(410, 510), 64, Color3.fromRGB(160, 50, 30), 0.7, 64, 10)

    DrawingImmediate.Triangle(Vector2.new(500, 400), Vector2.new(450, 370), Vector2.new(480, 370), Color3.fromRGB(30, 80, 250), 0.6, 3)
    DrawingImmediate.FilledTriangle(Vector2.new(500, 400), Vector2.new(450, 370), Vector2.new(480, 370), Color3.fromRGB(30, 80, 250), 0.6, 3)

    DrawingImmediate.Rectangle(Vector2.new(300, 400), Vector2.new(500, 500), Color3.fromRGB(4, 100, 0), 1, 0.2, 1)
    DrawingImmediate.FilledRectangle(Vector2.new(300, 400), Vector2.new(500, 500), Color3.fromRGB(4, 100, 0), 1, 0.2, 1)

    DrawingImmediate.Quad(Vector2.new(40, 40), Vector2.new(60, 45), Vector2.new(60, 105), Vector2.new(30, 90), Color3.fromRGB(30, 150, 150), 0.75, 2)
    DrawingImmediate.FilledQuad(Vector2.new(40, 40), Vector2.new(60, 45), Vector2.new(60, 105), Vector2.new(30, 90), Color3.fromRGB(30, 150, 150), 0.75, 2)

    DrawingImmediate.Text(Vector2.new(200, 30), 1, 24, Color3.new(1, 1, 1), 1, "FOO", false)
    DrawingImmediate.Text(Vector2.new(200, 30), 1, 24, Color3.new(1, 1, 1), 1, "BAR", true)
    DrawingImmediate.OutlinedText(Vector2.new(200, 50), 1, 34, Color3.new(1, 1, 0.6), 1, Color3.new(), 1, "BAR", true)
end)

workspace.CurrentCamera:GetPropertyChangedSignal("ViewportSize"):Wait()

for _,v in connections do
    v:Disconnect()
end
