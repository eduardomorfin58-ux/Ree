preserves the existing menu/features, expands normal camera zoom distance, expands custom orbit, and adds mobile pinch zoom in orbit mode.
-- Startup guard: this UI must run on the client in a LocalScript.
local SYSTEM_CFG_RUN_SERVICE = game:GetService("RunService")
if not SYSTEM_CFG_RUN_SERVICE:IsClient() then
    warn("[SYSTEM CFG RGB] This script is client-only. Put it in StarterPlayer > StarterPlayerScripts as a LocalScript.")
    return
end

local SYSTEM_CFG_STARTUP_GUI = nil
-- v25: optional ServerScript companion inventories up to 150,000 server-side BaseParts, creates a spatially distributed snow plan, adds two mobile Tool:Activate autoclickers at 0.1s, and keeps a bounded local snow-render budget.
local SYSTEM_CFG_STAGE = "starting SYSTEM_CFG_START"
local function SYSTEM_CFG_START()
    SYSTEM_CFG_STAGE = "getting Roblox services"
    local Players = game:GetService("Players")
    local UserInputService = game:GetService("UserInputService")
    local RunService = game:GetService("RunService")
    local Lighting = game:GetService("Lighting")
    local SoundService = game:GetService("SoundService")
    local ReplicatedStorage = game:GetService("ReplicatedStorage")

    local mapBridge = ReplicatedStorage:FindFirstChild("SYSTEM_CFG_MapSnowBridge")
    task.spawn(function()
        if not mapBridge then
            mapBridge = ReplicatedStorage:WaitForChild("SYSTEM_CFG_MapSnowBridge", 12)
        end
    end)
    local serverBridgeFailed = false

    SYSTEM_CFG_STAGE = "getting LocalPlayer and PlayerGui"
    local player = Players.LocalPlayer
    if not player then error("SYSTEM CFG must run as a LocalScript; LocalPlayer is nil") end
    local playerGui = player:WaitForChild("PlayerGui", 15)
    if not playerGui then error("PlayerGui did not become available") end

    -- Visible boot marker: if startup stalls, the user can tell the LocalScript did run.
    local staleBoot = playerGui:FindFirstChild("SYSTEM_CFG_STARTING")
    if staleBoot then staleBoot:Destroy() end
    SYSTEM_CFG_STARTUP_GUI = Instance.new("ScreenGui")
    SYSTEM_CFG_STARTUP_GUI.Name = "SYSTEM_CFG_STARTING"
    SYSTEM_CFG_STARTUP_GUI.ResetOnSpawn = false
    SYSTEM_CFG_STARTUP_GUI.IgnoreGuiInset = true
    SYSTEM_CFG_STARTUP_GUI.DisplayOrder = 9998
    SYSTEM_CFG_STARTUP_GUI.Parent = playerGui
    local bootCard = Instance.new("Frame")
    bootCard.Name = "BootCard"
    bootCard.AnchorPoint = Vector2.new(0.5, 0.5)
    bootCard.Position = UDim2.fromScale(0.5, 0.5)
    bootCard.Size = UDim2.fromOffset(280, 64)
    bootCard.BackgroundColor3 = Color3.fromRGB(8, 10, 18)
    bootCard.BorderSizePixel = 0
    bootCard.Parent = SYSTEM_CFG_STARTUP_GUI
    Instance.new("UICorner", bootCard).CornerRadius = UDim.new(0, 10)
    local bootStroke = Instance.new("UIStroke")
    bootStroke.Color = Color3.fromRGB(0, 240, 255)
    bootStroke.Thickness = 1.5
    bootStroke.Parent = bootCard
    local bootLabel = Instance.new("TextLabel")
    bootLabel.Name = "Status"
    bootLabel.BackgroundTransparency = 1
    bootLabel.Size = UDim2.new(1, -20, 1, -12)
    bootLabel.Position = UDim2.fromOffset(10, 6)
    bootLabel.Font = Enum.Font.Code
    bootLabel.TextSize = 13
    bootLabel.TextWrapped = true
    bootLabel.TextColor3 = Color3.fromRGB(238, 246, 255)
    bootLabel.Text = "SYSTEM CFG RGB v28\nIniciando interfaz..."
    bootLabel.Parent = bootCard

    for _, guiName in ipairs({"SYSTEM_CFG_RGB", "SYSTEM_CFG", "SYSTEM_CFG_BOOT_ERROR", "SYSTEM_CFG_TEST"}) do
        local oldGui = playerGui:FindFirstChild(guiName)
        if oldGui then oldGui:Destroy() end
    end

    local C = {
        BG = Color3.fromRGB(5, 7, 14),
        PANEL = Color3.fromRGB(12, 16, 29),
        PANEL2 = Color3.fromRGB(18, 23, 39),
        CYAN = Color3.fromRGB(0, 240, 255),
        MAGENTA = Color3.fromRGB(255, 0, 127),
        WHITE = Color3.fromRGB(238, 246, 255),
        MUTED = Color3.fromRGB(132, 148, 169),
        GREEN = Color3.fromRGB(67, 255, 176),
        YELLOW = Color3.fromRGB(255, 225, 32),
        SNOW = Color3.fromRGB(239, 246, 255),
    }
    local crossColorPalette = {
        YELLOW = C.YELLOW, CYAN = C.CYAN, MAGENTA = C.MAGENTA,
        GREEN = C.GREEN, WHITE = C.WHITE,
    }

    -- Camera limits are intentionally finite to avoid unusable distances and unstable rendering.
    -- The default Roblox camera supports large zoom ranges; 5000 studs is a practical ceiling here.
    local MAX_CAMERA_ZOOM_DISTANCE = 5000
    local MAX_CAMERA_ORBIT_DISTANCE = 5000

    local state = {
        menuOpen = true,
        activeTab = "AIM",
        crosshair = true, rainbow = false, crossRotate = true, crossStyle = "ORBIT", crossColor = "YELLOW",
        crossX = 0, crossY = -34, crossRadius = 25, crossFrozen = false, aimButtonsVisible = true,
        crossLength = 12, crossThickness = 3, heartSize = 18, crossDotSize = 4,
        rotationSpeed = 1500, rainbowSpeed = 0.28,
        rifle = false, rifleRainbow = true, rifleX = 0, rifleY = 0,
        rifleSize = 52, rifleThickness = 2, rifleLength = 13,
        freeCamera = false, cameraOrbit = false, cameraOrbitDistance = 42, cameraSpeed = 35, fov = (workspace.CurrentCamera and workspace.CurrentCamera.FieldOfView) or 70,
        zoomMin = player.CameraMinZoomDistance, zoomMax = player.CameraMaxZoomDistance, cameraUnlocked = false, mapScanLimit = 150000,
        fpsVisible = true, fpsSize = 12, fpsLocked = true,
        fpsCorner = "TR", fpsRainbow = true,
        night = false, fog = false, fogDensity = 0.25, fogEnd = 450,
        snowfall = false, accumulation = false, frost = false,
        snowRate = 420, flakeSize = 0.12, accumulationSpeed = 1,
        maxSnowDepth = 0.06, snowOpacity = 0.18, surfaceBudget = UserInputService.TouchEnabled and 12000 or 20000,
        snowProgress = 0, snowCoverCount = 0,
        avatarOpen = false, avatarWidth = 250, avatarHeight = 300,
        avatarCorner = "TR", avatarZoom = 10, avatarYaw = 0, avatarYawCurrent = 0,
        streamerMode = false, photoMode = false,
        colorFX = false, bloomFX = false, sunRaysFX = false, dofFX = false,
        colorBrightness = 3, colorContrast = 12, colorSaturation = 18,
        bloomIntensity = 35, raysIntensity = 8, dofFocus = 20,
        moonSize = 32, moonGlow = 55,
        audioVolume = 100,
        autoClickA = false, autoClickB = false, autoClickButtonsVisible = true, autoClickInterval = 0.1,
    }

    -- Client-only volume scale for Sound instances, remembering original values for restoration.
    local soundOriginalVolumes = setmetatable({}, {__mode = "k"})
    local globalConnections = {}
    local function globalConnect(signal, callback)
        local connection = signal:Connect(callback)
        table.insert(globalConnections, connection)
        return connection
    end
    local function registerSound(instance)
        if instance and instance:IsA("Sound") and soundOriginalVolumes[instance] == nil then
            soundOriginalVolumes[instance] = instance.Volume
            pcall(function() instance.Volume = soundOriginalVolumes[instance] * (state.audioVolume / 100) end)
        end
    end
    -- Scan existing sounds after the UI boot has completed, in small batches.
    task.defer(function()
        local descendants = game:GetDescendants()
        for index, instance in ipairs(descendants) do
            if instance:IsA("Sound") then registerSound(instance) end
            if index % 300 == 0 then task.wait() end
        end
    end)
    globalConnect(game.DescendantAdded, function(instance)
        if instance:IsA("Sound") then
            task.defer(function()
                if instance.Parent then registerSound(instance) end
            end)
        end
    end)
    local function setLocalAudioVolume(value)
        state.audioVolume = math.clamp(value, 0, 100)
        local multiplier = state.audioVolume / 100
        for sound, originalVolume in pairs(soundOriginalVolumes) do
            if sound and sound.Parent then
                pcall(function() sound.Volume = originalVolume * multiplier end)
            end
        end
    end

    local function new(className, properties, parent)
        local object = Instance.new(className)
        for key, value in pairs(properties or {}) do
            object[key] = value
        end
        object.Parent = parent
        return object
    end

    local function addCorner(object, radius)
        return new("UICorner", {CornerRadius = UDim.new(0, radius or 8)}, object)
    end

    local function addStroke(object, color, thickness, transparency)
        return new("UIStroke", {
            Color = color or C.CYAN,
            Thickness = thickness or 1,
            Transparency = transparency or 0,
            ApplyStrokeMode = Enum.ApplyStrokeMode.Border,
        }, object)
    end

    local function addText(parent, value, size, color, font)
        return new("TextLabel", {
            BackgroundTransparency = 1,
            Text = value,
            TextColor3 = color or C.WHITE,
            TextSize = size or 12,
            Font = font or Enum.Font.Code,
            TextWrapped = true,
            BorderSizePixel = 0,
        }, parent)
    end

    local function addButton(parent, label, size)
        local button = new("TextButton", {
            Size = size or UDim2.new(1, 0, 0, 30),
            BackgroundColor3 = C.PANEL2,
            BorderSizePixel = 0,
            Text = label,
            TextColor3 = C.WHITE,
            TextSize = 12,
            Font = Enum.Font.Code,
            TextWrapped = true,
            AutoButtonColor = true,
            Active = true,
        }, parent)
        addCorner(button, 6)
        addStroke(button, Color3.fromRGB(45, 111, 137), 1, 0.1)
        return button
    end

    SYSTEM_CFG_STAGE = "creating main ScreenGui"
    local gui = new("ScreenGui", {
        Name = "SYSTEM_CFG_RGB",
        ResetOnSpawn = false,
        IgnoreGuiInset = false,
        DisplayOrder = 999,
        ZIndexBehavior = Enum.ZIndexBehavior.Sibling,
    }, playerGui)

    local settings = {
        aimVisible = true,
        launcherVisible = true,
        fpsVisible = true,
        avatarVisible = false,
        camControlsVisible = false,
    }

    local panel = new("Frame", {
        Name = "MainPanel",
        Size = UDim2.fromOffset(300, 390),
        Position = UDim2.new(0, 66, 0.5, -195),
        BackgroundColor3 = C.BG,
        BorderSizePixel = 0,
        ClipsDescendants = true,
        ZIndex = 20,
    }, gui)
    addCorner(panel, 10)
    addStroke(panel, C.CYAN, 1.5, 0)

    local header = new("Frame", {
        Name = "DragHeader",
        Size = UDim2.new(1, 0, 0, 40),
        BackgroundColor3 = C.PANEL,
        BorderSizePixel = 0,
        Active = true,
        ZIndex = 21,
    }, panel)
    addCorner(header, 9)
    local title = addText(header, "SYSTEM CFG  /  RGB v25", 15, C.CYAN, Enum.Font.Code)
    title.Position = UDim2.new(0, 10, 0, 0)
    title.Size = UDim2.new(1, -50, 1, 0)
    title.TextXAlignment = Enum.TextXAlignment.Left
    title.TextYAlignment = Enum.TextYAlignment.Center

    local closeButton = addButton(header, "Ã", UDim2.fromOffset(29, 27))
    closeButton.Name = "CloseButton"
    closeButton.Position = UDim2.new(1, -34, 0, 6)
    closeButton.BackgroundColor3 = Color3.fromRGB(40, 12, 31)
    closeButton.TextColor3 = C.MAGENTA
    closeButton.TextSize = 19
    closeButton.ZIndex = 22
    addStroke(closeButton, C.MAGENTA, 1)

    local tabBar = new("Frame", {
        Name = "TabBar",
        Position = UDim2.new(0, 7, 0, 47),
        Size = UDim2.new(1, -14, 0, 31),
        BackgroundTransparency = 1,
        ZIndex = 21,
    }, panel)
    local tabLayout = new("UIListLayout", {
        FillDirection = Enum.FillDirection.Horizontal,
        SortOrder = Enum.SortOrder.LayoutOrder,
        Padding = UDim.new(0, 4),
    }, tabBar)

    local content = new("ScrollingFrame", {
        Name = "PageContent",
        Position = UDim2.new(0, 7, 0, 85),
        Size = UDim2.new(1, -14, 1, -92),
        BackgroundColor3 = C.PANEL,
        BackgroundTransparency = 0,
        BorderSizePixel = 0,
        ScrollBarThickness = 3,
        ScrollBarImageColor3 = C.MAGENTA,
        AutomaticCanvasSize = Enum.AutomaticSize.Y,
        CanvasSize = UDim2.new(),
        ZIndex = 21,
    }, panel)
    addCorner(content, 6)
    local contentPadding = new("UIPadding", {
        PaddingTop = UDim.new(0, 7),
        PaddingBottom = UDim.new(0, 7),
        PaddingLeft = UDim.new(0, 7),
        PaddingRight = UDim.new(0, 7),
    }, content)
    local contentLayout = new("UIListLayout", {
        Padding = UDim.new(0, 5),
        SortOrder = Enum.SortOrder.LayoutOrder,
    }, content)

    local launcher = addButton(gui, "CFG", UDim2.fromOffset(48, 38))
    launcher.Name = "CFG_Launcher"
    launcher.Position = UDim2.new(0, 12, 0.5, -19)
    launcher.BackgroundColor3 = Color3.fromRGB(25, 9, 39)
    launcher.TextColor3 = C.MAGENTA
    launcher.TextSize = 15
    launcher.ZIndex = 50
    addStroke(launcher, C.MAGENTA, 1.5)

    local mainAim = new("Frame", {
        Name = "MainCrosshair",
        AnchorPoint = Vector2.new(0.5, 0.5),
        Position = UDim2.new(0.5, state.crossX, 0.5, state.crossY),
        Size = UDim2.fromOffset(110, 110),
        BackgroundTransparency = 1,
        Active = false,
        ZIndex = 5,
    }, gui)
    local heart = addText(mainAim, "â¥", state.heartSize, C.YELLOW, Enum.Font.GothamBold)
    heart.AnchorPoint = Vector2.new(0.5, 0.5)
    heart.Position = UDim2.fromScale(0.5, 0.5)
    heart.Size = UDim2.fromOffset(30, 30)
    heart.ZIndex = 6
    heart.TextStrokeTransparency = 0.25
    heart.TextStrokeColor3 = C.YELLOW

    -- Optional circular reticle and center point. The heart remains fixed in ORBIT mode.
    local crossRing = new("Frame", {
        Name = "CrosshairRing", AnchorPoint = Vector2.new(0.5, 0.5),
        Position = UDim2.fromScale(0.5, 0.5), Size = UDim2.fromOffset(state.crossRadius * 2, state.crossRadius * 2),
        BackgroundTransparency = 1, BorderSizePixel = 0, Visible = false, ZIndex = 4,
    }, mainAim)
    addCorner(crossRing, 100)
    local crossRingStroke = addStroke(crossRing, C.CYAN, 2, 0.05)
    local crossCenterDot = new("Frame", {
        Name = "CrosshairCenterDot", AnchorPoint = Vector2.new(0.5, 0.5),
        Position = UDim2.fromScale(0.5, 0.5), Size = UDim2.fromOffset(4, 4),
        BackgroundColor3 = C.CYAN, BorderSizePixel = 0, Visible = false, ZIndex = 8,
    }, mainAim)
    addCorner(crossCenterDot, 4)

    local aimLines = {}
    for i = 1, 8 do
        local line = new("Frame", {
            Name = "AimLine" .. i,
            AnchorPoint = Vector2.new(0.5, 0.5),
            Position = UDim2.fromScale(0.5, 0.5),
            Size = UDim2.fromOffset(state.crossThickness, state.crossLength),
            BackgroundColor3 = C.YELLOW,
            BorderSizePixel = 0,
            ZIndex = 5,
        }, mainAim)
        addCorner(line, 2)
        table.insert(aimLines, line)
    end

    -- One shared layout definition drives BOTH the in-game reticle and its 3D preview.
    -- This prevents the preview from drifting away from the selected on-screen shape.
    local CROSSHAIR_STYLES = {
        {id = "ORBIT", label = "01 ORBIT"},
        {id = "RING", label = "02 RING"},
        {id = "DALLAS", label = "03 CLASSIC"},
        {id = "X", label = "04 X / DIAGONAL"},
        {id = "DOT", label = "05 DOT"},
        {id = "DIAMOND", label = "06 DIAMOND"},
        {id = "TACTICAL", label = "07 TACTICAL"},
        {id = "OCTA", label = "08 OCTA"},
        {id = "CHEVRON", label = "09 CHEVRON"},
        {id = "BOX", label = "10 BOX"},
    }

    local function getCrosshairLayout(style, radius, length, phaseDegrees)
        local layout = {segments = {}, ring = false, dot = false, heart = false}
        local r = math.max(3, radius or state.crossRadius)
        local L = math.max(2, length or state.crossLength)
        local phase = math.rad(phaseDegrees or 0)
        local function segment(x, y, rotationDegrees, lengthScale, thicknessScale)
            table.insert(layout.segments, {
                x = x, y = y, rotation = rotationDegrees,
                lengthScale = lengthScale or 1,
                thicknessScale = thicknessScale or 1,
            })
        end
        local function radial(count, offsetDegrees, radiusScale, lengthScale, rotateByPhase)
            for i = 0, count - 1 do
                local a = math.rad(offsetDegrees or 0) + i * (math.pi * 2 / count)
                if rotateByPhase then a += phase end
                segment(math.cos(a) * r * (radiusScale or 1), math.sin(a) * r * (radiusScale or 1), math.deg(a) + 90, lengthScale)
            end
        end

        if style == "ORBIT" then
            layout.heart = true
            radial(6, 0, 1, 1, true)
        elseif style == "RING" then
            layout.ring, layout.dot = true, true
            radial(4, 0, 1, 0.8, true)
        elseif style == "DALLAS" then
            layout.dot = true
            radial(4, 0, 1, 1, false)
        elseif style == "X" then
            radial(4, 45, 1, 1, false)
        elseif style == "DOT" then
            layout.dot = true
        elseif style == "DIAMOND" then
            layout.dot = true
            radial(4, 45, 0.72, 0.72, false)
        elseif style == "TACTICAL" then
            layout.ring, layout.dot = true, true
            radial(4, 0, 0.88, 0.58, false)
        elseif style == "OCTA" then
            layout.dot = true
            radial(8, 0, 1, 0.62, false)
        elseif style == "CHEVRON" then
            -- Four short angled bars form opposing < and > brackets.
            segment(-r * 0.55, -r * 0.25, 45, 0.72)
            segment(-r * 0.55,  r * 0.25, 135, 0.72)
            segment( r * 0.55, -r * 0.25, 135, 0.72)
            segment( r * 0.55,  r * 0.25, 45, 0.72)
            layout.dot = true
        elseif style == "BOX" then
            -- Eight pieces create four open square corners rather than a generic shape.
            for _, sx in ipairs({-1, 1}) do
                for _, sy in ipairs({-1, 1}) do
                    segment(sx * r * 0.58, sy * r, 90, 0.55)
                    segment(sx * r, sy * r * 0.58, 0, 0.55)
                end
            end
        else
            -- Safe fallback for unknown saved style names.
            layout.dot = true
            radial(4, 0, 1, 1, false)
        end
        return layout
    end

    local crosshairPreviewParts = {}
    local crosshairPreviewWorld, crosshairPreviewCamera, crosshairPreviewStatus, crosshairPreviewHeartLabel

    local rifleAim = new("Frame", {
        Name = "RifleCrosshair",
        AnchorPoint = Vector2.new(0.5, 0.5),
        Position = UDim2.new(0.5, state.rifleX, 0.5, state.rifleY),
        Size = UDim2.fromOffset(state.rifleSize, state.rifleSize),
        BackgroundTransparency = 1,
        Active = true,
        Visible = state.rifle,
        ZIndex = 7,
    }, gui)
    local rifleLines = {}
    for i = 1, 4 do
        local line = new("Frame", {
            Name = "RifleLine" .. i,
            AnchorPoint = Vector2.new(0.5, 0.5),
            Position = UDim2.fromScale(0.5, 0.5),
            Size = UDim2.fromOffset(state.rifleThickness, state.rifleLength),
            BackgroundColor3 = C.CYAN,
            BorderSizePixel = 0,
            Active = false,
            ZIndex = 7,
        }, rifleAim)
        addCorner(line, 2)
        table.insert(rifleLines, line)
    end

    local fpsWidget = new("TextLabel", {
        Name = "FPSMonitor",
        AnchorPoint = Vector2.new(1, 0),
        Position = UDim2.new(1, -12, 0, 8),
        Size = UDim2.fromOffset(160, 20),
        BackgroundTransparency = 1,
        Text = "FPS -- | MIN -- | MAX --",
        TextColor3 = C.CYAN,
        TextStrokeTransparency = 0.4,
        TextSize = state.fpsSize,
        Font = Enum.Font.Code,
        TextXAlignment = Enum.TextXAlignment.Right,
        Active = true,
        Visible = true,
        ZIndex = 10,
    }, gui)

    -- Floating preview window; hidden until opened in AVATAR tab.
    local avatarWindow = new("Frame", {
        Name = "AvatarPreviewWindow",
        Size = UDim2.fromOffset(state.avatarWidth, state.avatarHeight),
        Position = UDim2.new(1, -state.avatarWidth - 18, 0, 45),
        BackgroundColor3 = C.BG,
        BorderSizePixel = 0,
        Visible = false,
        ClipsDescendants = true,
        ZIndex = 30,
    }, gui)
    addCorner(avatarWindow, 9)
    addStroke(avatarWindow, C.MAGENTA, 1.5)
    local avatarHeader = new("Frame", {
        Name = "AvatarHeader",
        Size = UDim2.new(1, 0, 0, 32),
        BackgroundColor3 = C.PANEL,
        BorderSizePixel = 0,
        Active = true,
        ZIndex = 31,
    }, avatarWindow)
    addCorner(avatarHeader, 8)
    local avatarTitle = addText(avatarHeader, "AVATAR_PREVIEW  /  3D", 11, C.CYAN)
    avatarTitle.Position = UDim2.new(0, 8, 0, 0)
    avatarTitle.Size = UDim2.new(1, -37, 1, 0)
    avatarTitle.TextXAlignment = Enum.TextXAlignment.Left
    local avatarClose = addButton(avatarHeader, "Ã", UDim2.fromOffset(25, 24))
    avatarClose.Position = UDim2.new(1, -28, 0, 4)
    avatarClose.ZIndex = 32
    local avatarViewport = new("ViewportFrame", {
        Name = "AvatarViewport",
        Position = UDim2.new(0, 4, 0, 36),
        Size = UDim2.new(1, -8, 1, -40),
        BackgroundColor3 = Color3.fromRGB(8, 12, 24),
        BackgroundTransparency = 0,
        BorderSizePixel = 0,
        LightColor = Color3.fromRGB(255, 255, 255),
        LightDirection = Vector3.new(-1, -1, -1),
        Ambient = Color3.fromRGB(150, 160, 180),
        Active = true,
        ZIndex = 31,
    }, avatarWindow)
    addCorner(avatarViewport, 6)
    local avatarWorld = new("WorldModel", {Name = "AvatarWorld"}, avatarViewport)
    local avatarCamera = new("Camera", {Name = "AvatarCamera"}, avatarViewport)
    avatarViewport.CurrentCamera = avatarCamera
    local avatarModel = nil
    local avatarPartPairs = {}
    local avatarDistance = 10
    local avatarBasePosition = Vector3.new(0, 0, 0)
    local avatarCenterLocal = Vector3.new(0, 2, 0)
    local avatarYawCurrent = 0

    local function setAvatarCamera()
        avatarCamera.FieldOfView = 38
        avatarDistance = math.clamp(avatarDistance, 3.5, 28)
        avatarCamera.CFrame = CFrame.lookAt(Vector3.new(0, 0.2, avatarDistance), Vector3.zero)
    end
    setAvatarCamera()

    SYSTEM_CFG_STAGE = "building camera controls and visual effects"
    -- Camera touch controls. They appear only while free camera is active on touch devices.
    local camPad = new("Frame", {
        Name = "FreeCameraTouchPad",
        Size = UDim2.fromOffset(154, 142),
        Position = UDim2.new(0, 12, 1, -154),
        BackgroundColor3 = C.BG,
        BackgroundTransparency = 0.12,
        BorderSizePixel = 0,
        Visible = false,
        ZIndex = 40,
    }, gui)
    addCorner(camPad, 9)
    addStroke(camPad, C.CYAN, 1)
    local camPadTitle = addText(camPad, "FREE CAM", 10, C.CYAN)
    camPadTitle.Position = UDim2.new(0, 5, 0, 1)
    camPadTitle.Size = UDim2.new(1, -10, 0, 17)

    local movementFlags = {Forward = false, Back = false, Left = false, Right = false,
        Up = false, Down = false, YawLeft = false, YawRight = false, PitchUp = false, PitchDown = false}
    local camButtonSpecs = {
        {"â²", "Forward", 52, 24}, {"â", "Left", 20, 48}, {"â¼", "Back", 52, 48}, {"â¶", "Right", 84, 48},
        {"UP", "Up", 114, 24}, {"DN", "Down", 114, 48}, {"â²", "YawLeft", 20, 78}, {"â³", "YawRight", 52, 78},
        {"â", "PitchUp", 84, 78}, {"â", "PitchDown", 114, 78},
    }
    for _, spec in ipairs(camButtonSpecs) do
        local b = addButton(camPad, spec[1], UDim2.fromOffset(29, 24))
        b.Position = UDim2.fromOffset(spec[3], spec[4])
        b.ZIndex = 41
        local key = spec[2]
        b.InputBegan:Connect(function(input)
            if input.UserInputType == Enum.UserInputType.MouseButton1 or input.UserInputType == Enum.UserInputType.Touch then
                movementFlags[key] = true
            end
        end)
        b.InputEnded:Connect(function(input)
            if input.UserInputType == Enum.UserInputType.MouseButton1 or input.UserInputType == Enum.UserInputType.Touch then
                movementFlags[key] = false
            end
        end)
    end

    -- World effects: save original lighting, create only our own Atmosphere/effects.
    local originalLighting = {
        ClockTime = Lighting.ClockTime,
        Brightness = Lighting.Brightness,
        Ambient = Lighting.Ambient,
        OutdoorAmbient = Lighting.OutdoorAmbient,
        ExposureCompensation = Lighting.ExposureCompensation,
        FogStart = Lighting.FogStart,
        FogEnd = Lighting.FogEnd,
        FogColor = Lighting.FogColor,
    }

    local function ownedLightingEffect(className, name)
        local oldEffect = Lighting:FindFirstChild(name)
        if oldEffect then oldEffect:Destroy() end
        local effect = new(className, {Name = name}, Lighting)
        if effect:IsA("PostEffect") then effect.Enabled = false end
        return effect
    end

    local fogAtmosphere = ownedLightingEffect("Atmosphere", "SYSTEM_CFG_Atmosphere")
    fogAtmosphere.Parent = nil
    fogAtmosphere.Density = state.fogDensity
    fogAtmosphere.Haze = 1.5
    fogAtmosphere.Color = Color3.fromRGB(190, 205, 235)
    fogAtmosphere.Decay = Color3.fromRGB(70, 80, 120)
    fogAtmosphere.Glare = 0

    local nightCC = ownedLightingEffect("ColorCorrectionEffect", "SYSTEM_CFG_NightCC")
    nightCC.Brightness = -0.04
    nightCC.Contrast = 0.08
    nightCC.Saturation = -0.06
    nightCC.TintColor = Color3.fromRGB(220, 225, 255)

    local frostCC = ownedLightingEffect("ColorCorrectionEffect", "SYSTEM_CFG_FrostCC")
    frostCC.Brightness = -0.015
    frostCC.Contrast = 0.07
    frostCC.Saturation = -0.12
    frostCC.TintColor = Color3.fromRGB(205, 225, 255)

    local colorEffect = ownedLightingEffect("ColorCorrectionEffect", "SYSTEM_CFG_ColorFX")
    colorEffect.Brightness = 0.03
    colorEffect.Contrast = 0.12
    colorEffect.Saturation = 0.18
    colorEffect.TintColor = Color3.fromRGB(220, 235, 255)

    local bloomEffect = ownedLightingEffect("BloomEffect", "SYSTEM_CFG_Bloom")
    bloomEffect.Intensity = 0.35
    bloomEffect.Size = 24
    bloomEffect.Threshold = 1.1

    local sunRaysEffect = ownedLightingEffect("SunRaysEffect", "SYSTEM_CFG_SunRays")
    sunRaysEffect.Intensity = 0.08
    sunRaysEffect.Spread = 0.7

    local dofEffect = ownedLightingEffect("DepthOfFieldEffect", "SYSTEM_CFG_DepthOfField")
    dofEffect.FarIntensity = 0.1
    dofEffect.FocusDistance = 20
    dofEffect.InFocusRadius = 30
    dofEffect.NearIntensity = 0.05

    -- Local moon model with face-oriented crater details. All parts are local-only.
    local moonFolder = new("Folder", {Name = "SYSTEM_CFG_LocalMoon"}, workspace)
    local moon = new("Part", {
        Name = "MoonDisc", Shape = Enum.PartType.Ball,
        Size = Vector3.new(state.moonSize, state.moonSize, state.moonSize),
        Material = Enum.Material.SmoothPlastic,
        Color = Color3.fromRGB(225, 229, 239),
        Anchored = true, CanCollide = false, CanTouch = false, CanQuery = false,
        CastShadow = false, Transparency = 0,
    }, moonFolder)
    local moonLight = new("PointLight", {
        Name = "MoonGlow", Color = Color3.fromRGB(190, 211, 255),
        Brightness = state.moonGlow / 55, Range = 320, Shadows = false,
    }, moon)
    local moonHalo = new("Part", {
        Name = "MoonSoftHalo", Shape = Enum.PartType.Ball,
        Size = Vector3.new(state.moonSize * 1.48, state.moonSize * 1.48, state.moonSize * 1.48),
        Material = Enum.Material.Neon, Color = Color3.fromRGB(105, 149, 255),
        Transparency = 0.94, Anchored = true, CanCollide = false, CanTouch = false,
        CanQuery = false, CastShadow = false,
    }, moonFolder)
    local moonRim = new("Part", {
        Name = "MoonRimGlow", Shape = Enum.PartType.Ball,
        Size = Vector3.new(state.moonSize * 1.12, state.moonSize * 1.12, state.moonSize * 1.12),
        Material = Enum.Material.Neon, Color = Color3.fromRGB(170, 194, 255),
        Transparency = 0.90, Anchored = true, CanCollide = false, CanTouch = false,
        CanQuery = false, CastShadow = false,
    }, moonFolder)
    local moonCraterPositions = {
        Vector3.new(-4.8, 5.0, -1), Vector3.new(3.2, 6.0, -1), Vector3.new(6.8, 2.0, -1),
        Vector3.new(-7.1, 0.6, -1), Vector3.new(-2.0, 1.5, -1), Vector3.new(1.2, 2.9, -1),
        Vector3.new(4.5, -2.8, -1), Vector3.new(-4.2, -3.5, -1), Vector3.new(0.5, -5.9, -1),
        Vector3.new(7.3, -4.2, -1), Vector3.new(-7.6, -5.3, -1), Vector3.new(-1.1, 7.8, -1),
        Vector3.new(2.1, -0.8, -1), Vector3.new(5.1, 4.4, -1), Vector3.new(-5.5, 2.7, -1),
        Vector3.new(0.0, -2.4, -1), Vector3.new(-2.8, -7.6, -1), Vector3.new(8.2, 0.1, -1),
    }
    local moonDirection = Vector3.new(0, 0, -1)
    for i, offset in ipairs(moonCraterPositions) do
        local scale = state.moonSize / 32
        local w = (i % 4 == 0 and 3.0 or (i % 3 == 0 and 2.35 or 1.75)) * scale
        local h = (i % 4 == 0 and 2.1 or (i % 3 == 0 and 1.8 or 1.35)) * scale
        local crater = new("Part", {
            Name = "Crater" .. i, Shape = Enum.PartType.Ball,
            Size = Vector3.new(w, h, 0.24 * scale),
            Material = Enum.Material.SmoothPlastic,
            Color = (i % 3 == 0) and Color3.fromRGB(139, 146, 163)
                or ((i % 2 == 0) and Color3.fromRGB(161, 168, 185) or Color3.fromRGB(176, 183, 199)),
            Transparency = 0.08,
            Anchored = true, CanCollide = false, CanTouch = false, CanQuery = false,
            CastShadow = false,
        }, moonFolder)
        crater:SetAttribute("LocalMoonOffset", offset)
        crater:SetAttribute("BaseCraterSize", Vector3.new(w, h, 0.24 * scale))
    end
    moonFolder.Parent = nil

    local function applyNight(on)
        state.night = on
        nightCC.Enabled = on
        if on then
            local camera = workspace.CurrentCamera
            if camera then
                local look = camera.CFrame.LookVector
                local horizontal = Vector3.new(look.X, 0, look.Z)
                if horizontal.Magnitude > 0.1 then moonDirection = horizontal.Unit end
            end
            Lighting.ClockTime = 0
            Lighting.Brightness = math.max(1.5, originalLighting.Brightness * 0.8)
            Lighting.Ambient = Color3.fromRGB(88, 91, 130)
            Lighting.OutdoorAmbient = Color3.fromRGB(62, 67, 110)
            Lighting.ExposureCompensation = math.max(-0.1, originalLighting.ExposureCompensation)
            moonFolder.Parent = workspace
        else
            Lighting.ClockTime = originalLighting.ClockTime
            Lighting.Brightness = originalLighting.Brightness
            Lighting.Ambient = originalLighting.Ambient
            Lighting.OutdoorAmbient = originalLighting.OutdoorAmbient
            Lighting.ExposureCompensation = originalLighting.ExposureCompensation
            moonFolder.Parent = nil
        end
    end

    local function applyFog(on)
        state.fog = on
        fogAtmosphere.Parent = on and Lighting or nil
        if on then
            fogAtmosphere.Density = state.fogDensity
            fogAtmosphere.Haze = math.clamp(state.fogDensity * 6, 0, 8)
            Lighting.FogStart = 0
            Lighting.FogEnd = state.fogEnd
            Lighting.FogColor = Color3.fromRGB(170, 180, 215)
        else
            Lighting.FogStart = originalLighting.FogStart
            Lighting.FogEnd = originalLighting.FogEnd
            Lighting.FogColor = originalLighting.FogColor
        end
    end

    -- Snow particle volume and thin cover planes, entirely local and removable.
    local weatherFolder = new("Folder", {Name = "SYSTEM_CFG_WeatherFX"}, workspace)
    local emitterAnchor = new("Part", {
        Name = "SnowEmitterAnchor", Size = Vector3.new(180, 4, 180),
        Transparency = 1, Anchored = true, CanCollide = false, CanTouch = false,
        CanQuery = false, CastShadow = false,
    }, weatherFolder)
    local snowEmitter = new("ParticleEmitter", {
        Name = "FallingSnow",
        Enabled = false,
        Texture = "rbxasset://textures/particles/sparkles_main.dds",
        Rate = state.snowRate,
        Lifetime = NumberRange.new(4.0, 7.0),
        Speed = NumberRange.new(11, 18),
        Acceleration = Vector3.new(0, -3.5, 0),
        SpreadAngle = Vector2.new(180, 180),
        EmissionDirection = Enum.NormalId.Bottom,
        Rotation = NumberRange.new(0, 360),
        RotSpeed = NumberRange.new(-35, 35),
        Size = NumberSequence.new(state.flakeSize),
        Transparency = NumberSequence.new({NumberSequenceKeypoint.new(0, 0.08), NumberSequenceKeypoint.new(1, 0.35)}),
        LightEmission = 0.1,
        LockedToPart = false,
    }, emitterAnchor)
    pcall(function() snowEmitter.Shape = Enum.ParticleEmitterShape.Box end)
    pcall(function() snowEmitter.ShapeSize = Vector3.new(170, 2, 170) end)

    -- Rainbow stars are emitted near the view direction only at night when the camera points upward.
    local starAnchor = new("Part", {
        Name = "RainbowStarAnchor", Size = Vector3.new(1, 1, 1),
        Transparency = 1, Anchored = true, CanCollide = false, CanTouch = false,
        CanQuery = false, CastShadow = false,
    }, weatherFolder)
    local starAttachment = new("Attachment", {Name = "RainbowStarAttachment"}, starAnchor)
    local starEmitter = new("ParticleEmitter", {
        Name = "RainbowNightStars", Enabled = false,
        Texture = "rbxasset://textures/particles/sparkles_main.dds",
        Rate = 55, Lifetime = NumberRange.new(3.5, 5.5),
        Speed = NumberRange.new(0, 0.15), Acceleration = Vector3.zero,
        SpreadAngle = Vector2.new(180, 180), EmissionDirection = Enum.NormalId.Bottom,
        Rotation = NumberRange.new(0, 360), RotSpeed = NumberRange.new(-8, 8),
        Size = NumberSequence.new({
            NumberSequenceKeypoint.new(0, 0.10),
            NumberSequenceKeypoint.new(0.55, 0.16),
            NumberSequenceKeypoint.new(1, 0.04),
        }),
        Color = ColorSequence.new({
            ColorSequenceKeypoint.new(0.00, Color3.fromRGB(255, 0, 80)),
            ColorSequenceKeypoint.new(0.17, Color3.fromRGB(255, 185, 0)),
            ColorSequenceKeypoint.new(0.34, Color3.fromRGB(80, 255, 80)),
            ColorSequenceKeypoint.new(0.51, Color3.fromRGB(0, 245, 255)),
            ColorSequenceKeypoint.new(0.68, Color3.fromRGB(70, 80, 255)),
            ColorSequenceKeypoint.new(0.85, Color3.fromRGB(255, 0, 255)),
            ColorSequenceKeypoint.new(1.00, Color3.fromRGB(255, 0, 80)),
        }),
        Transparency = NumberSequence.new({
            NumberSequenceKeypoint.new(0, 0.18),
            NumberSequenceKeypoint.new(0.82, 0.15),
            NumberSequenceKeypoint.new(1, 0.7),
        }),
        LightEmission = 1, LockedToPart = true,
    }, starAttachment)
    pcall(function() starEmitter.Shape = Enum.ParticleEmitterShape.Box end)
    pcall(function() starEmitter.ShapeSize = Vector3.new(130, 90, 2) end)

    local coverFolder = new("Folder", {Name = "SYSTEM_CFG_SnowCovers"}, workspace)
    local surfaceRecords = {}
    local partSnowRecords = setmetatable({}, {__mode = "k"})
    local terrainCellKeys = {}
    local terrainGridStep, terrainGridOriginX, terrainGridOriginZ = nil, nil, nil
    local scanRunning = false
    local scanRequested = false
    local scanDebouncePending = false
    local terrainCoverCount = 0
    local currentSnowDepth = 0.012
    local frostProgress = 0
    local surfaceStatusLabel = nil
    local mapPartCache = setmetatable({}, {__mode = "k"})
    local partTileCache = setmetatable({}, {__mode = "k"})
    local cachedMapPartCount = 0

    -- Reused raycast settings. Always test the first visible surface in the scene, excluding only
    -- this script's visual effects and characters. This prevents snow on Terrain from appearing
    -- through roofs, platforms, and other map parts.
    local snowRaycastParams = RaycastParams.new()
    snowRaycastParams.FilterType = Enum.RaycastFilterType.Exclude
    snowRaycastParams.IgnoreWater = true
    local function refreshSnowRaycastExclusions()
        local excluded = {weatherFolder, coverFolder, moonFolder}
        for _, otherPlayer in ipairs(Players:GetPlayers()) do
            if otherPlayer.Character then table.insert(excluded, otherPlayer.Character) end
        end
        snowRaycastParams.FilterDescendantsInstances = excluded
    end
    refreshSnowRaycastExclusions()
    globalConnect(Players.PlayerAdded, function(otherPlayer)
        globalConnect(otherPlayer.CharacterAdded, function() task.defer(refreshSnowRaycastExclusions) end)
        task.defer(refreshSnowRaycastExclusions)
    end)
    for _, otherPlayer in ipairs(Players:GetPlayers()) do
        globalConnect(otherPlayer.CharacterAdded, function() task.defer(refreshSnowRaycastExclusions) end)
    end
    globalConnect(Players.PlayerRemoving, function() task.defer(refreshSnowRaycastExclusions) end)

    -- A local, in-memory snapshot for the map preview. It is not a downloadable .rbxl file;
    -- ordinary in-game LocalScripts have no filesystem access and only see client-replicated geometry.
    local mapScanRunning = false
    local mapSnapshotReady = false
    local mapSnapshot = {}
    local mapSnapshotPartCount = 0
    local mapSnapshotTerrainCount = 0
    local mapPreviewWorld = nil
    local mapPreviewCamera = nil
    local mapPreviewStatus = nil
    local snowAfterMapScan = false
    local serverMapStatus = nil
    local serverSnowPlanCount = 0
    local serverSnowPlanFetched = false
    local serverSnowPlanLoading = false
    local serverPlansByPartKey = {}
    local serverTerrainPlans = {}
    local appliedServerSnowTiles = {}
    local serverTerrainCheckCursor = 1

    local function invokeMapBridge(action, ...)
        if serverBridgeFailed or not mapBridge or not mapBridge:IsA("RemoteFunction") then
            return nil, "map server bridge missing"
        end
        local args = table.pack(...)
        local ok, result = pcall(function()
            return mapBridge:InvokeServer(action, table.unpack(args, 1, args.n))
        end)
        if not ok then return nil, tostring(result) end
        return result
    end

    -- Must match SYSTEM_CFG_RGB_v25_Server.lua exactly. Small rounding absorbs replication precision.
    local function makeLoadedPartKey(part)
        local p, s, right, up = part.Position, part.Size, part.CFrame.RightVector, part.CFrame.UpVector
        return string.format("%s|%.2f|%.2f|%.2f|%.2f|%.2f|%.2f|%.2f|%.2f|%.2f|%.2f|%.2f|%.2f|%.2f",
            part.Name, p.X, p.Y, p.Z, s.X, s.Y, s.Z, right.X, right.Y, right.Z, up.X, up.Y, up.Z)
    end

    local function isCharacterPart(part)
        for _, p in ipairs(Players:GetPlayers()) do
            local character = p.Character
            if character and part:IsDescendantOf(character) then return true end
        end
        return false
    end

    local function isGeneratedVisual(part)
        return part:IsDescendantOf(weatherFolder)
            or part:IsDescendantOf(coverFolder)
            or part:IsDescendantOf(moonFolder)
    end

    local MAP_SCAN_HARD_LIMIT = 150000

    local function canIndexMapPart(part)
        -- Inventory all visible/queryable BaseParts, including moving parts, for the map count/preview.
        -- Actual snow is restricted separately to anchored, upward-facing surfaces.
        return part and part:IsA("BasePart") and part:IsDescendantOf(workspace)
            and not isCharacterPart(part) and not isGeneratedVisual(part)
            and part.Transparency < 0.95 and part.CanQuery
    end

    local function indexMapPart(part)
        if canIndexMapPart(part) and not mapPartCache[part]
            and cachedMapPartCount < math.min(MAP_SCAN_HARD_LIMIT, state.mapScanLimit or MAP_SCAN_HARD_LIMIT) then
            mapPartCache[part] = true
            cachedMapPartCount += 1
        end
    end

    local function unindexMapPart(part)
        if mapPartCache[part] then
            mapPartCache[part] = nil
            cachedMapPartCount = math.max(0, cachedMapPartCount - 1)
        end
    end

    local function canSnowCover(part)
        if not part or not part:IsA("BasePart") then return false end
        if not part:IsDescendantOf(workspace) or isCharacterPart(part) or isGeneratedVisual(part) then return false end
        if not part.Anchored or part.Transparency >= 0.95 or not part.CanQuery then return false end
        if part.Size.X < 1.5 or part.Size.Z < 1.5 then return false end
        -- Snow is only placed on upward-facing, near-horizontal surfaces.
        if part.CFrame.UpVector.Y < 0.88 then return false end
        if part:FindFirstAncestorOfClass("Tool") then return false end
        return true
    end

    local function updateSnowStatus()
        if surfaceStatusLabel and surfaceStatusLabel.Parent then
            local limitNote = #surfaceRecords >= state.surfaceBudget and " | LÃMITE DE NIEVE" or ""
            surfaceStatusLabel.Text = string.format("Mapa: %d/%d piezas | Nieve: %d | Terreno: %d%s",
                cachedMapPartCount, state.mapScanLimit or MAP_SCAN_HARD_LIMIT, #surfaceRecords - terrainCoverCount, terrainCoverCount, limitNote)
        end
    end

    local function makeSnowCover(surfaceCFrame, width, depth, isTerrain, sourcePart, uniqueKey)
        if #surfaceRecords >= state.surfaceBudget then return false end
        if uniqueKey and appliedServerSnowTiles[uniqueKey] then return false end
        width = math.max(0.25, width)
        depth = math.max(0.25, depth)
        local thickness = math.clamp(currentSnowDepth, 0.006, state.maxSnowDepth)
        local cover = new("Part", {
            Name = isTerrain and "TerrainSnowLayer" or "SurfaceSnowLayer",
            Size = Vector3.new(width, thickness, depth),
            CFrame = surfaceCFrame * CFrame.new(0, thickness / 2 + 0.00025, 0),
            Material = Enum.Material.Snow,
            Color = state.frost and Color3.fromRGB(210, 226, 255) or C.SNOW,
            Transparency = math.clamp(state.snowOpacity, 0, 0.85),
            Reflectance = state.frost and 0.08 or 0,
            Anchored = true, CanCollide = false, CanTouch = false, CanQuery = false, CastShadow = false,
        }, coverFolder)
        local record = {
            cover = cover, surfaceCFrame = surfaceCFrame, width = width, depth = depth,
            isTerrain = isTerrain, sourcePart = sourcePart, planKey = uniqueKey,
        }
        table.insert(surfaceRecords, record)
        if sourcePart then
            local partRecords = partSnowRecords[sourcePart]
            if not partRecords then partRecords = {}; partSnowRecords[sourcePart] = partRecords end
            table.insert(partRecords, record)
        end
        if isTerrain then terrainCoverCount += 1 end
        if uniqueKey then appliedServerSnowTiles[uniqueKey] = true end
        return true
    end

    local function getWorldBounds(parts)
        -- Exact world-axis extents for each oriented bounding box, without transforming all eight
        -- corners. This keeps the 150k-part scan from doing over a million CFrame transforms.
        local minX, maxX, minY, maxY, minZ, maxZ
        for _, part in ipairs(parts) do
            if part and part.Parent and part:IsA("BasePart") then
                local cf, position = part.CFrame, part.Position
                local half = part.Size * 0.5
                local right, up, look = cf.RightVector, cf.UpVector, cf.LookVector
                local ex = math.abs(right.X) * half.X + math.abs(up.X) * half.Y + math.abs(look.X) * half.Z
                local ey = math.abs(right.Y) * half.X + math.abs(up.Y) * half.Y + math.abs(look.Y) * half.Z
                local ez = math.abs(right.Z) * half.X + math.abs(up.Z) * half.Y + math.abs(look.Z) * half.Z
                minX = minX and math.min(minX, position.X - ex) or position.X - ex
                maxX = maxX and math.max(maxX, position.X + ex) or position.X + ex
                minY = minY and math.min(minY, position.Y - ey) or position.Y - ey
                maxY = maxY and math.max(maxY, position.Y + ey) or position.Y + ey
                minZ = minZ and math.min(minZ, position.Z - ez) or position.Z - ez
                maxZ = maxZ and math.max(maxZ, position.Z + ez) or position.Z + ez
            end
        end
        if not minX then
            local root = player.Character and player.Character:FindFirstChild("HumanoidRootPart")
            local pos = root and root.Position or Vector3.zero
            return {pos.X - 200, pos.X + 200, pos.Y - 100, pos.Y + 250, pos.Z - 200, pos.Z + 200}
        end
        return {minX, maxX, minY, maxY, minZ, maxZ}
    end

    local function createMapSnapshotRecord(part)
        local record = {
            sourceName = part.Name,
            position = part.Position,
            size = part.Size,
            cframe = part.CFrame,
            color = part.Color,
            material = part.Material,
            transparency = part.Transparency,
            shape = (part:IsA("Part") and part.Shape) or Enum.PartType.Block,
        }
        return record
    end

    local function getSnapshotBounds(records)
        local minX, maxX, minY, maxY, minZ, maxZ
        for _, record in ipairs(records) do
            local half = record.size * 0.5
            for sx = -1, 1, 2 do
                for sy = -1, 1, 2 do
                    for sz = -1, 1, 2 do
                        local point = record.cframe:PointToWorldSpace(Vector3.new(half.X * sx, half.Y * sy, half.Z * sz))
                        minX = minX and math.min(minX, point.X) or point.X
                        maxX = maxX and math.max(maxX, point.X) or point.X
                        minY = minY and math.min(minY, point.Y) or point.Y
                        maxY = maxY and math.max(maxY, point.Y) or point.Y
                        minZ = minZ and math.min(minZ, point.Z) or point.Z
                        maxZ = maxZ and math.max(maxZ, point.Z) or point.Z
                    end
                end
            end
        end
        if not minX then return nil end
        return minX, maxX, minY, maxY, minZ, maxZ
    end

    local function renderMapSnapshotPreview()
        if not mapPreviewWorld or not mapPreviewWorld.Parent or not mapPreviewCamera or not mapPreviewCamera.Parent then return end
        mapPreviewWorld:ClearAllChildren()
        if #mapSnapshot == 0 then
            if mapPreviewStatus and mapPreviewStatus.Parent then mapPreviewStatus.Text = "No se detectaron piezas cargadas." end
            return
        end

        local minX, maxX, minY, maxY, minZ, maxZ = getSnapshotBounds(mapSnapshot)
        if not minX then return end
        local spanX, spanY, spanZ = math.max(maxX - minX, 1), math.max(maxY - minY, 1), math.max(maxZ - minZ, 1)
        local centerX, centerY, centerZ = (minX + maxX) / 2, minY, (minZ + maxZ) / 2
        local horizontalScale = 58 / math.max(spanX, spanZ)
        local verticalScale = math.min(horizontalScale, 22 / spanY)
        -- The viewport is deliberately a simplified map overview, not a full mesh export.
        local maxPreviewParts = 520
        local stride = math.max(1, math.ceil(#mapSnapshot / maxPreviewParts))
        local shown = 0
        for index = 1, #mapSnapshot, stride do
            local record = mapSnapshot[index]
            local position = record.position
            local originalSize = record.size
            local scaledSize = Vector3.new(
                math.clamp(originalSize.X * horizontalScale, 0.12, 8),
                math.clamp(originalSize.Y * verticalScale, 0.10, 6),
                math.clamp(originalSize.Z * horizontalScale, 0.12, 8)
            )
            local previewPart = Instance.new("Part")
            previewPart.Name = "MapPreviewPart"
            previewPart.Anchored = true
            previewPart.CanCollide = false
            previewPart.CanTouch = false
            previewPart.CanQuery = false
            previewPart.CastShadow = false
            previewPart.Size = scaledSize
            previewPart.Color = record.color
            previewPart.Material = Enum.Material.SmoothPlastic
            previewPart.Transparency = math.clamp(record.transparency, 0, 0.35)
            if previewPart:IsA("Part") then
                pcall(function() previewPart.Shape = record.shape end)
            end
            local viewportPosition = Vector3.new(
                (position.X - centerX) * horizontalScale,
                (position.Y - centerY) * verticalScale,
                (position.Z - centerZ) * horizontalScale
            )
            previewPart.CFrame = CFrame.new(viewportPosition) * record.cframe.Rotation
            previewPart.Parent = mapPreviewWorld
            shown += 1
        end
        mapPreviewCamera.FieldOfView = 48
        mapPreviewCamera.CFrame = CFrame.lookAt(Vector3.new(52, 36, 52), Vector3.new(0, 8, 0))
        if mapPreviewStatus and mapPreviewStatus.Parent then
            mapPreviewStatus.Text = string.format("Vista 3D: %d muestras | %d piezas + %d muestras Terrain", shown, mapSnapshotPartCount, mapSnapshotTerrainCount)
        end
    end

    local setAccumulation
    local loadServerSnowPlan
    local scanLoadedMapForPreview
    local startSnowScan

    local function scanServerMapForPreview(andApplySnow)
        if andApplySnow or state.accumulation then snowAfterMapScan = true end
        serverSnowPlanFetched = false
        if mapScanRunning then
            if mapPreviewStatus and mapPreviewStatus.Parent then mapPreviewStatus.Text = "Escaneo del servidor ya en curso..." end
            return
        end
        mapScanRunning = true
        mapSnapshotReady = false
        if mapPreviewStatus and mapPreviewStatus.Parent then mapPreviewStatus.Text = "Solicitando inventario completo del servidor..." end
        task.spawn(function()
            local success = false
            local startResult, startError = invokeMapBridge("startScan", math.min(MAP_SCAN_HARD_LIMIT, state.mapScanLimit or MAP_SCAN_HARD_LIMIT), true)
            if startResult and startResult.ok then
                local status = startResult
                while status and not status.ready and not status.error do
                    if mapPreviewStatus and mapPreviewStatus.Parent then
                        mapPreviewStatus.Text = string.format("Servidor: %s Â· %d/%d piezas Â· %d superficies de nieve",
                            tostring(status.phase or "escaneando"), tonumber(status.mapCount) or 0,
                            math.min(MAP_SCAN_HARD_LIMIT, state.mapScanLimit or MAP_SCAN_HARD_LIMIT), tonumber(status.planCount) or 0)
                    end
                    task.wait(0.45)
                    status = invokeMapBridge("status")
                end
                if status and status.ready then
                    local previewResult = invokeMapBridge("getPreview")
                    if previewResult and previewResult.ok and type(previewResult.preview) == "table" then
                        serverMapStatus = status
                        serverSnowPlanCount = tonumber(status.planCount) or 0
                        mapSnapshot = previewResult.preview
                        mapSnapshotPartCount = tonumber(previewResult.mapCount) or 0
                        mapSnapshotTerrainCount = tonumber(previewResult.terrainPreviewCount) or 0
                        mapSnapshotReady = true
                        renderMapSnapshotPreview()
                        updateSnowStatus()
                        if mapPreviewStatus and mapPreviewStatus.Parent then
                            mapPreviewStatus.Text = string.format("Servidor completo: %d/%d piezas | plan nieve: %d | Terrain: %d muestras",
                                mapSnapshotPartCount, math.min(MAP_SCAN_HARD_LIMIT, state.mapScanLimit or MAP_SCAN_HARD_LIMIT),
                                serverSnowPlanCount, mapSnapshotTerrainCount)
                        end
                        success = true
                    else
                        startError = previewResult and previewResult.error or "No se pudo obtener la vista previa."
                    end
                else
                    startError = status and status.error or "El escaneo no terminÃ³ correctamente."
                end
            else
                startError = startError or (startResult and startResult.error) or "No hay respuesta del puente servidor."
            end
            mapScanRunning = false
            if not success then
                serverBridgeFailed = true
                if mapPreviewStatus and mapPreviewStatus.Parent then
                    mapPreviewStatus.Text = "Servidor no disponible; usando solo el mapa cargado en el cliente."
                end
                warn("[SYSTEM CFG] Puente de mapa no disponible: " .. tostring(startError))
                scanLoadedMapForPreview(false)
                if andApplySnow then snowAfterMapScan = true end
            elseif snowAfterMapScan then
                snowAfterMapScan = false
                if state.accumulation and setAccumulation then setAccumulation(false) end
                if setAccumulation then setAccumulation(true) end
            end
        end)
    end

    scanLoadedMapForPreview = function(andApplySnow)
        if mapBridge and mapBridge:IsA("RemoteFunction") and not serverBridgeFailed then
            scanServerMapForPreview(andApplySnow)
            return
        end
        if andApplySnow then snowAfterMapScan = true end
        if mapScanRunning then
            if mapPreviewStatus and mapPreviewStatus.Parent then mapPreviewStatus.Text = "Escaneo ya en curso..." end
            return
        end
        mapScanRunning = true
        mapSnapshotReady = false
        if mapPreviewStatus and mapPreviewStatus.Parent then mapPreviewStatus.Text = "Escaneando solo piezas cargadas en el cliente..." end
        task.spawn(function()
            local ok, err = pcall(function()
                local descendants = workspace:GetDescendants()
                local totalDescendants = #descendants
                local visited = 0
                for index, object in ipairs(descendants) do
                    visited = index
                    if object:IsA("BasePart") then indexMapPart(object) end
                    if index % 600 == 0 then
                        if mapPreviewStatus and mapPreviewStatus.Parent then
                            mapPreviewStatus.Text = string.format("Escaneando cliente: %d piezas / mÃ¡x. %d (%d%% del Ã¡rbol local)", cachedMapPartCount, state.mapScanLimit, math.floor(index / math.max(totalDescendants, 1) * 100))
                        end
                        task.wait()
                    end
                    if cachedMapPartCount >= math.min(MAP_SCAN_HARD_LIMIT, state.mapScanLimit or MAP_SCAN_HARD_LIMIT) then
                        -- Count/preview is intentionally capped; the live cache also refreshes as parts stream in.
                        break
                    end
                end
                local records = {}
                local sourceParts = {}
                for part in pairs(mapPartCache) do
                    if part and part.Parent and canIndexMapPart(part) then
                        table.insert(sourceParts, part)
                    end
                end
                -- Store only a representative sample for the ViewportFrame to avoid retaining 150k
                -- duplicate CFrames/colors in memory on phones. The cache itself still tracks up to 150k parts.
                local previewStride = math.max(1, math.ceil(#sourceParts / 2400))
                for index = 1, #sourceParts, previewStride do
                    table.insert(records, createMapSnapshotRecord(sourceParts[index]))
                end

                -- Add visible Terrain samples to the preview. Cast against the whole loaded scene;
                -- Terrain hidden under another part is not treated as the top surface.
                local bounds = getWorldBounds(sourceParts)
                local minX, maxX, minY, maxY, minZ, maxZ = table.unpack(bounds)
                local spanX, spanZ = math.max(1, maxX - minX), math.max(1, maxZ - minZ)
                local terrainStep = math.max(8, math.sqrt((spanX * spanZ) / 500))
                local terrainParams = snowRaycastParams
                local terrainOriginY = maxY + 400
                local terrainCastLength = math.clamp((maxY - minY) + 1000, 1500, 14900)
                local terrainSamples = 0
                for x = minX + terrainStep * 0.5, maxX, terrainStep do
                    for z = minZ + terrainStep * 0.5, maxZ, terrainStep do
                        local hit = workspace:Raycast(Vector3.new(x, terrainOriginY, z), Vector3.new(0, -terrainCastLength, 0), terrainParams)
                        if hit and hit.Instance == workspace.Terrain and hit.Material ~= Enum.Material.Water and hit.Normal.Y >= 0.86 then
                            local normal = hit.Normal.Unit
                            local tangent = Vector3.new(1, 0, 0) - normal * normal.X
                            if tangent.Magnitude < 0.05 then tangent = Vector3.new(0, 0, 1) - normal * normal.Z end
                            tangent = tangent.Unit
                            local bitangent = tangent:Cross(normal).Unit
                            local surfaceCF = CFrame.fromMatrix(hit.Position, tangent, normal, bitangent)
                            local okColor, terrainColor = pcall(function() return workspace.Terrain:GetMaterialColor(hit.Material) end)
                            table.insert(records, {
                                sourceName = "TerrainPreview",
                                position = hit.Position,
                                size = Vector3.new(terrainStep, 0.18, terrainStep),
                                cframe = surfaceCF,
                                color = okColor and terrainColor or Color3.fromRGB(105, 125, 100),
                                material = hit.Material,
                                transparency = 0,
                                shape = Enum.PartType.Block,
                            })
                            terrainSamples += 1
                        end
                        if terrainSamples > 0 and terrainSamples % 120 == 0 then task.wait() end
                    end
                end

                table.sort(records, function(a, b)
                    if a.position.X == b.position.X then return a.position.Z < b.position.Z end
                    return a.position.X < b.position.X
                end)
                mapSnapshot = records
                mapSnapshotPartCount = #sourceParts
                mapSnapshotTerrainCount = terrainSamples
                mapSnapshotReady = true
                renderMapSnapshotPreview()
                updateSnowStatus()
                if mapPreviewStatus and mapPreviewStatus.Parent then
                    local reachedCap = mapSnapshotPartCount >= math.min(MAP_SCAN_HARD_LIMIT, state.mapScanLimit or MAP_SCAN_HARD_LIMIT)
                    mapPreviewStatus.Text = string.format("%d piezas detectadas (lÃ­mite %d)%s | %d muestras Terrain", mapSnapshotPartCount, state.mapScanLimit, reachedCap and " Â· lÃ­mite alcanzado" or " Â· solo cliente cargado", mapSnapshotTerrainCount)
                end
            end)
            mapScanRunning = false
            if not ok then
                warn("[SYSTEM CFG] Error escaneando el mapa: " .. tostring(err))
                if mapPreviewStatus and mapPreviewStatus.Parent then mapPreviewStatus.Text = "Error de escaneo. Vuelve a intentarlo." end
            end
            if snowAfterMapScan then
                snowAfterMapScan = false
                if ok then setAccumulation(true) end
            end
        end)
    end

    local streamRequestRunning = false
    local function requestMoreMapChunks()
        if streamRequestRunning then
            if mapPreviewStatus and mapPreviewStatus.Parent then mapPreviewStatus.Text = "La solicitud de streaming ya estÃ¡ en curso..." end
            return
        end
        if not workspace.StreamingEnabled then
            if mapPreviewStatus and mapPreviewStatus.Parent then mapPreviewStatus.Text = "Instance Streaming estÃ¡ desactivado; reescanea el Workspace cargado." end
            scanLoadedMapForPreview(state.accumulation)
            return
        end
        streamRequestRunning = true
        task.spawn(function()
            local ok, err = pcall(function()
                -- Two bounded expanding passes ask Roblox to stream around a 3x3 grid based on
                -- currently known bounds. Requests are temporary and Roblox may refuse them under memory pressure.
                for pass = 1, 2 do
                    local parts = {}
                    for part in pairs(mapPartCache) do
                        if part and part.Parent and canIndexMapPart(part) then table.insert(parts, part) end
                    end
                    local bounds = getWorldBounds(parts)
                    local minX, maxX, minY, maxY, minZ, maxZ = table.unpack(bounds)
                    local span = math.max(maxX - minX, maxZ - minZ)
                    local padding = math.clamp(span * 0.22, 256, 768)
                    local xs = {minX - padding, (minX + maxX) * 0.5, maxX + padding}
                    local zs = {minZ - padding, (minZ + maxZ) * 0.5, maxZ + padding}
                    local y = (minY + maxY) * 0.5
                    local asked = 0
                    for _, x in ipairs(xs) do
                        for _, z in ipairs(zs) do
                            local position = Vector3.new(x, y, z)
                            pcall(function() player:RequestStreamAroundAsync(position, 0.8) end)
                            asked += 1
                            if mapPreviewStatus and mapPreviewStatus.Parent then
                                mapPreviewStatus.Text = string.format("Solicitando zonas %d/18 Â· fase %d/2 Â· %d piezas locales", (pass - 1) * 9 + asked, pass, cachedMapPartCount)
                            end
                            task.wait(0.05)
                        end
                    end
                    -- Refresh current client inventory between the two expanding passes.
                    local loaded = workspace:GetDescendants()
                    for index, object in ipairs(loaded) do
                        if object:IsA("BasePart") then indexMapPart(object) end
                        if index % 900 == 0 then task.wait() end
                        if cachedMapPartCount >= math.min(MAP_SCAN_HARD_LIMIT, state.mapScanLimit or MAP_SCAN_HARD_LIMIT) then break end
                    end
                    task.wait(0.15)
                end
                scanLoadedMapForPreview(state.accumulation)
            end)
            streamRequestRunning = false
            if not ok then
                warn("[SYSTEM CFG] Error solicitando mÃ¡s zonas: " .. tostring(err))
                if mapPreviewStatus and mapPreviewStatus.Parent then mapPreviewStatus.Text = "No se pudieron solicitar mÃ¡s zonas; reintenta el escaneo normal." end
            end
        end)
    end

    local function getTerrainParams()
        refreshSnowRaycastExclusions()
        return snowRaycastParams
    end

    local function addTerrainSnow(bounds)
        if not state.accumulation then return end
        local terrainCap = math.max(1, math.floor(state.surfaceBudget * 0.62))
        if terrainCoverCount >= terrainCap then return end
        local minX, maxX, minY, maxY, minZ, maxZ = table.unpack(bounds)
        local spanX, spanZ = math.max(1, maxX - minX), math.max(1, maxZ - minZ)
        local area = spanX * spanZ
        -- Fix the terrain grid origin/step for the whole session. When streaming adds map chunks,
        -- new cells align with the old ones and do not produce overlapping snow sheets.
        if not terrainGridStep then
            local desiredBaseCells = math.max(1, math.floor(terrainCap * 0.35))
            terrainGridStep = math.max(5, math.sqrt(area / desiredBaseCells))
            terrainGridOriginX = math.floor(minX / terrainGridStep) * terrainGridStep
            terrainGridOriginZ = math.floor(minZ / terrainGridStep) * terrainGridStep
        end
        local step = terrainGridStep
        local yOrigin = maxY + 400
        local castLength = math.clamp((maxY - minY) + 1000, 1500, 14900)
        local params = getTerrainParams()
        local raySamples = 0

        local function castTerrain(x, z)
            local result = workspace:Raycast(Vector3.new(x, yOrigin, z), Vector3.new(0, -castLength, 0), params)
            raySamples += 1
            if result and result.Instance == workspace.Terrain and result.Material ~= Enum.Material.Water then
                return result
            end
            return nil
        end

        local function terrainTile(x, z, size, level)
            if not state.accumulation or terrainCoverCount >= terrainCap then return end
            local key = string.format("%.2f:%.2f:%.2f", x, z, size)
            if terrainCellKeys[key] then return end
            local h = size * 0.49
            local sampleOffsets = {
                {0, 0}, {-h, -h}, {-h, h}, {h, -h}, {h, h},
                {0, -h}, {0, h}, {-h, 0}, {h, 0},
            }
            local hits = {}
            local count = 0
            local minHeight, maxHeight = math.huge, -math.huge
            local allGood = true
            for _, offset in ipairs(sampleOffsets) do
                local result = castTerrain(x + offset[1], z + offset[2])
                if result then
                    count += 1
                    table.insert(hits, result)
                    minHeight = math.min(minHeight, result.Position.Y)
                    maxHeight = math.max(maxHeight, result.Position.Y)
                    if result.Normal.Y < 0.86 then allGood = false end
                else
                    allGood = false
                end
            end
            local heightTolerance = math.max(0.18, size * 0.015)
            if maxHeight - minHeight > heightTolerance then allGood = false end
            if allGood and #hits == #sampleOffsets and terrainCoverCount < terrainCap then
                local centerResult = hits[1]
                local normal = centerResult.Normal.Unit
                local tangent = Vector3.new(1, 0, 0) - normal * normal.X
                if tangent.Magnitude < 0.05 then tangent = Vector3.new(0, 0, 1) - normal * normal.Z end
                tangent = tangent.Unit
                local bitangent = tangent:Cross(normal).Unit
                local frame = CFrame.fromMatrix(centerResult.Position, tangent, normal, bitangent)
                if makeSnowCover(frame, size, size, true) then terrainCellKeys[key] = true end
            elseif size > 4.5 and level < 6 and terrainCoverCount < terrainCap and count > 0 then
                -- Split incomplete cells instead of allowing a blanket slab to bridge edges, ledges or gaps.
                local child = size / 2
                terrainTile(x - child / 2, z - child / 2, child, level + 1)
                terrainTile(x - child / 2, z + child / 2, child, level + 1)
                terrainTile(x + child / 2, z - child / 2, child, level + 1)
                terrainTile(x + child / 2, z + child / 2, child, level + 1)
                if count == #sampleOffsets and not allGood and terrainCoverCount < terrainCap then terrainCellKeys[key] = true end
            elseif count == #sampleOffsets then
                -- A permanently steep or broken leaf cell is known and should not be retested every cycle.
                terrainCellKeys[key] = true
            end
            if raySamples % 180 == 0 then task.wait() end
        end

        -- Interleave cells across the entire bounding area so the budget is not spent on one edge of the map.
        local gridCells = {}
        local minCellX = math.floor((minX - terrainGridOriginX) / step)
        local maxCellX = math.ceil((maxX - terrainGridOriginX) / step) - 1
        local minCellZ = math.floor((minZ - terrainGridOriginZ) / step)
        local maxCellZ = math.ceil((maxZ - terrainGridOriginZ) / step) - 1
        for ix = minCellX, maxCellX do
            local x = terrainGridOriginX + (ix + 0.5) * step
            for iz = minCellZ, maxCellZ do
                local z = terrainGridOriginZ + (iz + 0.5) * step
                local rank = math.abs(math.sin(x * 12.9898 + z * 78.233) * 43758.5453)
                rank = rank - math.floor(rank)
                table.insert(gridCells, {x = x, z = z, rank = rank})
            end
        end
        table.sort(gridCells, function(a, b)
            if a.rank == b.rank then
                if a.x == b.x then return a.z < b.z end
                return a.x < b.x
            end
            return a.rank < b.rank
        end)
        for index, cell in ipairs(gridCells) do
            if not state.accumulation or terrainCoverCount >= terrainCap then break end
            terrainTile(cell.x, cell.z, step, 0)
            if index % 12 == 0 then task.wait() end
        end
    end

    local function partSurfaceFrame(part, localX, localZ, tileX, tileZ)
        -- Do not assume the part's theoretical top is visible. Sample its center, corners and edge
        -- midpoints from the full scene; if any point is occluded by another part or leaves the edge,
        -- reject this tile and let the recursive subdivision cut it into smaller exposed pieces.
        local offsets = {
            {0, 0}, {-tileX * 0.46, -tileZ * 0.46}, {-tileX * 0.46, tileZ * 0.46},
            {tileX * 0.46, -tileZ * 0.46}, {tileX * 0.46, tileZ * 0.46},
            {-tileX * 0.46, 0}, {tileX * 0.46, 0}, {0, -tileZ * 0.46}, {0, tileZ * 0.46},
        }
        local centerResult, minY, maxY = nil, math.huge, -math.huge
        for _, offset in ipairs(offsets) do
            local localTop = Vector3.new(localX + offset[1], part.Size.Y * 0.5, localZ + offset[2])
            local worldTop = part.CFrame:PointToWorldSpace(localTop)
            local origin = worldTop + Vector3.new(0, 2.5, 0)
            local result = workspace:Raycast(origin, Vector3.new(0, -12, 0), snowRaycastParams)
            if not result or result.Instance ~= part or result.Normal.Y < 0.86 then
                return nil
            end
            minY = math.min(minY, result.Position.Y)
            maxY = math.max(maxY, result.Position.Y)
            if offset[1] == 0 and offset[2] == 0 then centerResult = result end
        end
        if not centerResult then return nil end
        local heightTolerance = math.max(0.08, math.min(tileX, tileZ) * 0.012)
        if maxY - minY > heightTolerance then return nil end
        local normal = centerResult.Normal.Unit
        local tangent = part.CFrame.RightVector - normal * part.CFrame.RightVector:Dot(normal)
        if tangent.Magnitude < 0.05 then tangent = Vector3.new(1, 0, 0) - normal * normal.X end
        if tangent.Magnitude < 0.05 then tangent = Vector3.new(0, 0, 1) - normal * normal.Z end
        tangent = tangent.Unit
        local bitangent = tangent:Cross(normal).Unit
        return CFrame.fromMatrix(centerResult.Position, tangent, normal, bitangent)
    end

    local function coverPartTop(part)
        if not canSnowCover(part) then return end
        local tileCache = partTileCache[part]
        if not tileCache then tileCache = {}; partTileCache[part] = tileCache end
        local maxTile = 24
        local sx, sz = part.Size.X, part.Size.Z
        local tileCountX = math.max(1, math.ceil(sx / maxTile))
        local tileCountZ = math.max(1, math.ceil(sz / maxTile))
        for ix = 1, tileCountX do
            for iz = 1, tileCountZ do
                if not state.accumulation or #surfaceRecords >= state.surfaceBudget then return end
                local startX = -sx / 2 + (ix - 1) * (sx / tileCountX)
                local startZ = -sz / 2 + (iz - 1) * (sz / tileCountZ)
                local tileX = sx / tileCountX
                local tileZ = sz / tileCountZ
                local localX = startX + tileX / 2
                local localZ = startZ + tileZ / 2
                local baseKey = string.format("%d:%d", ix, iz)
                if not tileCache[baseKey] then
                    local function coverSubTile(cx, cz, tw, td, level)
                        if not state.accumulation or #surfaceRecords >= state.surfaceBudget then return end
                        local key = string.format("%.2f:%.2f:%.2f:%.2f", cx, cz, tw, td)
                        if tileCache[key] then return end
                        local frame = partSurfaceFrame(part, cx, cz, tw, td)
                        if frame then
                            if makeSnowCover(frame, tw, td, false, part) then tileCache[key] = true end
                            return
                        end
                        if math.max(tw, td) > 4.5 and level < 5 then
                            local hw, hd = tw / 2, td / 2
                            coverSubTile(cx - hw / 2, cz - hd / 2, hw, hd, level + 1)
                            coverSubTile(cx - hw / 2, cz + hd / 2, hw, hd, level + 1)
                            coverSubTile(cx + hw / 2, cz - hd / 2, hw, hd, level + 1)
                            coverSubTile(cx + hw / 2, cz + hd / 2, hw, hd, level + 1)
                            if #surfaceRecords < state.surfaceBudget then tileCache[key] = true end
                        else
                            -- At leaf scale, ignore invalid/non-horizontal geometry rather than making a floating block.
                            tileCache[key] = true
                        end
                    end
                    coverSubTile(localX, localZ, tileX, tileZ, 0)
                    if tileCache[string.format("%.2f:%.2f:%.2f:%.2f", localX, localZ, tileX, tileZ)] then
                        tileCache[baseKey] = true
                    end
                end
            end
        end
    end

    local function applyServerPlansToPart(part)
        if not state.accumulation or not serverSnowPlanFetched or not part or not part.Parent then return end
        if not canSnowCover(part) then return end
        local key = makeLoadedPartKey(part)
        local plans = serverPlansByPartKey[key]
        if not plans then return end
        for _, plan in ipairs(plans) do
            if not appliedServerSnowTiles[plan.tileId] and #surfaceRecords < state.surfaceBudget then
                -- Re-check the surface locally; streamed geometry or moving props may have changed since server scan.
                local frame = partSurfaceFrame(part, plan.localX, plan.localZ, plan.size.X, plan.size.Z)
                if frame then makeSnowCover(frame, plan.size.X, plan.size.Z, false, part, plan.tileId) end
            end
        end
        updateSnowStatus()
    end

    local function applyServerTerrainPlansNearPlayer()
        if not state.accumulation or not serverSnowPlanFetched or #serverTerrainPlans == 0 then return end
        local character = player.Character
        local root = character and character:FindFirstChild("HumanoidRootPart")
        local center = root and root.Position or (workspace.CurrentCamera and workspace.CurrentCamera.CFrame.Position)
        if not center then return end
        local processed = 0
        -- Cheap distance checks cover the current streamed region; cells are revisited as the player moves.
        for _, plan in ipairs(serverTerrainPlans) do
            if processed >= 220 then break end
            if not appliedServerSnowTiles[plan.tileId] and (plan.cframe.Position - center).Magnitude <= 700 then
                processed += 1
                local origin = plan.cframe.Position + Vector3.new(0, 6, 0)
                local hit = workspace:Raycast(origin, Vector3.new(0, -14, 0), getTerrainParams())
                if hit and hit.Instance == workspace.Terrain and hit.Material ~= Enum.Material.Water
                    and hit.Normal.Y >= 0.86 and math.abs(hit.Position.Y - plan.cframe.Position.Y) <= 1.2 then
                    local normal = hit.Normal.Unit
                    local tangent = Vector3.new(1, 0, 0) - normal * normal.X
                    if tangent.Magnitude < 0.05 then tangent = Vector3.new(0, 0, 1) - normal * normal.Z end
                    tangent = tangent.Unit
                    local bitangent = tangent:Cross(normal).Unit
                    local frame = CFrame.fromMatrix(hit.Position, tangent, normal, bitangent)
                    makeSnowCover(frame, plan.size.X, plan.size.Z, true, nil, plan.tileId)
                end
            end
        end
        updateSnowStatus()
    end

    loadServerSnowPlan = function()
        if serverBridgeFailed or not mapBridge or not mapBridge:IsA("RemoteFunction") then
            startSnowScan()
            return
        end
        if serverSnowPlanFetched then
            for part in pairs(mapPartCache) do applyServerPlansToPart(part) end
            applyServerTerrainPlansNearPlayer()
            return
        end
        if serverSnowPlanLoading then return end
        serverSnowPlanLoading = true
        task.spawn(function()
            local status = invokeMapBridge("status")
            if not status or not status.ready then
                serverSnowPlanLoading = false
                snowAfterMapScan = true
                scanLoadedMapForPreview(true)
                return
            end
            serverSnowPlanCount = tonumber(status.planCount) or 0
            table.clear(serverPlansByPartKey)
            table.clear(serverTerrainPlans)
            table.clear(appliedServerSnowTiles)
            local cursor = 1
            while cursor <= serverSnowPlanCount and state.accumulation do
                local response = invokeMapBridge("getSnowBatch", cursor, 180)
                if not response or not response.ok or type(response.items) ~= "table" or #response.items == 0 then
                    if mapPreviewStatus and mapPreviewStatus.Parent then mapPreviewStatus.Text = "Error leyendo el plan de nieve; revisa el Script servidor." end
                    break
                end
                for _, plan in ipairs(response.items) do
                    if plan.isTerrain then
                        table.insert(serverTerrainPlans, plan)
                    elseif type(plan.partKey) == "string" then
                        local group = serverPlansByPartKey[plan.partKey]
                        if not group then group = {}; serverPlansByPartKey[plan.partKey] = group end
                        table.insert(group, plan)
                    end
                end
                cursor += #response.items
                if mapPreviewStatus and mapPreviewStatus.Parent then
                    mapPreviewStatus.Text = string.format("Preparando nieve de mapa completo: %d/%d superficies", math.min(cursor - 1, serverSnowPlanCount), serverSnowPlanCount)
                end
                task.wait(0.025)
            end
            serverSnowPlanLoading = false
            serverSnowPlanFetched = cursor > serverSnowPlanCount
            if serverSnowPlanFetched and state.accumulation then
                for part in pairs(mapPartCache) do applyServerPlansToPart(part) end
                applyServerTerrainPlansNearPlayer()
                if mapPreviewStatus and mapPreviewStatus.Parent then
                    mapPreviewStatus.Text = string.format("Plan listo: %d superficies muestreadas | aplicadas %d | presupuesto local %d",
                        serverSnowPlanCount, #surfaceRecords, state.surfaceBudget)
                end
            end
            updateSnowStatus()
        end)
    end

    local function clearSnowCovers()
        coverFolder:ClearAllChildren()
        table.clear(appliedServerSnowTiles)
        table.clear(surfaceRecords)
        partSnowRecords = setmetatable({}, {__mode = "k"})
        table.clear(terrainCellKeys)
        partTileCache = setmetatable({}, {__mode = "k"})
        terrainCoverCount = 0
        currentSnowDepth = 0.012
        frostProgress = 0
        updateSnowStatus()
    end

    startSnowScan = function()
        if not state.accumulation then return end
        if scanRunning then scanRequested = true; return end
        scanRunning = true
        task.spawn(function()
            repeat
                scanRequested = false
                if not state.accumulation then break end
                local mapParts, candidates = {}, {}
                for part in pairs(mapPartCache) do
                    if part and part.Parent and canIndexMapPart(part) then
                        table.insert(mapParts, part)
                        if canSnowCover(part) then table.insert(candidates, part) end
                    end
                end
                -- Visit surface candidates in a deterministic spatially distributed order instead of
                -- filling the first cluster until the layer budget is exhausted.
                local rankedCandidates = {}
                for _, candidate in ipairs(candidates) do
                    local pos = candidate.Position
                    local rank = math.abs(math.sin(pos.X * 12.9898 + pos.Y * 78.233 + pos.Z * 37.719) * 43758.5453) % 1
                    table.insert(rankedCandidates, {part = candidate, rank = rank})
                end
                table.sort(rankedCandidates, function(a, b) return a.rank < b.rank end)
                candidates = rankedCandidates
                local bounds = getWorldBounds(mapParts)
                -- Scan Terrain across the map's loaded world bounds, not only around the player's camera.
                addTerrainSnow(bounds)
                local processed = 0
                for _, candidate in ipairs(candidates) do
                    if not state.accumulation or #surfaceRecords >= state.surfaceBudget then break end
                    local part = candidate.part or candidate
                    if part.Parent then coverPartTop(part) end
                    processed += 1
                    if processed % 6 == 0 then
                        updateSnowStatus()
                        task.wait(0.025 / math.max(1, state.accumulationSpeed))
                    end
                end
                updateSnowStatus()
            until not scanRequested
            scanRunning = false
        end)
    end

    local function setSnowfall(on)
        state.snowfall = on
        snowEmitter.Enabled = on
        if on then weatherFolder.Parent = workspace end
    end

    setAccumulation = function(on)
        state.accumulation = on
        if on then
            currentSnowDepth = math.max(currentSnowDepth, 0.012)
            coverFolder.Parent = workspace
            if mapBridge and mapBridge:IsA("RemoteFunction") and not serverBridgeFailed then
                if not serverMapStatus or not serverMapStatus.ready then
                    snowAfterMapScan = true
                    scanLoadedMapForPreview(true)
                else
                    loadServerSnowPlan()
                end
            else
                startSnowScan()
            end
        else
            clearSnowCovers()
        end
    end

    -- Cache loaded static map geometry for this play session, including non-collidable queryable roofs/decorative surfaces.
    do
        local startupDescendants = workspace:GetDescendants()
        for index, object in ipairs(startupDescendants) do
            if object:IsA("BasePart") then indexMapPart(object) end
            if index % 1200 == 0 then task.wait() end
            if cachedMapPartCount >= MAP_SCAN_HARD_LIMIT then break end
        end
    end
    globalConnect(workspace.DescendantAdded, function(object)
        if not object:IsA("BasePart") then return end
        indexMapPart(object)
        if state.accumulation then
            if mapBridge and mapBridge:IsA("RemoteFunction") and not serverBridgeFailed then
                if serverSnowPlanFetched then applyServerPlansToPart(object) end
            else
                if scanDebouncePending then return end
                scanDebouncePending = true
                task.delay(0.45, function()
                    scanDebouncePending = false
                    if state.accumulation then startSnowScan() end
                end)
            end
        end
    end)
    local function removeSnowForPart(part)
        local recordsForPart = partSnowRecords[part]
        if not recordsForPart then return end
        local removeSet = {}
        for _, record in ipairs(recordsForPart) do
            removeSet[record] = true
            if record.planKey then appliedServerSnowTiles[record.planKey] = nil end
            if record.cover and record.cover.Parent then record.cover:Destroy() end
        end
        partSnowRecords[part] = nil
        local remaining = {}
        for _, record in ipairs(surfaceRecords) do
            if not removeSet[record] then table.insert(remaining, record) end
        end
        surfaceRecords = remaining
        partTileCache[part] = nil
        updateSnowStatus()
    end

    globalConnect(workspace.DescendantRemoving, function(object)
        if object:IsA("BasePart") then
            removeSnowForPart(object)
            unindexMapPart(object)
        end
    end)

    -- Avatar preview: independent ViewportFrame; source avatar is never anchored or changed.
    local function buildAvatarPreview()
        if avatarModel then avatarModel:Destroy(); avatarModel = nil end
        table.clear(avatarPartPairs)
        local character = player.Character
        if not character then return false end
        local oldArchivable = character.Archivable
        local clone
        local clonedOk, clonedResult = pcall(function()
            character.Archivable = true
            return character:Clone()
        end)
        pcall(function() character.Archivable = oldArchivable end)
        if not clonedOk or not clonedResult then return false end
        clone = clonedResult
        clone.Name = "AvatarPreviewModel"
        for _, desc in ipairs(clone:GetDescendants()) do
            if desc:IsA("Script") or desc:IsA("LocalScript") or desc:IsA("ModuleScript") or desc:IsA("Tool") then
                desc:Destroy()
            elseif desc:IsA("BasePart") then
                desc.Anchored = true
                desc.CanCollide = false
                desc.CanTouch = false
                desc.CanQuery = false
                desc.CastShadow = false
            elseif desc:IsA("BillboardGui") or desc:IsA("SurfaceGui") or desc:IsA("Highlight") then
                desc:Destroy()
            end
        end
        for _, desc in ipairs(clone:GetChildren()) do
            if desc:IsA("Humanoid") then
                desc.DisplayDistanceType = Enum.HumanoidDisplayDistanceType.None
                desc.HealthDisplayType = Enum.HumanoidHealthDisplayType.AlwaysOff
                desc.NameDisplayDistance = 0
            end
        end
        clone.Parent = avatarWorld
        avatarModel = clone
        local sourceRoot = character:FindFirstChild("HumanoidRootPart")
        if not sourceRoot then
            local sourceHumanoid = character:FindFirstChildOfClass("Humanoid")
            sourceRoot = sourceHumanoid and sourceHumanoid.RootPart
        end
        local okBounds, bboxCF, bboxSize = pcall(function() return clone:GetBoundingBox() end)
        if okBounds and typeof(bboxCF) == "CFrame" and sourceRoot then
            avatarCenterLocal = sourceRoot.CFrame:PointToObjectSpace(bboxCF.Position)
            local fitHeight = math.max(bboxSize.Y, bboxSize.X * 0.72, bboxSize.Z * 0.72)
            avatarDistance = math.clamp((fitHeight * 0.5) / math.tan(math.rad(avatarCamera.FieldOfView * 0.5)) * 1.34, 3.8, 28)
            state.avatarZoom = avatarDistance
        else
            avatarCenterLocal = Vector3.new(0, 2, 0)
            avatarDistance = 10
            state.avatarZoom = avatarDistance
        end
        -- Pair by relative hierarchy path instead of part name alone (many accessories share "Handle").
        local function isUnderTool(obj, root)
            local node = obj
            while node and node ~= root do
                if node:IsA("Tool") then return true end
                node = node.Parent
            end
            return false
        end
        local function relativeKey(root, obj)
            local segments = {}
            local node = obj
            while node and node ~= root do
                local sameNameIndex = 0
                if node.Parent then
                    for _, sibling in ipairs(node.Parent:GetChildren()) do
                        if sibling.Name == node.Name and sibling.ClassName == node.ClassName then
                            sameNameIndex += 1
                            if sibling == node then break end
                        end
                    end
                end
                table.insert(segments, 1, node.ClassName .. ":" .. node.Name .. "#" .. tostring(sameNameIndex))
                node = node.Parent
            end
            return table.concat(segments, "/")
        end
        local cloneParts = {}
        for _, part in ipairs(clone:GetDescendants()) do
            if part:IsA("BasePart") and not isUnderTool(part, clone) then
                cloneParts[relativeKey(clone, part)] = part
            end
        end
        if sourceRoot then
            for _, sourcePart in ipairs(character:GetDescendants()) do
                if sourcePart:IsA("BasePart") and not isUnderTool(sourcePart, character) then
                    local previewPart = cloneParts[relativeKey(character, sourcePart)]
                    if previewPart then
                        table.insert(avatarPartPairs, {source = sourcePart, preview = previewPart})
                    end
                end
            end
        end
        state.avatarYaw = 0
        avatarYawCurrent = 0
        setAvatarCamera()
        -- The render loop copies the first pose on the next frame, avoiding a forward-reference boot error.
        return #avatarPartPairs > 0
    end

    local function updateAvatarPose(dt)
        if not state.avatarOpen or not avatarModel or not avatarModel.Parent then return end
        local character = player.Character
        local sourceRoot = character and character:FindFirstChild("HumanoidRootPart")
        if not sourceRoot then return end
        local delta = math.atan2(math.sin(state.avatarYaw - avatarYawCurrent), math.cos(state.avatarYaw - avatarYawCurrent))
        avatarYawCurrent += delta * math.clamp((dt or 1) * 16, 0, 1)
        local rotation = CFrame.Angles(0, avatarYawCurrent, 0)
        for _, pair in ipairs(avatarPartPairs) do
            if pair.source.Parent and pair.preview.Parent then
                local relative = sourceRoot.CFrame:ToObjectSpace(pair.source.CFrame)
                pair.preview.CFrame = rotation * CFrame.new(-avatarCenterLocal) * relative
            end
        end
    end

    local function positionAvatar(corner)
        state.avatarCorner = corner
        local w, h = avatarWindow.Size.X.Offset, avatarWindow.Size.Y.Offset
        local camera = workspace.CurrentCamera
        local view = camera and camera.ViewportSize or Vector2.new(900, 600)
        local margin = 10
        local x = (corner == "TR" or corner == "BR") and (view.X - w - margin) or margin
        local y = (corner == "BL" or corner == "BR") and (view.Y - h - margin) or 45
        x = math.clamp(x, 0, math.max(0, view.X - w - margin))
        y = math.clamp(y, 0, math.max(0, view.Y - h - margin))
        avatarWindow.Position = UDim2.fromOffset(x, y)
    end

    local function setAvatarOpen(on)
        state.avatarOpen = on
        avatarWindow.Visible = on
        if on then
            if not buildAvatarPreview() then
                warn("[SYSTEM CFG] Avatar preview could not clone the current character yet.")
            end
            positionAvatar(state.avatarCorner)
        end
    end

    avatarClose.Activated:Connect(function() setAvatarOpen(false) end)
    local avatarDragInput, avatarDragStart, avatarYawStart
    avatarViewport.InputBegan:Connect(function(input)
        if input.UserInputType == Enum.UserInputType.MouseButton1 or input.UserInputType == Enum.UserInputType.Touch then
            avatarDragInput = input
            avatarDragStart = Vector2.new(input.Position.X, input.Position.Y)
            avatarYawStart = state.avatarYaw
        end
    end)

    -- Free camera saved state.
    local freeCameraState = {
        enabled = false, pos = Vector3.zero, yaw = 0, pitch = 0,
        savedType = nil, savedSubject = nil, savedFov = 70,
        savedMinZoom = 0.5, savedMaxZoom = 128,
        savedCameraMode = player.CameraMode,
        orbit = false,
    }

    local function restoreCamera()
        state.freeCamera = false
        state.cameraOrbit = false
        freeCameraState.orbit = false
        freeCameraState.enabled = false
        camPad.Visible = false
        local camera = workspace.CurrentCamera
        if not camera then return end
        local character = player.Character
        local humanoid = character and character:FindFirstChildOfClass("Humanoid")
        local savedType = freeCameraState.savedType
        camera.CameraType = (savedType and savedType ~= Enum.CameraType.Scriptable) and savedType or Enum.CameraType.Custom
        if humanoid then
            camera.CameraSubject = humanoid
        elseif freeCameraState.savedSubject and freeCameraState.savedSubject.Parent then
            camera.CameraSubject = freeCameraState.savedSubject
        end
        camera.FieldOfView = freeCameraState.savedFov or 70
        state.fov = camera.FieldOfView
        player.CameraMinZoomDistance = math.clamp(state.zoomMin or freeCameraState.savedMinZoom or 0.5, 0.5, MAX_CAMERA_ZOOM_DISTANCE)
        player.CameraMaxZoomDistance = math.clamp(state.zoomMax or freeCameraState.savedMaxZoom or MAX_CAMERA_ZOOM_DISTANCE, player.CameraMinZoomDistance, MAX_CAMERA_ZOOM_DISTANCE)
        player.CameraMode = state.cameraUnlocked and Enum.CameraMode.Classic or (freeCameraState.savedCameraMode or player.CameraMode)
    end

    local function setFreeCamera(on)
        local camera = workspace.CurrentCamera
        if not camera then return end
        if on then
            if freeCameraState.enabled then return end
            freeCameraState.savedType = camera.CameraType
            freeCameraState.savedSubject = camera.CameraSubject
            freeCameraState.savedFov = camera.FieldOfView
            state.fov = camera.FieldOfView
            freeCameraState.savedMinZoom = player.CameraMinZoomDistance
            freeCameraState.savedMaxZoom = player.CameraMaxZoomDistance
            freeCameraState.savedCameraMode = player.CameraMode
            freeCameraState.pos = camera.CFrame.Position
            local pitch, yaw = camera.CFrame:ToOrientation()
            freeCameraState.pitch = pitch
            freeCameraState.yaw = yaw
            freeCameraState.enabled = true
            state.freeCamera = true
            camera.CameraType = Enum.CameraType.Scriptable
            camPad.Visible = UserInputService.TouchEnabled
        else
            if not freeCameraState.enabled then return end
            restoreCamera()
        end
    end

    -- Draggable panel/launcher, FPS and rifle. One shared input router, avoiding tab-level leaks.
    local dragState = nil
    local activeSlider = nil
    local function clampPosition(guiObject, x, y)
        local camera = workspace.CurrentCamera
        local view = camera and camera.ViewportSize or Vector2.new(800, 600)
        local size = guiObject.AbsoluteSize
        return math.clamp(x, 0, math.max(0, view.X - size.X)), math.clamp(y, 0, math.max(0, view.Y - size.Y))
    end

    local function makeDraggable(target, handle, canDrag, onMoved)
        handle.InputBegan:Connect(function(input)
            if input.UserInputType ~= Enum.UserInputType.MouseButton1 and input.UserInputType ~= Enum.UserInputType.Touch then return end
            if canDrag and not canDrag() then return end
            dragState = {
                target = target, input = input,
                pointerStart = Vector2.new(input.Position.X, input.Position.Y),
                objectStart = target.AbsolutePosition,
                moved = false, onMoved = onMoved,
            }
        end)
    end

    makeDraggable(panel, header)
    makeDraggable(launcher, launcher, function() return true end, function() end)
    makeDraggable(fpsWidget, fpsWidget, function() return not state.fpsLocked end, function(pos)
        local camera = workspace.CurrentCamera
        if camera then
            local center = pos + fpsWidget.AbsoluteSize / 2
            local vp = camera.ViewportSize
            state.fpsCorner = center.X < vp.X / 2 and (center.Y < vp.Y / 2 and "TL" or "BL") or (center.Y < vp.Y / 2 and "TR" or "BR")
        end
    end)
    makeDraggable(rifleAim, rifleAim, function() return state.rifle end, function(pos)
        local camera = workspace.CurrentCamera
        if camera then
            state.rifleX = pos.X + rifleAim.AbsoluteSize.X / 2 - camera.ViewportSize.X / 2
            state.rifleY = pos.Y + rifleAim.AbsoluteSize.Y / 2 - camera.ViewportSize.Y / 2
        end
    end)
    makeDraggable(avatarWindow, avatarHeader, function() return state.avatarOpen end, function() end)

    local function restoreFpsCorner(corner)
        state.fpsCorner = corner
        local camera = workspace.CurrentCamera
        local view = camera and camera.ViewportSize or Vector2.new(900, 600)
        local w, h, margin = fpsWidget.AbsoluteSize.X, fpsWidget.AbsoluteSize.Y, 12
        local x = (corner == "TR" or corner == "BR") and (view.X - w - margin) or margin
        local y = (corner == "BL" or corner == "BR") and (view.Y - h - margin) or margin
        fpsWidget.Position = UDim2.fromOffset(math.max(0, x), math.max(0, y))
    end

    -- Forward declaration: UI actions and keyboard handlers share this callback.
    local setPhotoMode

    -- An invisible restore target remains touchable in photo mode; P also toggles it on keyboard.
    local photoHotspot = addButton(gui, "â© CFG", UDim2.fromOffset(66, 34))
    photoHotspot.Name = "PhotoRestoreHotspot"
    photoHotspot.Position = UDim2.fromOffset(10, 98)
    photoHotspot.BackgroundColor3 = C.BG
    photoHotspot.TextColor3 = C.CYAN
    photoHotspot.TextSize = 11
    photoHotspot.Visible = false
    photoHotspot.ZIndex = 100
    addStroke(photoHotspot, C.MAGENTA, 1.5)
    SYSTEM_CFG_STAGE = "building tabs and page controls"
    -- Tab/panel builder.
    local tabConnections = {}
    local function clearPage()
        activeSlider = nil
        for _, connection in ipairs(tabConnections) do pcall(function() connection:Disconnect() end) end
        table.clear(tabConnections)
        for _, child in ipairs(content:GetChildren()) do
            if child ~= contentPadding and child ~= contentLayout then child:Destroy() end
        end
    end
    local function pageConnect(signal, callback)
        local connection = signal:Connect(callback)
        table.insert(tabConnections, connection)
        return connection
    end

    -- Small semantic 3D thumbnails are built from a label/category, never from random filler.
    -- They are deliberately low-poly and static to keep mobile rendering costs bounded.
    local function controlPreviewCategory(labelText)
        local label = string.lower(tostring(labelText or ""))
        if string.find(label, "crosshair", 1, true) or string.find(label, "reticle", 1, true)
            or string.find(label, "mira", 1, true) or string.find(label, "aim", 1, true)
            or string.find(label, "rifle", 1, true) then return "CROSSHAIR" end
        if string.find(label, "audio", 1, true) or string.find(label, "volumen", 1, true)
            or string.find(label, "sound", 1, true) or string.find(label, "sonido", 1, true) then return "SPEAKER" end
        if label == "cam" or string.find(label, "camera", 1, true) or string.find(label, "cÃ¡mara", 1, true)
            or string.find(label, "camara", 1, true) or string.find(label, "fov", 1, true)
            or string.find(label, "zoom", 1, true) then return "CAMERA" end
        if label == "visual" then return "SPECTRUM" end
        if string.find(label, "fps", 1, true) or string.find(label, "monitor", 1, true)
            or string.find(label, "graphics", 1, true) or string.find(label, "grÃ¡fic", 1, true) then return "MONITOR" end
        if string.find(label, "snow", 1, true) or string.find(label, "nieve", 1, true)
            or string.find(label, "flake", 1, true) or string.find(label, "frost", 1, true)
            or string.find(label, "escarcha", 1, true) then return "SNOW" end
        if string.find(label, "map", 1, true) or string.find(label, "mapa", 1, true)
            or string.find(label, "terrain", 1, true) or string.find(label, "terreno", 1, true)
            or string.find(label, "world", 1, true) or string.find(label, "mundo", 1, true) then return "TERRAIN" end
        if string.find(label, "moon", 1, true) or string.find(label, "luna", 1, true)
            or string.find(label, "night", 1, true) or string.find(label, "noche", 1, true) then return "MOON" end
        if string.find(label, "sun", 1, true) or string.find(label, "sol", 1, true)
            or string.find(label, "ray", 1, true) or string.find(label, "brillo", 1, true)
            or string.find(label, "brightness", 1, true) then return "SUN" end
        if string.find(label, "fog", 1, true) or string.find(label, "niebla", 1, true)
            or string.find(label, "atmosphere", 1, true) or string.find(label, "atmÃ³sfera", 1, true) then return "FOG" end
        if string.find(label, "color", 1, true) or string.find(label, "rgb", 1, true)
            or string.find(label, "rainbow", 1, true) or string.find(label, "arcoÃ­ris", 1, true)
            or string.find(label, "bloom", 1, true) or string.find(label, "saturation", 1, true)
            or string.find(label, "contrast", 1, true) or string.find(label, "tint", 1, true) then return "SPECTRUM" end
        if string.find(label, "avatar", 1, true) or string.find(label, "player", 1, true)
            or string.find(label, "jugador", 1, true) or string.find(label, "character", 1, true)
            or string.find(label, "personaje", 1, true) or string.find(label, "esp", 1, true) then return "AVATAR" end
        if string.find(label, "click", 1, true) or string.find(label, "clic", 1, true)
            or string.find(label, "activate", 1, true) then return "MOUSE" end
        if string.find(label, "restore", 1, true) or string.find(label, "restaurar", 1, true)
            or string.find(label, "reset", 1, true) or string.find(label, "centrar", 1, true) then return "RESET" end
        if string.find(label, "open", 1, true) or string.find(label, "abrir", 1, true)
            or string.find(label, "close", 1, true) or string.find(label, "cerrar", 1, true) then return "DOOR" end
        if string.find(label, "noclip", 1, true) or string.find(label, "collision", 1, true)
            or string.find(label, "colisiÃ³n", 1, true) or string.find(label, "shield", 1, true) then return "SHIELD" end
        if string.find(label, "photo", 1, true) or string.find(label, "foto", 1, true)
            or string.find(label, "capture", 1, true) or string.find(label, "stream", 1, true) then return "CAMERA" end
        if string.find(label, "size", 1, true) or string.find(label, "tamaÃ±o", 1, true)
            or string.find(label, "thickness", 1, true) or string.find(label, "grosor", 1, true)
            or string.find(label, "length", 1, true) or string.find(label, "longitud", 1, true)
            or string.find(label, "radio", 1, true) or string.find(label, "speed", 1, true)
            or string.find(label, "velocidad", 1, true) then return "GAUGE" end
        return "GEAR"
    end

    local function addMiniControlPreview(parent, labelText, position, size)
        local category = controlPreviewCategory(labelText)
        local viewportSize = size or 24
        local viewport = new("ViewportFrame", {
            Name = "Semantic3DPreview_" .. category,
            Active = false,
            Selectable = false,
            Position = position or UDim2.fromOffset(4, 4),
            Size = UDim2.fromOffset(viewportSize, viewportSize),
            BackgroundColor3 = Color3.fromRGB(4, 8, 17),
            BackgroundTransparency = 0.12,
            BorderSizePixel = 0,
            LightColor = Color3.fromRGB(236, 246, 255),
            LightDirection = Vector3.new(-1, -1, -1),
            Ambient = Color3.fromRGB(130, 148, 170),
            ZIndex = 25,
        }, parent)
        addCorner(viewport, 4)
        local world = new("WorldModel", {Name = category .. "Model"}, viewport)
        local camera = new("Camera", {Name = "SemanticPreviewCamera", FieldOfView = 42}, viewport)
        viewport.CurrentCamera = camera
        camera.CFrame = CFrame.lookAt(Vector3.new(0, 0, 3.6), Vector3.zero)

        local function part(name, size3, position3, color, rotation3, shape, material)
            local props = {
                Name = name,
                Size = size3,
                CFrame = CFrame.new(position3) * CFrame.Angles(rotation3 and rotation3.X or 0, rotation3 and rotation3.Y or 0, rotation3 and rotation3.Z or 0),
                Anchored = true,
                CanCollide = false,
                CanTouch = false,
                CanQuery = false,
                CastShadow = false,
                Color = color or C.CYAN,
                Material = material or Enum.Material.Neon,
            }
            if shape then props.Shape = shape end
            return new("Part", props, world)
        end
        local dark = Color3.fromRGB(28, 38, 57)
        local cyan, magenta, yellow = C.CYAN, C.MAGENTA, C.YELLOW
        if category == "CROSSHAIR" then
            part("ReticleVertical", Vector3.new(0.08, 0.85, 0.1), Vector3.new(0, 0, 0), cyan)
            part("ReticleHorizontal", Vector3.new(0.85, 0.08, 0.1), Vector3.new(0, 0, 0), cyan)
            part("ReticleCore", Vector3.new(0.16, 0.16, 0.12), Vector3.new(0, 0, 0.1), yellow, nil, Enum.PartType.Ball)
        elseif category == "SPEAKER" then
            part("SpeakerCabinet", Vector3.new(0.9, 1.05, 0.55), Vector3.new(0, 0, 0), dark, nil, nil, Enum.Material.SmoothPlastic)
            part("SpeakerCone", Vector3.new(0.55, 0.55, 0.18), Vector3.new(0, 0, 0.35), cyan, nil, Enum.PartType.Ball)
            part("SpeakerTweeter", Vector3.new(0.18, 0.18, 0.12), Vector3.new(0, 0.36, 0.45), yellow, nil, Enum.PartType.Ball)
        elseif category == "CAMERA" then
            part("CameraBody", Vector3.new(1.1, 0.72, 0.45), Vector3.new(0, 0, 0), dark, nil, nil, Enum.Material.SmoothPlastic)
            part("CameraLens", Vector3.new(0.34, 0.34, 0.32), Vector3.new(0, 0, 0.34), cyan, Vector3.new(0, math.rad(90), 0), Enum.PartType.Cylinder)
            part("CameraViewfinder", Vector3.new(0.36, 0.18, 0.28), Vector3.new(-0.2, 0.47, 0), magenta)
        elseif category == "MONITOR" then
            part("MonitorFrame", Vector3.new(1.25, 0.82, 0.16), Vector3.new(0, 0.12, 0), cyan, nil, nil, Enum.Material.SmoothPlastic)
            part("MonitorScreen", Vector3.new(1.02, 0.6, 0.06), Vector3.new(0, 0.12, 0.11), dark, nil, nil, Enum.Material.SmoothPlastic)
            part("MonitorStand", Vector3.new(0.16, 0.28, 0.18), Vector3.new(0, -0.43, 0), C.WHITE, nil, nil, Enum.Material.SmoothPlastic)
            part("MonitorFoot", Vector3.new(0.62, 0.08, 0.26), Vector3.new(0, -0.56, 0), magenta)
        elseif category == "SNOW" then
            for i = 0, 2 do part("SnowflakeArm" .. i, Vector3.new(0.08, 1.15, 0.1), Vector3.zero, cyan, Vector3.new(0, 0, math.rad(i * 60))) end
            for i = 0, 5 do
                local a = math.rad(i * 60)
                part("SnowflakeTip" .. i, Vector3.new(0.22, 0.06, 0.1), Vector3.new(math.cos(a) * 0.42, math.sin(a) * 0.42, 0.04), yellow, Vector3.new(0, 0, math.rad(i * 60 + 40)))
            end
        elseif category == "TERRAIN" then
            part("TerrainBase", Vector3.new(1.5, 0.22, 0.9), Vector3.new(0, -0.4, 0), Color3.fromRGB(70, 128, 78), nil, nil, Enum.Material.Grass)
            part("TerrainHillLeft", Vector3.new(0.74, 0.48, 0.6), Vector3.new(-0.35, -0.12, 0), Color3.fromRGB(85, 160, 95), nil, Enum.PartType.Ball, Enum.Material.Grass)
            part("TerrainHillRight", Vector3.new(0.7, 0.36, 0.55), Vector3.new(0.38, -0.18, 0.05), Color3.fromRGB(65, 135, 82), nil, Enum.PartType.Ball, Enum.Material.Grass)
        elseif category == "MOON" then
            part("MoonDisc", Vector3.new(0.95, 0.95, 0.22), Vector3.new(-0.08, 0.05, 0), Color3.fromRGB(205, 221, 239), nil, Enum.PartType.Ball)
            part("MoonShadow", Vector3.new(0.86, 0.86, 0.24), Vector3.new(0.22, 0.2, 0.16), Color3.fromRGB(4, 8, 17), nil, Enum.PartType.Ball, Enum.Material.SmoothPlastic)
            part("MoonStarA", Vector3.new(0.12, 0.12, 0.12), Vector3.new(-0.55, 0.62, 0), cyan, nil, Enum.PartType.Ball)
            part("MoonStarB", Vector3.new(0.1, 0.1, 0.1), Vector3.new(0.55, -0.45, 0), magenta, nil, Enum.PartType.Ball)
        elseif category == "SUN" then
            part("SunCore", Vector3.new(0.7, 0.7, 0.25), Vector3.zero, yellow, nil, Enum.PartType.Ball)
            for i = 0, 7 do
                local a = math.rad(i * 45)
                part("SunRay" .. i, Vector3.new(0.08, 0.28, 0.08), Vector3.new(math.cos(a) * 0.55, math.sin(a) * 0.55, 0), yellow, Vector3.new(0, 0, a))
            end
        elseif category == "FOG" then
            part("FogCloudLeft", Vector3.new(0.85, 0.56, 0.28), Vector3.new(-0.28, 0, 0), Color3.fromRGB(173, 204, 229), nil, Enum.PartType.Ball, Enum.Material.SmoothPlastic)
            part("FogCloudCenter", Vector3.new(0.82, 0.68, 0.3), Vector3.new(0.08, 0.14, 0.08), C.WHITE, nil, Enum.PartType.Ball, Enum.Material.SmoothPlastic)
            part("FogCloudRight", Vector3.new(0.72, 0.5, 0.24), Vector3.new(0.39, -0.06, -0.04), Color3.fromRGB(139, 184, 218), nil, Enum.PartType.Ball, Enum.Material.SmoothPlastic)
        elseif category == "SPECTRUM" then
            part("SpectrumRed", Vector3.new(0.25, 0.85, 0.22), Vector3.new(-0.38, 0, 0), Color3.fromRGB(255, 65, 112), nil, Enum.PartType.Block)
            part("SpectrumGreen", Vector3.new(0.25, 0.85, 0.22), Vector3.new(0, 0, 0), C.GREEN, nil, Enum.PartType.Block)
            part("SpectrumBlue", Vector3.new(0.25, 0.85, 0.22), Vector3.new(0.38, 0, 0), cyan, nil, Enum.PartType.Block)
        elseif category == "AVATAR" then
            part("AvatarHead", Vector3.new(0.4, 0.4, 0.32), Vector3.new(0, 0.52, 0), Color3.fromRGB(255, 205, 157), nil, Enum.PartType.Ball)
            part("AvatarTorso", Vector3.new(0.48, 0.6, 0.28), Vector3.new(0, -0.05, 0), cyan, nil, nil, Enum.Material.SmoothPlastic)
            part("AvatarArmL", Vector3.new(0.18, 0.58, 0.18), Vector3.new(-0.36, -0.04, 0), magenta)
            part("AvatarArmR", Vector3.new(0.18, 0.58, 0.18), Vector3.new(0.36, -0.04, 0), magenta)
            part("AvatarLegL", Vector3.new(0.18, 0.48, 0.2), Vector3.new(-0.14, -0.58, 0), dark)
            part("AvatarLegR", Vector3.new(0.18, 0.48, 0.2), Vector3.new(0.14, -0.58, 0), dark)
        elseif category == "MOUSE" then
            part("MouseBody", Vector3.new(0.72, 1.05, 0.28), Vector3.new(0, -0.04, 0), C.WHITE, nil, Enum.PartType.Ball, Enum.Material.SmoothPlastic)
            part("MouseButtonL", Vector3.new(0.29, 0.35, 0.07), Vector3.new(-0.16, 0.25, 0.18), cyan)
            part("MouseButtonR", Vector3.new(0.29, 0.35, 0.07), Vector3.new(0.16, 0.25, 0.18), magenta)
            part("MouseWheel", Vector3.new(0.08, 0.18, 0.08), Vector3.new(0, 0.28, 0.24), yellow)
        elseif category == "RESET" then
            part("ResetCircleTop", Vector3.new(0.08, 0.85, 0.08), Vector3.new(0, 0, 0), cyan, nil)
            part("ResetCircleSideA", Vector3.new(0.08, 0.58, 0.08), Vector3.new(-0.24, 0.2, 0), cyan, Vector3.new(0, 0, math.rad(45)))
            part("ResetCircleSideB", Vector3.new(0.08, 0.58, 0.08), Vector3.new(0.24, 0.2, 0), cyan, Vector3.new(0, 0, math.rad(-45)))
            part("ResetArrowHead", Vector3.new(0.35, 0.14, 0.1), Vector3.new(-0.3, 0.47, 0.04), yellow, nil)
        elseif category == "DOOR" then
            part("DoorFrame", Vector3.new(0.92, 1.3, 0.14), Vector3.new(0, 0, 0), cyan, nil, nil, Enum.Material.SmoothPlastic)
            part("DoorPanel", Vector3.new(0.72, 1.12, 0.1), Vector3.new(0, -0.02, 0.12), dark, nil, nil, Enum.Material.SmoothPlastic)
            part("DoorHandle", Vector3.new(0.08, 0.08, 0.08), Vector3.new(0.22, -0.04, 0.2), yellow, nil, Enum.PartType.Ball)
        elseif category == "SHIELD" then
            part("ShieldPlate", Vector3.new(0.95, 1.15, 0.22), Vector3.new(0, 0, 0), cyan, Vector3.new(0, 0, math.rad(45)), nil, Enum.Material.Metal)
            part("ShieldCore", Vector3.new(0.3, 0.3, 0.12), Vector3.new(0, 0.02, 0.14), yellow, nil, Enum.PartType.Ball)
        elseif category == "GAUGE" then
            for i = 0, 7 do
                local a = math.rad(-140 + i * 40)
                part("GaugeTick" .. i, Vector3.new(0.06, 0.2, 0.08), Vector3.new(math.cos(a) * 0.45, math.sin(a) * 0.45, 0), cyan, Vector3.new(0, 0, a + math.pi / 2))
            end
            part("GaugeNeedle", Vector3.new(0.06, 0.58, 0.08), Vector3.new(0, 0.08, 0.08), yellow, Vector3.new(0, 0, math.rad(-36)))
            part("GaugePin", Vector3.new(0.16, 0.16, 0.12), Vector3.new(0, 0, 0.14), magenta, nil, Enum.PartType.Ball)
        else -- GEAR: the control's settings/system context.
            for i = 0, 7 do
                local a = math.rad(i * 45)
                part("GearTooth" .. i, Vector3.new(0.12, 0.32, 0.12), Vector3.new(math.cos(a) * 0.38, math.sin(a) * 0.38, 0), cyan, Vector3.new(0, 0, a))
            end
            part("GearHub", Vector3.new(0.42, 0.42, 0.18), Vector3.zero, magenta, nil, Enum.PartType.Cylinder)
        end
        return viewport
    end

    local function addSection(labelText)
        local sectionRow = new("Frame", {
            Name = "Section_" .. string.gsub(tostring(labelText), "[^%w]", "_"),
            Size = UDim2.new(1, -2, 0, 22),
            BackgroundTransparency = 1,
            BorderSizePixel = 0,
            ZIndex = 22,
        }, content)
        local h = addText(sectionRow, labelText, 11, C.MAGENTA, Enum.Font.Code)
        h.Position = UDim2.fromOffset(25, 0)
        h.Size = UDim2.new(1, -27, 1, 0)
        h.TextXAlignment = Enum.TextXAlignment.Left
        h.ZIndex = 23
        addMiniControlPreview(sectionRow, labelText, UDim2.fromOffset(3, 2), 18)
        return h
    end

    -- The external menu launcher and close control keep their original click targets.
    launcher.Text = "   CFG"
    launcher.TextXAlignment = Enum.TextXAlignment.Left
    launcher.TextSize = 11
    addMiniControlPreview(launcher, "open settings", UDim2.fromOffset(2, 7), 22)
    closeButton.Text = "  Ã"
    closeButton.TextXAlignment = Enum.TextXAlignment.Right
    closeButton.TextSize = 16
    addMiniControlPreview(closeButton, "close menu", UDim2.fromOffset(1, 6), 15)

    local function addInfo(labelText, detail)
        local row = new("Frame", {
            Size = UDim2.new(1, -2, 0, 32),
            BackgroundColor3 = C.PANEL2,
            BorderSizePixel = 0,
            ZIndex = 22,
        }, content)
        addCorner(row, 6)
        local t = addText(row, labelText .. "  /  " .. detail, 10, C.MUTED, Enum.Font.Code)
        t.Position = UDim2.new(0, 34, 0, 0)
        t.Size = UDim2.new(1, -40, 1, 0)
        t.TextXAlignment = Enum.TextXAlignment.Left
        addMiniControlPreview(row, labelText .. " " .. detail, UDim2.fromOffset(4, 4), 24)
        return t
    end

    local function addAction(labelText, callback)
        local b = addButton(content, "    " .. labelText, UDim2.new(1, -2, 0, 29))
        b.TextXAlignment = Enum.TextXAlignment.Left
        b.TextSize = 10
        b.ZIndex = 22
        addMiniControlPreview(b, labelText, UDim2.fromOffset(3, 3), 22)
        b.Activated:Connect(function()
            local ok, err = pcall(callback)
            if not ok then warn("[SYSTEM CFG] " .. tostring(err)) end
        end)
        return b
    end

    local refreshCrosshair3D
    local function addCrosshairPicker3D()
        local card = new("Frame", {
            Name = "CrosshairPicker3DCard",
            Size = UDim2.new(1, -2, 0, 270),
            BackgroundColor3 = C.BG,
            BorderSizePixel = 0,
            ZIndex = 22,
        }, content)
        addCorner(card, 8)
        addStroke(card, C.CYAN, 1)
        local heading = addText(card, "CROSSHAIR SELECTOR  /  LIVE 3D", 10, C.CYAN, Enum.Font.Code)
        heading.Position = UDim2.fromOffset(7, 4)
        heading.Size = UDim2.new(1, -14, 0, 18)
        heading.TextXAlignment = Enum.TextXAlignment.Left

        local viewport = new("ViewportFrame", {
            Name = "CrosshairLive3DViewport",
            Position = UDim2.new(0, 116, 0, 27),
            Size = UDim2.new(1, -123, 0, 211),
            BackgroundColor3 = Color3.fromRGB(3, 7, 16),
            BorderSizePixel = 0,
            LightColor = Color3.fromRGB(235, 245, 255),
            LightDirection = Vector3.new(-1, -1, -1),
            Ambient = Color3.fromRGB(125, 145, 170),
            ZIndex = 23,
        }, card)
        addCorner(viewport, 6)
        crosshairPreviewWorld = new("WorldModel", {Name = "CrosshairPreviewWorld"}, viewport)
        crosshairPreviewCamera = new("Camera", {Name = "CrosshairPreviewCamera", FieldOfView = 38}, viewport)
        viewport.CurrentCamera = crosshairPreviewCamera
        crosshairPreviewHeartLabel = new("TextLabel", {
            Name = "CrosshairPreviewHeart",
            AnchorPoint = Vector2.new(0.5, 0.5),
            Position = UDim2.new(0.5, 54.5, 0, 132.5),
            Size = UDim2.fromOffset(30, 30),
            BackgroundTransparency = 1,
            Text = "â¥",
            TextColor3 = C.YELLOW,
            TextStrokeColor3 = C.BG,
            TextStrokeTransparency = 0.15,
            TextSize = 18,
            Font = Enum.Font.GothamBold,
            Visible = false,
            ZIndex = 26,
        }, card)

        crosshairPreviewStatus = addText(card, "Selecciona una mira para previsualizarla.", 9, C.MUTED, Enum.Font.Code)
        crosshairPreviewStatus.Position = UDim2.new(0, 116, 0, 241)
        crosshairPreviewStatus.Size = UDim2.new(1, -123, 0, 24)
        crosshairPreviewStatus.TextXAlignment = Enum.TextXAlignment.Left
        crosshairPreviewStatus.TextYAlignment = Enum.TextYAlignment.Top

        local styleButtons = {}
        local function chooseStyle(styleId)
            state.crossStyle = styleId
            for _, entry in ipairs(styleButtons) do
                local selected = entry.styleId == styleId
                entry.button.BackgroundColor3 = selected and Color3.fromRGB(26, 50, 66) or C.PANEL2
                entry.button.TextColor3 = selected and C.CYAN or C.WHITE
                local stroke = entry.button:FindFirstChildOfClass("UIStroke")
                if stroke then stroke.Color = selected and C.MAGENTA or Color3.fromRGB(45, 111, 137) end
            end
            if refreshCrosshair3D then refreshCrosshair3D() end
        end

        for index, styleInfo in ipairs(CROSSHAIR_STYLES) do
            local b = addButton(card, styleInfo.label, UDim2.fromOffset(104, 19))
            b.Name = "CrosshairStyle_" .. styleInfo.id
            b.Position = UDim2.fromOffset(6, 27 + (index - 1) * 21)
            b.TextSize = 8
            b.TextXAlignment = Enum.TextXAlignment.Left
            b.ZIndex = 24
            pageConnect(b.Activated, function() chooseStyle(styleInfo.id) end)
            table.insert(styleButtons, {styleId = styleInfo.id, button = b})
        end

        refreshCrosshair3D = function()
            if not crosshairPreviewWorld or not crosshairPreviewWorld.Parent then return end
            for _, child in ipairs(crosshairPreviewWorld:GetChildren()) do child:Destroy() end
            table.clear(crosshairPreviewParts)

            local layout = getCrosshairLayout(state.crossStyle, state.crossRadius, state.crossLength, 0)
            local fixedColor = crossColorPalette[state.crossColor] or C.YELLOW
            local scale = 1 / 18
            local thickness = math.max(0.045, state.crossThickness * scale)
            for _, segmentInfo in ipairs(layout.segments) do
                local piece = new("Part", {
                    Name = "CrosshairSegment3D",
                    Anchored = true,
                    CanCollide = false,
                    CanTouch = false,
                    CanQuery = false,
                    Material = Enum.Material.Neon,
                    Color = fixedColor,
                    Size = Vector3.new(thickness * segmentInfo.thicknessScale, math.max(0.12, state.crossLength * scale * segmentInfo.lengthScale), 0.12),
                    CFrame = CFrame.new(segmentInfo.x * scale, -segmentInfo.y * scale, 0)
                        * CFrame.Angles(0, 0, math.rad(segmentInfo.rotation)),
                    CastShadow = false,
                }, crosshairPreviewWorld)
                table.insert(crosshairPreviewParts, piece)
            end

            if layout.ring then
                local ringRadius = math.max(0.18, state.crossRadius * scale)
                local ringPieces = 24
                local arcLength = (2 * math.pi * ringRadius / ringPieces) * 1.2
                for index = 0, ringPieces - 1 do
                    local angle = (index / ringPieces) * math.pi * 2
                    local piece = new("Part", {
                        Name = "CrosshairRingSegment3D",
                        Anchored = true,
                        CanCollide = false,
                        CanTouch = false,
                        CanQuery = false,
                        Material = Enum.Material.Neon,
                        Color = fixedColor,
                        Size = Vector3.new(thickness, arcLength, 0.10),
                        CFrame = CFrame.new(math.cos(angle) * ringRadius, math.sin(angle) * ringRadius, 0)
                            * CFrame.Angles(0, 0, angle),
                        CastShadow = false,
                    }, crosshairPreviewWorld)
                    table.insert(crosshairPreviewParts, piece)
                end
            end

            if layout.dot then
                local requestedCoreSize = state.crossDotSize
                local dotRadius = math.max(0.045, requestedCoreSize * scale)
                local core = new("Part", {
                    Name = layout.heart and "CrosshairHeartCore3D" or "CrosshairCenterDot3D",
                    Shape = Enum.PartType.Ball,
                    Anchored = true,
                    CanCollide = false,
                    CanTouch = false,
                    CanQuery = false,
                    Material = Enum.Material.Neon,
                    Color = layout.heart and C.YELLOW or fixedColor,
                    Size = Vector3.new(dotRadius * 2, dotRadius * 2, dotRadius * 1.5),
                    CFrame = CFrame.new(0, 0, 0.08),
                    CastShadow = false,
                }, crosshairPreviewWorld)
                table.insert(crosshairPreviewParts, core)
            end

            local largestExtent = (state.crossRadius * scale) + (state.crossLength * scale) + 0.8
            local cameraDistance = math.clamp(largestExtent * 2.45, 4.5, 13)
            crosshairPreviewCamera.CFrame = CFrame.lookAt(Vector3.new(0, 0, cameraDistance), Vector3.zero)
            crosshairPreviewCamera.FieldOfView = 38
            if crosshairPreviewStatus then
                crosshairPreviewStatus.Text = string.format("%s\nGrosor %d  Â·  Longitud %d", state.crossStyle, state.crossThickness, state.crossLength)
            end
            if crosshairPreviewHeartLabel then
                crosshairPreviewHeartLabel.Visible = layout.heart
                crosshairPreviewHeartLabel.TextSize = math.clamp(state.heartSize, 10, 34)
                crosshairPreviewHeartLabel.Size = UDim2.fromOffset(state.heartSize + 12, state.heartSize + 12)
                crosshairPreviewHeartLabel.TextColor3 = fixedColor
                crosshairPreviewHeartLabel.TextStrokeColor3 = C.BG
            end
            for _, entry in ipairs(styleButtons) do
                local selected = entry.styleId == state.crossStyle
                entry.button.BackgroundColor3 = selected and Color3.fromRGB(26, 50, 66) or C.PANEL2
                entry.button.TextColor3 = selected and C.CYAN or C.WHITE
                local stroke = entry.button:FindFirstChildOfClass("UIStroke")
                if stroke then stroke.Color = selected and C.MAGENTA or Color3.fromRGB(45, 111, 137) end
            end
        end
        chooseStyle(state.crossStyle)
        return card
    end

    local function addMapPreviewCard()
        local card = new("Frame", {
            Name = "MapSnapshot3DCard",
            Size = UDim2.new(1, -2, 0, 278),
            BackgroundColor3 = C.BG,
            BorderSizePixel = 0,
            ZIndex = 22,
        }, content)
        addCorner(card, 8)
        addStroke(card, C.CYAN, 1)
        local heading = addText(card, "MAP_SNAPSHOT  /  3D", 11, C.CYAN, Enum.Font.Code)
        heading.Position = UDim2.fromOffset(8, 4)
        heading.Size = UDim2.new(1, -16, 0, 19)
        heading.TextXAlignment = Enum.TextXAlignment.Left

        local viewport = new("ViewportFrame", {
            Name = "MapPreviewViewport",
            Position = UDim2.new(0, 7, 0, 25),
            Size = UDim2.new(1, -14, 0, 132),
            BackgroundColor3 = Color3.fromRGB(4, 8, 17),
            BorderSizePixel = 0,
            LightColor = Color3.fromRGB(235, 245, 255),
            LightDirection = Vector3.new(-1, -1, -1),
            Ambient = Color3.fromRGB(130, 145, 165),
            ZIndex = 23,
        }, card)
        addCorner(viewport, 6)
        mapPreviewWorld = new("WorldModel", {Name = "MapSnapshotWorld"}, viewport)
        mapPreviewCamera = new("Camera", {Name = "MapSnapshotCamera"}, viewport)
        viewport.CurrentCamera = mapPreviewCamera

        local scanButton = addButton(card, "    DESCARGAR MAPA  /  ESCANEAR", UDim2.new(1, -14, 0, 27))
        scanButton.Position = UDim2.new(0, 7, 0, 162)
        scanButton.TextSize = 9
        scanButton.TextXAlignment = Enum.TextXAlignment.Left
        scanButton.TextColor3 = C.CYAN
        scanButton.ZIndex = 24
        addMiniControlPreview(scanButton, "mapa escanear terreno", UDim2.fromOffset(2, 2), 22)
        pageConnect(scanButton.Activated, function() scanLoadedMapForPreview(false) end)

        local streamButton = addButton(card, "    ACTUALIZAR MAPA / MÃS ZONAS", UDim2.new(1, -14, 0, 27))
        streamButton.Position = UDim2.new(0, 7, 0, 192)
        streamButton.TextSize = 9
        streamButton.TextXAlignment = Enum.TextXAlignment.Left
        streamButton.TextColor3 = C.CYAN
        streamButton.ZIndex = 24
        addMiniControlPreview(streamButton, "mapa streaming terreno", UDim2.fromOffset(2, 2), 22)
        pageConnect(streamButton.Activated, function()
            if mapBridge and mapBridge:IsA("RemoteFunction") and not serverBridgeFailed then
                scanLoadedMapForPreview(false)
            else
                requestMoreMapChunks()
            end
        end)

        local snowButton = addButton(card, "    PONER NIEVE EN TODO EL MAPA CARGADO", UDim2.new(1, -14, 0, 27))
        snowButton.Position = UDim2.new(0, 7, 0, 222)
        snowButton.TextSize = 9
        snowButton.TextXAlignment = Enum.TextXAlignment.Left
        snowButton.TextColor3 = C.WHITE
        addMiniControlPreview(snowButton, "nieve en mapa", UDim2.fromOffset(2, 2), 22)
        snowButton.BackgroundColor3 = Color3.fromRGB(23, 40, 57)
        addStroke(snowButton, C.MAGENTA, 1.2)
        snowButton.ZIndex = 24
        pageConnect(snowButton.Activated, function()
            if not mapSnapshotReady then
                scanLoadedMapForPreview(true)
            else
                setAccumulation(true)
            end
        end)

        mapPreviewStatus = addText(card, "Pulsa ESCANEAR MAPA para crear una vista 3D.", 9, C.MUTED, Enum.Font.Code)
        mapPreviewStatus.Position = UDim2.new(0, 8, 0, 252)
        mapPreviewStatus.Size = UDim2.new(1, -16, 0, 18)
        mapPreviewStatus.TextXAlignment = Enum.TextXAlignment.Left
        mapPreviewStatus.ZIndex = 24
        if mapSnapshotReady then renderMapSnapshotPreview() end
    end

    local function addToggle(labelText, key, callback)
        local row = new("Frame", {
            Size = UDim2.new(1, -2, 0, 34),
            BackgroundColor3 = C.PANEL2,
            BorderSizePixel = 0,
            ZIndex = 22,
        }, content)
        addCorner(row, 6)
        local t = addText(row, labelText, 11, C.WHITE, Enum.Font.Code)
        t.Position = UDim2.new(0, 34, 0, 0)
        t.Size = UDim2.new(1, -112, 1, 0)
        t.TextXAlignment = Enum.TextXAlignment.Left
        addMiniControlPreview(row, labelText, UDim2.fromOffset(5, 5), 24)
        local b = addButton(row, state[key] and "ON" or "OFF", UDim2.fromOffset(60, 25))
        b.Position = UDim2.new(1, -66, 0.5, -12)
        b.ZIndex = 23
        b.TextColor3 = state[key] and C.GREEN or C.MUTED
        b.Activated:Connect(function()
            state[key] = not state[key]
            b.Text = state[key] and "ON" or "OFF"
            b.TextColor3 = state[key] and C.GREEN or C.MUTED
            if callback then
                local ok, err = pcall(callback, state[key])
                if not ok then warn("[SYSTEM CFG] " .. tostring(err)) end
            end
        end)
        return b
    end

    local function formatNumber(v, step)
        if step and step < 1 then return string.format("%.2f", v) end
        return tostring(math.floor(v + 0.5))
    end

    local function addSlider(labelText, minValue, maxValue, initial, step, callback)
        step = step or 1
        local row = new("Frame", {
            Size = UDim2.new(1, -2, 0, 52),
            BackgroundColor3 = C.PANEL2,
            BorderSizePixel = 0,
            ZIndex = 22,
        }, content)
        addCorner(row, 6)
        local titleLabel = addText(row, labelText, 11, C.WHITE, Enum.Font.Code)
        titleLabel.Position = UDim2.new(0, 31, 0, 2)
        titleLabel.Size = UDim2.new(1, -101, 0, 18)
        titleLabel.TextXAlignment = Enum.TextXAlignment.Left
        addMiniControlPreview(row, labelText, UDim2.fromOffset(4, 2), 23)
        local number = addText(row, formatNumber(initial, step), 11, C.CYAN, Enum.Font.Code)
        number.Position = UDim2.new(1, -62, 0, 2)
        number.Size = UDim2.new(0, 54, 0, 18)
        number.TextXAlignment = Enum.TextXAlignment.Right
        local track = new("Frame", {
            Name = "Track",
            Position = UDim2.new(0, 10, 0, 31),
            Size = UDim2.new(1, -20, 0, 6),
            BackgroundColor3 = Color3.fromRGB(47, 54, 70),
            BorderSizePixel = 0,
            Active = true,
            ZIndex = 23,
        }, row)
        addCorner(track, 4)
        local ratio = math.clamp((initial - minValue) / math.max(maxValue - minValue, 0.00001), 0, 1)
        local fill = new("Frame", {
            Size = UDim2.new(ratio, 0, 1, 0),
            BackgroundColor3 = C.MAGENTA,
            BorderSizePixel = 0,
            ZIndex = 24,
        }, track)
        addCorner(fill, 4)
        local knob = new("Frame", {
            AnchorPoint = Vector2.new(0.5, 0.5),
            Position = UDim2.new(ratio, 0, 0.5, 0),
            Size = UDim2.fromOffset(12, 12),
            BackgroundColor3 = C.CYAN,
            BorderSizePixel = 0,
            ZIndex = 25,
        }, track)
        addCorner(knob, 6)
        addStroke(knob, C.BG, 1)
        local control = {track = track, fill = fill, knob = knob, number = number,
            min = minValue, max = maxValue, step = step, callback = callback}
        track.InputBegan:Connect(function(input)
            if input.UserInputType == Enum.UserInputType.MouseButton1 or input.UserInputType == Enum.UserInputType.Touch then
                activeSlider = {control = control, input = input}
                local alpha = math.clamp((input.Position.X - track.AbsolutePosition.X) / math.max(track.AbsoluteSize.X, 1), 0, 1)
                local raw = minValue + (maxValue - minValue) * alpha
                local value = minValue + math.floor((raw - minValue) / step + 0.5) * step
                value = math.clamp(value, minValue, maxValue)
                local r = (value - minValue) / math.max(maxValue - minValue, 0.00001)
                fill.Size = UDim2.new(r, 0, 1, 0)
                knob.Position = UDim2.new(r, 0, 0.5, 0)
                number.Text = formatNumber(value, step)
                if callback then callback(value) end
            end
        end)
        return row
    end

    -- Global input router for sliders, drag operations and avatar turn. Connected once.
    globalConnect(UserInputService.InputChanged, function(input)
        if freeCameraState.enabled and input.UserInputType == Enum.UserInputType.MouseMovement then
            local rightHeld = false
            pcall(function() rightHeld = UserInputService:IsMouseButtonPressed(Enum.UserInputType.MouseButton2) end)
            if rightHeld then
                freeCameraState.yaw -= input.Delta.X * 0.0035
                freeCameraState.pitch = math.clamp(freeCameraState.pitch - input.Delta.Y * 0.0035, -1.45, 1.45)
            end
        elseif freeCameraState.enabled and input.UserInputType == Enum.UserInputType.MouseWheel then
            local camera = workspace.CurrentCamera
            if camera then
                local wheel = input.Position.Z
                if freeCameraState.orbit and state.cameraOrbit then
                    state.cameraOrbitDistance = math.clamp(state.cameraOrbitDistance - wheel * 8, 10, MAX_CAMERA_ORBIT_DISTANCE)
                else
                    freeCameraState.pos += camera.CFrame.LookVector * wheel * state.cameraSpeed * 0.8
                end
            end
        end
        if activeSlider then
            local startInput = activeSlider.input
            local isMouseMove = startInput.UserInputType == Enum.UserInputType.MouseButton1 and input.UserInputType == Enum.UserInputType.MouseMovement
            local isTouchMove = startInput.UserInputType == Enum.UserInputType.Touch and input == startInput
            if isMouseMove or isTouchMove then
                local c = activeSlider.control
                if not c.track.Parent then activeSlider = nil; return end
                local alpha = math.clamp((input.Position.X - c.track.AbsolutePosition.X) / math.max(c.track.AbsoluteSize.X, 1), 0, 1)
                local raw = c.min + (c.max - c.min) * alpha
                local value = c.min + math.floor((raw - c.min) / c.step + 0.5) * c.step
                value = math.clamp(value, c.min, c.max)
                local ratio = (value - c.min) / math.max(c.max - c.min, 0.00001)
                c.fill.Size = UDim2.new(ratio, 0, 1, 0)
                c.knob.Position = UDim2.new(ratio, 0, 0.5, 0)
                c.number.Text = formatNumber(value, c.step)
                if c.callback then
                    local ok, err = pcall(c.callback, value)
                    if not ok then warn("[SYSTEM CFG] Slider: " .. tostring(err)) end
                end
            end
        end
        if dragState then
            local ds = dragState
            local isMouseMove = ds.input.UserInputType == Enum.UserInputType.MouseButton1 and input.UserInputType == Enum.UserInputType.MouseMovement
            local isTouchMove = ds.input.UserInputType == Enum.UserInputType.Touch and input == ds.input
            if isMouseMove or isTouchMove then
                local delta = Vector2.new(input.Position.X, input.Position.Y) - ds.pointerStart
                if delta.Magnitude > 3 then ds.moved = true end
                local x, y = clampPosition(ds.target, ds.objectStart.X + delta.X, ds.objectStart.Y + delta.Y)
                if ds.target == rifleAim then
                    local camera = workspace.CurrentCamera
                    if camera then
                        state.rifleX = x + rifleAim.AbsoluteSize.X/2 - camera.ViewportSize.X/2
                        state.rifleY = y + rifleAim.AbsoluteSize.Y/2 - camera.ViewportSize.Y/2
                        rifleAim.Position = UDim2.new(0.5, state.rifleX, 0.5, state.rifleY)
                    end
                else
                    ds.target.Position = UDim2.fromOffset(x, y)
                end
                if ds.onMoved then pcall(ds.onMoved, Vector2.new(x, y)) end
            end
        end
        if avatarDragInput then
            local isMouseMove = avatarDragInput.UserInputType == Enum.UserInputType.MouseButton1 and input.UserInputType == Enum.UserInputType.MouseMovement
            local isTouchMove = avatarDragInput.UserInputType == Enum.UserInputType.Touch and input == avatarDragInput
            if isMouseMove or isTouchMove then
                local deltaX = input.Position.X - avatarDragStart.X
                state.avatarYaw = avatarYawStart + deltaX * 0.008
            end
        end
    end)
    -- Mobile two-finger pinch controls orbit distance while the custom orbit camera is active.
    local previousPinchScale = 1
    globalConnect(UserInputService.TouchPinch, function(touchPositions, scale, velocity, pinchState, gameProcessedEvent)
        if pinchState == Enum.UserInputState.Begin then
            previousPinchScale = 1
            return
        end
        if pinchState == Enum.UserInputState.End or pinchState == Enum.UserInputState.Cancel then
            previousPinchScale = 1
            return
        end
        if gameProcessedEvent or not freeCameraState.enabled or not freeCameraState.orbit or not state.cameraOrbit then return end
        if pinchState ~= Enum.UserInputState.Change then return end
        if type(scale) ~= "number" or scale <= 0 then return end
        -- Apply only incremental scale change so a long pinch is smooth, not exponential.
        local incrementalScale = scale / math.max(previousPinchScale, 0.001)
        previousPinchScale = scale
        -- incrementalScale > 1 means fingers spread apart (zoom in); < 1 means zoom out.
        state.cameraOrbitDistance = math.clamp(state.cameraOrbitDistance / incrementalScale, 10, MAX_CAMERA_ORBIT_DISTANCE)
    end)

    globalConnect(UserInputService.InputEnded, function(input)
        if activeSlider and (input == activeSlider.input or input.UserInputType == Enum.UserInputType.MouseButton1) then activeSlider = nil end
        if dragState and (input == dragState.input or input.UserInputType == Enum.UserInputType.MouseButton1) then
            local target = dragState.target
            local moved = dragState.moved
            dragState = nil
            if target == launcher and moved then launcher:SetAttribute("Dragged", true) end
        end
        if avatarDragInput and (input == avatarDragInput or input.UserInputType == Enum.UserInputType.MouseButton1) then avatarDragInput = nil end
    end)

    local tabButtons = {}
    local tabSpecs = {
        {"AIM", "AIM"}, {"CAM", "CAM"}, {"WORLD", "WORLD"},
        {"HUD", "HUD"}, {"AVATAR", "AVTR"}, {"SYS", "SYS"},
    }

    local function applyZoom()
        state.zoomMin = math.clamp(state.zoomMin, 0.5, math.max(0.5, state.zoomMax - 1))
        state.zoomMax = math.clamp(state.zoomMax, state.zoomMin + 1, MAX_CAMERA_ZOOM_DISTANCE)
        player.CameraMinZoomDistance = state.zoomMin
        player.CameraMaxZoomDistance = state.zoomMax
    end

    local function resetCamera()
        restoreCamera()
        local camera = workspace.CurrentCamera
        if camera then camera.FieldOfView = 70 end
        state.fov = 70
        state.zoomMin = 0.5
        state.zoomMax = MAX_CAMERA_ZOOM_DISTANCE
        state.cameraUnlocked = true
        player.CameraMode = Enum.CameraMode.Classic
        applyZoom()
    end

    local function setRifleVisible(on)
        state.rifle = on
        rifleAim.Visible = on
    end

    local function moveCrosshair()
        local camera = workspace.CurrentCamera
        local view = camera and camera.ViewportSize or Vector2.new(900, 600)
        local halfW, halfH = mainAim.AbsoluteSize.X * 0.5, mainAim.AbsoluteSize.Y * 0.5
        state.crossX = math.clamp(state.crossX, -view.X * 0.5 + halfW + 2, view.X * 0.5 - halfW - 2)
        state.crossY = math.clamp(state.crossY, -view.Y * 0.5 + halfH + 2, view.Y * 0.5 - halfH - 2)
        mainAim.Position = UDim2.new(0.5, state.crossX, 0.5, state.crossY)
    end

    local aimUpButton = addButton(gui, "+5", UDim2.fromOffset(44, 34))
    aimUpButton.Name = "AimNudgeUp"
    aimUpButton.Position = UDim2.new(0, 10, 0, 108)
    aimUpButton.TextSize = 14
    aimUpButton.TextColor3 = C.CYAN
    aimUpButton.ZIndex = 46
    addStroke(aimUpButton, C.CYAN, 1.4)
    local aimDownButton = addButton(gui, "â5", UDim2.fromOffset(44, 34))
    aimDownButton.Name = "AimNudgeDown"
    aimDownButton.AnchorPoint = Vector2.new(0, 1)
    aimDownButton.Position = UDim2.new(0, 10, 1, -68)
    aimDownButton.TextSize = 14
    aimDownButton.TextColor3 = C.MAGENTA
    aimDownButton.ZIndex = 46
    addStroke(aimDownButton, C.MAGENTA, 1.4)
    local function nudgeCrosshair(deltaY)
        if state.crossFrozen then return end
        state.crossY += deltaY
        moveCrosshair()
    end
    aimUpButton.Activated:Connect(function() nudgeCrosshair(-5) end)
    aimDownButton.Activated:Connect(function() nudgeCrosshair(5) end)

    -- Two low-overhead mobile autoclickers. They call only the equipped Tool's supported Activate API.
    local clickerState = {
        A = {enabled = false, generation = 0},
        B = {enabled = false, generation = 0},
    }
    local clickerButtons = {}
    local clickerLastToolUse = setmetatable({}, {__mode = "k"})

    local clickerAButton = addButton(gui, "AC1 OFF", UDim2.fromOffset(62, 30))
    clickerAButton.Name = "AutoClickerA"
    clickerAButton.AnchorPoint = Vector2.new(1, 0.5)
    clickerAButton.Position = UDim2.new(1, -10, 0.42, 0)
    clickerAButton.ZIndex = 47
    clickerAButton.TextSize = 10
    clickerAButton.TextColor3 = C.CYAN
    clickerAButton.BackgroundColor3 = Color3.fromRGB(7, 21, 31)
    addStroke(clickerAButton, C.CYAN, 1.3)
    clickerButtons.A = clickerAButton

    local clickerBButton = addButton(gui, "AC2 OFF", UDim2.fromOffset(62, 30))
    clickerBButton.Name = "AutoClickerB"
    clickerBButton.AnchorPoint = Vector2.new(1, 0.5)
    clickerBButton.Position = UDim2.new(1, -10, 0.42, 34)
    clickerBButton.ZIndex = 47
    clickerBButton.TextSize = 10
    clickerBButton.TextColor3 = C.MAGENTA
    clickerBButton.BackgroundColor3 = Color3.fromRGB(31, 7, 23)
    addStroke(clickerBButton, C.MAGENTA, 1.3)
    clickerButtons.B = clickerBButton

    local function updateClickerButton(which)
        local data = clickerState[which]
        local button = clickerButtons[which]
        if not button then return end
        button.Text = (which == "A" and "AC1 " or "AC2 ") .. (data.enabled and "ON" or "OFF")
        button.BackgroundColor3 = data.enabled
            and (which == "A" and Color3.fromRGB(0, 62, 70) or Color3.fromRGB(73, 0, 38))
            or (which == "A" and Color3.fromRGB(7, 21, 31) or Color3.fromRGB(31, 7, 23))
        button.TextColor3 = data.enabled and C.WHITE or (which == "A" and C.CYAN or C.MAGENTA)
    end

    local function activateEquippedTool()
        local character = player.Character
        if not character then return false end
        local tool = nil
        for _, child in ipairs(character:GetChildren()) do
            if child:IsA("Tool") and child.Enabled then tool = child; break end
        end
        if not tool then return false end
        local now = os.clock()
        if clickerLastToolUse[tool] and now - clickerLastToolUse[tool] < 0.045 then return false end
        clickerLastToolUse[tool] = now
        local ok = pcall(function() tool:Activate() end)
        if ok then
            task.delay(0.025, function()
                if tool and tool.Parent == character then pcall(function() tool:Deactivate() end) end
            end)
            return true
        end
        return false
    end

    local function setAutoClicker(which, enabled)
        local data = clickerState[which]
        if not data then return end
        data.generation += 1
        local generation = data.generation
        data.enabled = enabled == true
        state[which == "A" and "autoClickA" or "autoClickB"] = data.enabled
        updateClickerButton(which)
        if not data.enabled then return end
        task.spawn(function()
            if which == "B" then task.wait(0.05) end
            while gui.Parent and data.enabled and data.generation == generation do
                if not state.photoMode then activateEquippedTool() end
                task.wait(0.1)
            end
        end)
    end

    clickerAButton.Activated:Connect(function()
        if clickerAButton:GetAttribute("Dragged") then clickerAButton:SetAttribute("Dragged", false); return end
        setAutoClicker("A", not clickerState.A.enabled)
    end)
    clickerBButton.Activated:Connect(function()
        if clickerBButton:GetAttribute("Dragged") then clickerBButton:SetAttribute("Dragged", false); return end
        setAutoClicker("B", not clickerState.B.enabled)
    end)

    local function setClickerButtonsVisible(on)
        state.autoClickButtonsVisible = on
        clickerAButton.Visible = on and not state.photoMode
        clickerBButton.Visible = on and not state.photoMode
    end

    local function stopBothClickers()
        setAutoClicker("A", false)
        setAutoClicker("B", false)
    end

    local function setFpsPosition(cornerName)
        state.fpsCorner = cornerName
        local camera = workspace.CurrentCamera
        local view = camera and camera.ViewportSize or Vector2.new(900, 600)
        local width, height = fpsWidget.AbsoluteSize.X, fpsWidget.AbsoluteSize.Y
        local x = (cornerName == "TR" or cornerName == "BR") and (view.X - width - 12) or 12
        local y = (cornerName == "BL" or cornerName == "BR") and (view.Y - height - 12) or 8
        fpsWidget.Position = UDim2.fromOffset(math.max(0, x), math.max(0, y))
    end

    local function populateTab(tabName)
        state.activeTab = tabName
        activeSlider = nil
        clearPage()
        for name, b in pairs(tabButtons) do
            b.BackgroundColor3 = name == tabName and Color3.fromRGB(27, 39, 59) or C.PANEL2
            local uiStroke = b:FindFirstChildOfClass("UIStroke")
            if uiStroke then uiStroke.Color = name == tabName and C.MAGENTA or C.CYAN end
            b.TextColor3 = name == tabName and C.CYAN or C.WHITE
        end

        if tabName == "AIM" then
            addSection("CROSSHAIR_ENGINE")
            addToggle("Mira principal", "crosshair", function(on) mainAim.Visible = on end)
            addToggle("Rainbow RGB", "rainbow", function() if refreshCrosshair3D then refreshCrosshair3D() end end)
            addToggle("Giro orbital", "crossRotate", function() if refreshCrosshair3D then refreshCrosshair3D() end end)
            addSlider("SeparaciÃ³n central / radio", 5, 65, state.crossRadius, 1, function(v)
                state.crossRadius = v
                if refreshCrosshair3D then refreshCrosshair3D() end
            end)
            addSlider("Longitud de lÃ­neas", 3, 40, state.crossLength, 1, function(v)
                state.crossLength = v
                for _, line in ipairs(aimLines) do line.Size = UDim2.fromOffset(state.crossThickness, v) end
                if refreshCrosshair3D then refreshCrosshair3D() end
            end)
            addSlider("Grosor de lÃ­neas", 1, 10, state.crossThickness, 1, function(v)
                state.crossThickness = v
                for _, line in ipairs(aimLines) do line.Size = UDim2.fromOffset(v, state.crossLength) end
                if refreshCrosshair3D then refreshCrosshair3D() end
            end)
            addSlider("TamaÃ±o del corazÃ³n", 6, 36, state.heartSize, 1, function(v)
                state.heartSize = v; heart.TextSize = v; heart.Size = UDim2.fromOffset(v + 10, v + 10)
                if refreshCrosshair3D then refreshCrosshair3D() end
            end)
            addSlider("TamaÃ±o del punto central", 1, 12, state.crossDotSize, 1, function(v)
                state.crossDotSize = v
                if refreshCrosshair3D then refreshCrosshair3D() end
            end)
            addSection("RETICLE_STYLE  /  ELIGE UNA DE 10")
            addCrosshairPicker3D()
            addSection("COLOR_SELECTOR")
            addAction("COLOR: AMARILLO", function() state.crossColor = "YELLOW"; if refreshCrosshair3D then refreshCrosshair3D() end end)
            addAction("COLOR: CIAN", function() state.crossColor = "CYAN"; if refreshCrosshair3D then refreshCrosshair3D() end end)
            addAction("COLOR: MAGENTA", function() state.crossColor = "MAGENTA"; if refreshCrosshair3D then refreshCrosshair3D() end end)
            addAction("COLOR: VERDE", function() state.crossColor = "GREEN"; if refreshCrosshair3D then refreshCrosshair3D() end end)
            addAction("COLOR: BLANCO", function() state.crossColor = "WHITE"; if refreshCrosshair3D then refreshCrosshair3D() end end)
            addSlider("Velocidad de giro de mira (mÃ¡x. 100.000Â°/s)", 0, 100000, state.rotationSpeed, 100, function(v) state.rotationSpeed = v end)
            addSlider("Velocidad RGB", 1, 100, math.floor(state.rainbowSpeed * 100), 1, function(v) state.rainbowSpeed = v / 100 end)
            addToggle("Bloquear posiciÃ³n de mira", "crossFrozen", function(on)
                state.crossFrozen = on
                aimUpButton.Active = not on
                aimDownButton.Active = not on
                aimUpButton.TextColor3 = on and C.MUTED or C.CYAN
                aimDownButton.TextColor3 = on and C.MUTED or C.MAGENTA
            end)
            addToggle("Botones rÃ¡pidos +5 / â5", "aimButtonsVisible", function(on) aimUpButton.Visible = on and not state.photoMode; aimDownButton.Visible = on and not state.photoMode end)
            addAction("CENTRAR MIRA", function() if not state.crossFrozen then state.crossX = 0; state.crossY = -34; moveCrosshair() end end)
            addSection("RIFLE_MATRIX")
            addToggle("Mira de rifle", "rifle", setRifleVisible)
            addToggle("Rifle arcoÃ­ris", "rifleRainbow", function() end)
            addSlider("Rifle tamaÃ±o", 20, 160, state.rifleSize, 2, function(v)
                state.rifleSize = v; rifleAim.Size = UDim2.fromOffset(v, v)
                for _, line in ipairs(rifleLines) do line.Size = UDim2.fromOffset(state.rifleThickness, state.rifleLength) end
            end)
            addSlider("Rifle X", -400, 400, state.rifleX, 1, function(v) state.rifleX = v; rifleAim.Position = UDim2.new(0.5, v, 0.5, state.rifleY) end)
            addSlider("Rifle Y", -400, 400, state.rifleY, 1, function(v) state.rifleY = v; rifleAim.Position = UDim2.new(0.5, state.rifleX, 0.5, v) end)
            addSlider("Grosor rifle", 1, 8, state.rifleThickness, 1, function(v) state.rifleThickness = v; for _, line in ipairs(rifleLines) do line.Size = UDim2.fromOffset(v, state.rifleLength) end end)
            addSlider("Longitud rifle", 3, 32, state.rifleLength, 1, function(v) state.rifleLength = v; for _, line in ipairs(rifleLines) do line.Size = UDim2.fromOffset(state.rifleThickness, v) end end)
        elseif tabName == "CAM" then
            addSection("CAMERA_MATRIX")
            addToggle("CÃ¡mara libre", "freeCamera", setFreeCamera)
            addSlider("FOV vertical (10Â°â120Â° mÃ¡x.)", 10, 120, state.fov, 1, function(v)
                state.fov = v
                local camera = workspace.CurrentCamera
                if camera then camera.FieldOfView = v end
            end)
            addSlider("Velocidad cÃ¡mara libre", 5, 150, state.cameraSpeed, 1, function(v) state.cameraSpeed = v end)
            addToggle("Ãrbita centrada en personaje", "cameraOrbit", function(on)
                if on then
                    if not freeCameraState.enabled then setFreeCamera(true) end
                    local camera = workspace.CurrentCamera
                    local character = player.Character
                    local root = character and character:FindFirstChild("HumanoidRootPart")
                    if camera and root then
                        freeCameraState.orbit = true
                        local look = camera.CFrame.LookVector
                        freeCameraState.pitch = math.clamp(math.asin(look.Y), -1.1, 1.1)
                        freeCameraState.yaw = math.atan2(-look.X, -look.Z)
                        state.cameraOrbit = true
                    end
                else
                    freeCameraState.orbit = false
                    state.cameraOrbit = false
                end
            end)
            addSlider("Distancia orbital al personaje (10â5000 studs)", 10, MAX_CAMERA_ORBIT_DISTANCE, state.cameraOrbitDistance, 10, function(v) state.cameraOrbitDistance = v end)
            addAction("ALEJAR CÃMARA +50", function()
                if not freeCameraState.enabled then setFreeCamera(true) end
                if not freeCameraState.orbit then
                    freeCameraState.orbit = true
                    state.cameraOrbit = true
                    state.cameraOrbitDistance = math.max(state.cameraOrbitDistance, 50)
                    local camera = workspace.CurrentCamera
                    local look = camera and camera.CFrame.LookVector or Vector3.new(0, 0, -1)
                    freeCameraState.pitch = math.clamp(math.asin(look.Y), -1.1, 1.1)
                    freeCameraState.yaw = math.atan2(-look.X, -look.Z)
                end
                state.cameraOrbitDistance = math.clamp(state.cameraOrbitDistance + 50, 10, MAX_CAMERA_ORBIT_DISTANCE)
            end)
            addAction("ACERCAR CÃMARA â50", function()
                if not freeCameraState.enabled then setFreeCamera(true) end
                if freeCameraState.orbit then state.cameraOrbitDistance = math.clamp(state.cameraOrbitDistance - 50, 10, MAX_CAMERA_ORBIT_DISTANCE)
                else
                    freeCameraState.pos += (workspace.CurrentCamera and workspace.CurrentCamera.CFrame.LookVector or Vector3.new(0,0,-1)) * 50
                end
            end)
            addAction("DESBLOQUEAR ZOOM (HASTA 5000 STUDS)", function()
                state.cameraUnlocked = true
                player.CameraMode = Enum.CameraMode.Classic
                state.zoomMin = 0.5
                state.zoomMax = MAX_CAMERA_ZOOM_DISTANCE
                applyZoom()
            end)
            addAction("SALIR DE CÃMARA LIBRE / VOLVER AL PERSONAJE", function()
                state.cameraOrbit = false
                freeCameraState.orbit = false
                if freeCameraState.enabled then restoreCamera() end
                local cam = workspace.CurrentCamera
                local character = player.Character
                local humanoid = character and character:FindFirstChildOfClass("Humanoid")
                if cam and humanoid then
                    player.CameraMode = state.cameraUnlocked and Enum.CameraMode.Classic or player.CameraMode
                    cam.CameraType = Enum.CameraType.Custom
                    cam.CameraSubject = humanoid
                end
            end)
            addSlider("Zoom mÃ­nimo", 0.5, 20, state.zoomMin, 0.5, function(v) state.zoomMin = v; applyZoom() end)
            addSlider("Zoom mÃ¡ximo (hasta 5000 studs)", 10, MAX_CAMERA_ZOOM_DISTANCE, state.zoomMax, 10, function(v) state.zoomMax = v; applyZoom() end)
            addAction("ZOOM IN â50", function()
                state.zoomMax = math.max(state.zoomMin + 1, state.zoomMax - 50)
                applyZoom()
            end)
            addAction("ZOOM OUT +50", function()
                state.zoomMax = math.min(MAX_CAMERA_ZOOM_DISTANCE, state.zoomMax + 50)
                applyZoom()
            end)
            addAction("RESTAURAR CÃMARA", resetCamera)
            addInfo("Controles PC", "WASD + Q/E + flechas; rueda para acercar/alejar")
            addInfo("Ãrbita", "mantiene el personaje centrado y permite alejar hasta 5000 studs")
            addInfo("Zoom", "rueda del ratÃ³n o pellizco con dos dedos en mÃ³vil ajusta la distancia orbital.")
            addInfo("FOV", "el campo de visiÃ³n de esta interfaz se limita a 120Â°.")
            addInfo("Controles mÃ³vil", "aparecen al activar cÃ¡mara libre")
        elseif tabName == "WORLD" then
            addSection("WORLD_EFFECTS")
            addSection("MAP_DOWNLOADER / 3D_PREVIEW")
            addMapPreviewCard()
            addSlider("LÃ­mite de escaneo del mapa (mÃ¡x. 150.000)", 10000, 150000, state.mapScanLimit, 5000, function(v)
                state.mapScanLimit = math.min(MAP_SCAN_HARD_LIMIT, v)
                if cachedMapPartCount > state.mapScanLimit then
                    for part in pairs(mapPartCache) do
                        if cachedMapPartCount <= state.mapScanLimit then break end
                        unindexMapPart(part)
                    end
                    mapSnapshotReady = false
                    if mapPreviewStatus and mapPreviewStatus.Parent then
                        mapPreviewStatus.Text = "LÃ­mite actualizado. Pulsa DESCARGAR MAPA / ESCANEAR."
                    end
                end
                updateSnowStatus()
            end)
            addToggle("Noche con Luna", "night", applyNight)
            addToggle("Niebla", "fog", applyFog)
            addSlider("Densidad de niebla", 0, 100, math.floor(state.fogDensity * 100), 1, function(v)
                state.fogDensity = v / 100
                fogAtmosphere.Density = state.fogDensity
                fogAtmosphere.Haze = math.clamp(state.fogDensity * 6, 0, 8)
            end)
            addSlider("Distancia de niebla", 50, 2000, state.fogEnd, 10, function(v)
                state.fogEnd = v
                if state.fog then Lighting.FogEnd = v end
            end)
            addSlider("TamaÃ±o de Luna", 12, 60, state.moonSize, 2, function(v)
                state.moonSize = v
                moon.Size = Vector3.new(v, v, v)
                moonHalo.Size = Vector3.new(v * 1.48, v * 1.48, v * 1.48)
                moonRim.Size = Vector3.new(v * 1.12, v * 1.12, v * 1.12)
                local scale = v / 32
                for _, crater in ipairs(moonFolder:GetChildren()) do
                    if crater.Name:match("^Crater") and crater:IsA("BasePart") then
                        local baseSize = crater:GetAttribute("BaseCraterSize")
                        if typeof(baseSize) == "Vector3" then crater.Size = baseSize * scale end
                    end
                end
            end)
            addSlider("Resplandor de Luna", 0, 100, state.moonGlow, 1, function(v)
                state.moonGlow = v
                moonLight.Brightness = v / 55
                moonHalo.Transparency = 0.97 - (v / 100) * 0.06
                moonRim.Transparency = 0.94 - (v / 100) * 0.08
            end)
            addToggle("ColorCorrection neÃ³n", "colorFX", function(on) colorEffect.Enabled = on end)
            addToggle("Bloom", "bloomFX", function(on) bloomEffect.Enabled = on end)
            addToggle("SunRays", "sunRaysFX", function(on) sunRaysEffect.Enabled = on end)
            addToggle("Depth of Field", "dofFX", function(on) dofEffect.Enabled = on end)
            addSlider("Brillo color correction", -100, 100, state.colorBrightness, 1, function(v)
                state.colorBrightness = v; colorEffect.Brightness = v / 100
            end)
            addSlider("Contraste color correction", -100, 100, state.colorContrast, 1, function(v)
                state.colorContrast = v; colorEffect.Contrast = v / 100
            end)
            addSlider("SaturaciÃ³n color correction", -100, 100, state.colorSaturation, 1, function(v)
                state.colorSaturation = v; colorEffect.Saturation = v / 100
            end)
            addSlider("Intensidad Bloom", 0, 100, state.bloomIntensity, 1, function(v)
                state.bloomIntensity = v; bloomEffect.Intensity = v / 100
            end)
            addSlider("Intensidad SunRays", 0, 100, state.raysIntensity, 1, function(v)
                state.raysIntensity = v; sunRaysEffect.Intensity = v / 100
            end)
            addSlider("Enfoque DOF", 5, 120, state.dofFocus, 1, function(v)
                state.dofFocus = v; dofEffect.FocusDistance = v
            end)
            addAction("RESTAURAR AMBIENTE", function()
                applyNight(false); applyFog(false)
                state.colorFX = false; state.bloomFX = false; state.sunRaysFX = false; state.dofFX = false
                colorEffect.Enabled = false; bloomEffect.Enabled = false; sunRaysEffect.Enabled = false; dofEffect.Enabled = false
            end)
            addSection("SNOW_ACCUMULATION")
            addToggle("Copos cayendo", "snowfall", function(on) setSnowfall(on) end)
            addSlider("Intensidad de nevada", 50, 1500, state.snowRate, 25, function(v) state.snowRate = v; snowEmitter.Rate = v end)
            addSlider("TamaÃ±o de copos", 5, 50, math.floor(state.flakeSize * 100), 1, function(v)
                state.flakeSize = v / 100
                snowEmitter.Size = NumberSequence.new(state.flakeSize)
            end)
            addToggle("AcumulaciÃ³n progresiva", "accumulation", function(on) setAccumulation(on) end)
            addToggle("CongelaciÃ³n gradual del suelo", "frost", function(on)
                frostCC.Enabled = on
                if on then frostProgress = 0 else frostProgress = 0 end
            end)
            addSlider("Velocidad de acumulaciÃ³n", 1, 10, state.accumulationSpeed, 1, function(v) state.accumulationSpeed = v end)
            addSlider("Grosor mÃ¡ximo de nieve Ã0.01", 1, 10, math.floor(state.maxSnowDepth * 100 + 0.5), 1, function(v)
                state.maxSnowDepth = v / 100
                currentSnowDepth = math.min(currentSnowDepth, state.maxSnowDepth)
            end)
            addSlider("Opacidad capa de nieve (%)", 5, 85, math.floor((1 - state.snowOpacity) * 100), 1, function(v)
                state.snowOpacity = 1 - v / 100
                for _, record in ipairs(surfaceRecords) do if record.cover and record.cover.Parent then record.cover.Transparency = state.snowOpacity end end
            end)
            addSlider("Presupuesto de nieve (mÃ¡x. 50.000; alto consumo en mÃ³viles)", 1000, 50000, state.surfaceBudget, 1000, function(v)
                state.surfaceBudget = v
                if state.accumulation then startSnowScan() end
            end)
            surfaceStatusLabel = addInfo("Cobertura", "sin iniciar")
            addAction("REESCANEAR MAPA", function()
                scanLoadedMapForPreview(state.accumulation)
                if state.accumulation then startSnowScan() end
            end)
            addAction("QUITAR NIEVE DEL MAPA", function()
                setAccumulation(false)
                setSnowfall(false)
                state.frost = false; frostCC.Enabled = false
            end)
            addInfo("Escaneo del mapa", "con Script servidor: hasta 150.000 piezas del Workspace completo; sin Ã©l, solo las piezas replicadas al cliente")
            addInfo("Vista 3D", "muestra representativa del inventario; no exporta un archivo .rbxl. La nieve se aplica localmente dentro del presupuesto disponible.")
            addInfo("Mapa completo", "instala SYSTEM_CFG_RGB_v25_Server.lua en ServerScriptService para inventariar geometrÃ­a del servidor sin depender del streaming del cliente.")
        elseif tabName == "HUD" then
            addSection("FPS_MONITOR")
            addToggle("Contador FPS visible", "fpsVisible", function(on) fpsWidget.Visible = on end)
            addToggle("FPS arcoÃ­ris", "fpsRainbow", function() end)
            addToggle("Bloquear posiciÃ³n FPS", "fpsLocked", function() end)
            addSlider("TamaÃ±o texto FPS", 8, 28, state.fpsSize, 1, function(v) state.fpsSize = v; fpsWidget.TextSize = v end)
            addAction("FPS ESQUINA ARRIBA DERECHA", function() setFpsPosition("TR") end)
            addAction("FPS ESQUINA ARRIBA IZQUIERDA", function() setFpsPosition("TL") end)
            addAction("FPS ESQUINA ABAJO DERECHA", function() setFpsPosition("BR") end)
            addAction("FPS ESQUINA ABAJO IZQUIERDA", function() setFpsPosition("BL") end)
            addAction("REINICIAR FPS MÃN / MÃX", function() state.fpsMin = math.huge; state.fpsMax = 0 end)
            addSection("CAPTURE_MODE")
            addToggle("Streamer mode Roblox Capture", "streamerMode", function(on) state.streamerMode = on end)
            addAction("ACTIVAR / SALIR MODO FOTO", function() setPhotoMode(not state.photoMode) end)
            addInfo("Modo foto", "P en PC; en mÃ³vil toca la esquina superior izquierda, debajo de la barra Roblox")
            addInfo("Streamer Mode", "solo puede responder a capturas compatibles de Roblox; iOS/Android no ofrecen ocultar solo esta UI en todas las grabaciones externas")
            addInfo("Nivel grÃ¡fico", "no disponible de forma fiable por API local")
        elseif tabName == "AVATAR" then
            addSection("AVATAR_PREVIEW")
            addToggle("Abrir visor 3D", "avatarOpen", setAvatarOpen)
            addSlider("Zoom del avatar", 4, 28, state.avatarZoom, 1, function(v)
                state.avatarZoom = v; avatarDistance = v; setAvatarCamera()
            end)
            addAction("VENTANA MÃS GRANDE", function()
                state.avatarWidth = math.clamp(state.avatarWidth + 25, 180, 420)
                state.avatarHeight = math.clamp(state.avatarHeight + 25, 220, 480)
                avatarWindow.Size = UDim2.fromOffset(state.avatarWidth, state.avatarHeight)
                positionAvatar(state.avatarCorner)
            end)
            addAction("VENTANA MÃS PEQUEÃA", function()
                state.avatarWidth = math.clamp(state.avatarWidth - 25, 180, 420)
                state.avatarHeight = math.clamp(state.avatarHeight - 25, 220, 480)
                avatarWindow.Size = UDim2.fromOffset(state.avatarWidth, state.avatarHeight)
                positionAvatar(state.avatarCorner)
            end)
            addAction("ESQUINA ARRIBA DERECHA", function() positionAvatar("TR") end)
            addAction("ESQUINA ARRIBA IZQUIERDA", function() positionAvatar("TL") end)
            addAction("ESQUINA ABAJO DERECHA", function() positionAvatar("BR") end)
            addAction("ESQUINA ABAJO IZQUIERDA", function() positionAvatar("BL") end)
            addAction("ACTUALIZAR AVATAR", function() if state.avatarOpen then buildAvatarPreview() end end)
            addInfo("Girar", "arrastra el modelo dentro de la ventana")
        else -- SYS
            addSection("VISUAL_&_SYS")
            addInfo("Estado", "SYSTEM CFG RGB v26 cargado")
            addInfo("Dispositivo", UserInputService.TouchEnabled and "pantalla tÃ¡ctil" or "ratÃ³n / teclado")
            addInfo("Rendimiento", "el FPS mide renderizado local")
            addSection("AUTO_CLICKER / MOBILE")
            addToggle("Auto Clicker A Â· 0.1 s", "autoClickA", function(on) setAutoClicker("A", on) end)
            addToggle("Auto Clicker B Â· 0.1 s", "autoClickB", function(on) setAutoClicker("B", on) end)
            addToggle("Mostrar botones flotantes AC1 / AC2", "autoClickButtonsVisible", function(on) setClickerButtonsVisible(on) end)
            addAction("DETENER LOS DOS AUTO CLICKERS", stopBothClickers)
            addInfo("Compatibilidad", "activa la herramienta equipada mediante Tool:Activate(). No simula toques generales ni pulsa botones de interfaces ajenas; necesita una Tool compatible.")
            addSection("AUDIO_LOCAL")
            addSlider("Volumen de audio (%)", 0, 100, state.audioVolume, 1, function(v)
                setLocalAudioVolume(v)
            end)
            addInfo("Audio", "ajusta el volumen local de sonidos cargados")
            addSection("RESTORE")
            addAction("RESTAURAR CÃMARA", resetCamera)
            addAction("RESTAURAR ILUMINACIÃN", function()
                applyNight(false); applyFog(false)
                Lighting.Brightness = originalLighting.Brightness
                Lighting.Ambient = originalLighting.Ambient
                Lighting.OutdoorAmbient = originalLighting.OutdoorAmbient
                Lighting.ExposureCompensation = originalLighting.ExposureCompensation
                Lighting.FogStart = originalLighting.FogStart
                Lighting.FogEnd = originalLighting.FogEnd
                Lighting.FogColor = originalLighting.FogColor
                nightCC.Enabled = false; fogAtmosphere.Parent = nil; frostCC.Enabled = false
                colorEffect.Enabled = false; bloomEffect.Enabled = false; sunRaysEffect.Enabled = false; dofEffect.Enabled = false
                state.night = false; state.fog = false; state.frost = false
                state.colorFX = false; state.bloomFX = false; state.sunRaysFX = false; state.dofFX = false
            end)
            addAction("RESTAURAR MIRA", function()
                state.crosshair = true; state.rainbow = false; state.crossRotate = true
                state.crossX = 0; state.crossY = -34; state.crossRadius = 25
                state.crossLength = 12; state.crossThickness = 3; state.heartSize = 18; state.crossDotSize = 4; state.crossStyle = "ORBIT"; state.crossColor = "YELLOW"
                state.crossFrozen = false
                if refreshCrosshair3D then refreshCrosshair3D() end
                aimUpButton.Active = true; aimDownButton.Active = true
                aimUpButton.TextColor3 = C.CYAN; aimDownButton.TextColor3 = C.MAGENTA
                mainAim.Visible = true; mainAim.Position = UDim2.new(0.5, 0, 0.5, -34)
            end)
        end
    end

    for i, tab in ipairs(tabSpecs) do
        local b = addButton(tabBar, tab[2], UDim2.new(1 / #tabSpecs, -4, 1, 0))
        b.LayoutOrder = i
        b.TextSize = 9
        b.Name = "Tab_" .. tab[1]
        b.Text = "   " .. tab[2]
        b.TextXAlignment = Enum.TextXAlignment.Left
        addMiniControlPreview(b, tab[1], UDim2.fromOffset(1, 8), 14)
        tabButtons[tab[1]] = b
        b.Activated:Connect(function() populateTab(tab[1]) end)
    end

    local function movePanelToViewport()
        local camera = workspace.CurrentCamera
        if not camera then return end
        local view = camera.ViewportSize
        if view.X < 510 then
            panel.Size = UDim2.fromOffset(math.max(270, math.min(340, view.X - 20)), math.max(300, math.min(440, view.Y - 70)))
            panel.Position = UDim2.new(0, 10, 0, 60)
            launcher.Position = UDim2.new(0, 10, 0, 18)
        else
            panel.Size = UDim2.fromOffset(340, 420)
            panel.Position = UDim2.new(0, 66, 0.5, -210)
            launcher.Position = UDim2.new(0, 12, 0.5, -19)
        end
        if state.avatarOpen then positionAvatar(state.avatarCorner) end
        setFpsPosition(state.fpsCorner)
    end

    -- CaptureService can signal Roblox-managed captures. External OS recorders cannot be detected reliably.
    local captureService = nil
    pcall(function() captureService = game:GetService("CaptureService") end)
    local captureHideInProgress = false
    if captureService then
        pcall(function()
            globalConnect(captureService.CaptureBegan, function()
                if state.streamerMode then
                    captureHideInProgress = true
                    gui.Enabled = false
                end
            end)
        end)
        pcall(function()
            globalConnect(captureService.CaptureEnded, function()
                if captureHideInProgress then
                    captureHideInProgress = false
                    task.delay(0.15, function()
                        if gui and gui.Parent then gui.Enabled = true end
                    end)
                end
            end)
        end)
    end

    globalConnect(UserInputService.InputBegan, function(input, gameProcessed)
        if gameProcessed then return end
        if input.KeyCode == Enum.KeyCode.P then
            setPhotoMode(not state.photoMode)
        elseif state.freeCamera then
            if input.KeyCode == Enum.KeyCode.W then movementFlags.Forward = true
            elseif input.KeyCode == Enum.KeyCode.S then movementFlags.Back = true
            elseif input.KeyCode == Enum.KeyCode.A then movementFlags.Left = true
            elseif input.KeyCode == Enum.KeyCode.D then movementFlags.Right = true
            elseif input.KeyCode == Enum.KeyCode.Q then movementFlags.Down = true
            elseif input.KeyCode == Enum.KeyCode.E then movementFlags.Up = true
            elseif input.KeyCode == Enum.KeyCode.Left then movementFlags.YawLeft = true
            elseif input.KeyCode == Enum.KeyCode.Right then movementFlags.YawRight = true
            elseif input.KeyCode == Enum.KeyCode.Up then movementFlags.PitchUp = true
            elseif input.KeyCode == Enum.KeyCode.Down then movementFlags.PitchDown = true end
        end
    end)
    globalConnect(UserInputService.InputEnded, function(input)
        if input.KeyCode == Enum.KeyCode.W then movementFlags.Forward = false
        elseif input.KeyCode == Enum.KeyCode.S then movementFlags.Back = false
        elseif input.KeyCode == Enum.KeyCode.A then movementFlags.Left = false
        elseif input.KeyCode == Enum.KeyCode.D then movementFlags.Right = false
        elseif input.KeyCode == Enum.KeyCode.Q then movementFlags.Down = false
        elseif input.KeyCode == Enum.KeyCode.E then movementFlags.Up = false
        elseif input.KeyCode == Enum.KeyCode.Left then movementFlags.YawLeft = false
        elseif input.KeyCode == Enum.KeyCode.Right then movementFlags.YawRight = false
        elseif input.KeyCode == Enum.KeyCode.Up then movementFlags.PitchUp = false
        elseif input.KeyCode == Enum.KeyCode.Down then movementFlags.PitchDown = false end
    end)

    setPhotoMode = function(on)
        state.photoMode = on
        panel.Visible = not on and state.menuOpen
        launcher.Visible = not on
        mainAim.Visible = not on and state.crosshair
        rifleAim.Visible = not on and state.rifle
        fpsWidget.Visible = not on and state.fpsVisible
        avatarWindow.Visible = not on and state.avatarOpen
        camPad.Visible = not on and freeCameraState.enabled and UserInputService.TouchEnabled
        aimUpButton.Visible = not on and state.aimButtonsVisible
        aimDownButton.Visible = not on and state.aimButtonsVisible
        clickerAButton.Visible = not on and state.autoClickButtonsVisible
        clickerBButton.Visible = not on and state.autoClickButtonsVisible
        photoHotspot.Visible = on
        if on then
            local cam = workspace.CurrentCamera
            local view = cam and cam.ViewportSize or Vector2.new(900, 600)
            photoHotspot.Position = UDim2.fromOffset(10, math.clamp(96, 8, math.max(8, view.Y - 42)))
        end
    end
    -- One rendering loop for FPS, reticles, free camera, moon, avatar pose and slow snow.
    local rotation = 0
    local hue = 0
    local fpsFrames, fpsElapsed = 0, 0
        local snowAccumElapsed = 0
    local lastSnowScan = 0
    globalConnect(RunService.RenderStepped, function(dt)
        fpsFrames += 1
        fpsElapsed += dt
        if fpsElapsed >= 0.5 then
            local measured = math.floor(fpsFrames / math.max(fpsElapsed, 0.0001) + 0.5)
            state.fpsCurrent = measured
            state.fpsMin = math.min(state.fpsMin or math.huge, measured)
            state.fpsMax = math.max(state.fpsMax or 0, measured)
            fpsWidget.Text = string.format("FPS %d | MIN %s | MAX %s", measured,
                state.fpsMin == math.huge and "--" or tostring(state.fpsMin),
                state.fpsMax == 0 and "--" or tostring(state.fpsMax))
            fpsFrames = 0
            fpsElapsed = 0
        end

        hue = (hue + dt * state.rainbowSpeed) % 1
        rotation = (rotation + dt * state.rotationSpeed) % 360
        local fixedCrossColor = crossColorPalette[state.crossColor] or C.YELLOW
        local phase = ((state.crossStyle == "ORBIT" or state.crossStyle == "RING") and state.crossRotate) and rotation or 0
        local crosshairLayout = getCrosshairLayout(state.crossStyle, state.crossRadius, state.crossLength, phase)
        crossRing.Size = UDim2.fromOffset(state.crossRadius * 2, state.crossRadius * 2)
        crossRing.Visible = crosshairLayout.ring
        crossRingStroke.Color = state.rainbow and Color3.fromHSV(hue, 1, 1) or fixedCrossColor
        crossCenterDot.Visible = crosshairLayout.dot
        crossCenterDot.Size = UDim2.fromOffset(state.crossDotSize, state.crossDotSize)
        crossCenterDot.BackgroundColor3 = state.rainbow and Color3.fromHSV(hue, 1, 1) or fixedCrossColor
        heart.Visible = crosshairLayout.heart
        for i, line in ipairs(aimLines) do
            local segmentInfo = crosshairLayout.segments[i]
            line.Visible = segmentInfo ~= nil
            if segmentInfo then
                line.Position = UDim2.new(0.5, segmentInfo.x, 0.5, segmentInfo.y)
                line.Rotation = segmentInfo.rotation
                line.Size = UDim2.fromOffset(
                    math.max(1, state.crossThickness * segmentInfo.thicknessScale),
                    math.max(2, state.crossLength * segmentInfo.lengthScale)
                )
            end
            line.BackgroundColor3 = state.rainbow and Color3.fromHSV((hue + (i - 1) / #aimLines) % 1, 1, 1) or fixedCrossColor
        end
        heart.Rotation = 0
        heart.TextColor3 = state.rainbow and Color3.fromHSV(hue, 1, 1) or fixedCrossColor
        heart.TextStrokeColor3 = heart.TextColor3
        -- The 3D preview uses the same selected color and updates its RGB tint live.
        if state.rainbow then
            local previewCount = math.max(1, #crosshairPreviewParts)
            for i, part in ipairs(crosshairPreviewParts) do
                if part and part.Parent then part.Color = Color3.fromHSV((hue + (i - 1) / previewCount) % 1, 1, 1) end
            end
        end
        if crosshairPreviewHeartLabel and crosshairPreviewHeartLabel.Parent then
            crosshairPreviewHeartLabel.TextColor3 = state.rainbow and Color3.fromHSV(hue, 1, 1) or fixedCrossColor
        end

        for i, line in ipairs(rifleLines) do
            line.Rotation = (i - 1) * 90
            line.Size = UDim2.fromOffset(state.rifleThickness, state.rifleLength)
            line.BackgroundColor3 = state.rifleRainbow and Color3.fromHSV((hue + (i - 1) / #rifleLines) % 1, 1, 1) or C.YELLOW
        end
        fpsWidget.TextColor3 = state.fpsRainbow and Color3.fromHSV(hue, 1, 1) or C.CYAN

        if freeCameraState.enabled then
            local camera = workspace.CurrentCamera
            if camera then
                local turnRate = 1.6 * dt
                if movementFlags.YawLeft then freeCameraState.yaw += turnRate end
                if movementFlags.YawRight then freeCameraState.yaw -= turnRate end
                if movementFlags.PitchUp then freeCameraState.pitch = math.clamp(freeCameraState.pitch + turnRate, -1.45, 1.45) end
                if movementFlags.PitchDown then freeCameraState.pitch = math.clamp(freeCameraState.pitch - turnRate, -1.45, 1.45) end
                local orientation = CFrame.fromOrientation(freeCameraState.pitch, freeCameraState.yaw, 0)
                local move = Vector3.zero
                if movementFlags.Forward then move += orientation.LookVector end
                if movementFlags.Back then move -= orientation.LookVector end
                if movementFlags.Right then move += orientation.RightVector end
                if movementFlags.Left then move -= orientation.RightVector end
                if movementFlags.Up then move += Vector3.new(0, 1, 0) end
                if movementFlags.Down then move -= Vector3.new(0, 1, 0) end
                camera.CameraType = Enum.CameraType.Scriptable
                if freeCameraState.orbit and state.cameraOrbit then
                    local character = player.Character
                    local root = character and character:FindFirstChild("HumanoidRootPart")
                    if root then
                        local target = root.Position + Vector3.new(0, 1.5, 0)
                        if movementFlags.Forward then state.cameraOrbitDistance = math.max(10, state.cameraOrbitDistance - state.cameraSpeed * dt) end
                        if movementFlags.Back then state.cameraOrbitDistance = math.min(MAX_CAMERA_ORBIT_DISTANCE, state.cameraOrbitDistance + state.cameraSpeed * dt) end
                        if movementFlags.Right then freeCameraState.yaw -= dt * 0.7 end
                        if movementFlags.Left then freeCameraState.yaw += dt * 0.7 end
                        local cameraOffset = orientation.LookVector * state.cameraOrbitDistance
                        freeCameraState.pos = target - cameraOffset
                        camera.CFrame = CFrame.lookAt(freeCameraState.pos, target)
                    else
                        freeCameraState.orbit = false
                        state.cameraOrbit = false
                        camera.CFrame = CFrame.new(freeCameraState.pos) * orientation
                    end
                else
                    if move.Magnitude > 0 then freeCameraState.pos += move.Unit * state.cameraSpeed * dt end
                    camera.CFrame = CFrame.new(freeCameraState.pos) * orientation
                end
                camera.FieldOfView = state.fov
            end
        end

        local camera = workspace.CurrentCamera
        if camera then
            emitterAnchor.Position = camera.CFrame.Position + Vector3.new(0, 48, 0)
            starAnchor.Position = camera.CFrame.Position + camera.CFrame.LookVector * 82 + Vector3.new(0, 12, 0)
            starEmitter.Enabled = state.night and camera.CFrame.LookVector.Y > 0.22 and not state.photoMode
            if state.night and moonFolder.Parent then
                local character = player.Character
                local root = character and character:FindFirstChild("HumanoidRootPart")
                if root then
                    moon.Position = root.Position + moonDirection * 220 + Vector3.new(0, 125, 0)
                    moonHalo.Position = moon.Position + moonDirection * (state.moonSize * 0.08)
                    moonRim.Position = moon.Position + moonDirection * (state.moonSize * 0.035)
                    -- Face the moon back toward the player, so the crater relief is on the visible hemisphere.
                    local moonFrame = CFrame.lookAt(moon.Position, moon.Position - moonDirection, Vector3.new(0, 1, 0))
                    local radius = state.moonSize * 0.5
                    local scale = state.moonSize / 32
                    for _, crater in ipairs(moonFolder:GetChildren()) do
                        if crater.Name:match("^Crater") and crater:IsA("BasePart") then
                            local offset = crater:GetAttribute("LocalMoonOffset")
                            if typeof(offset) == "Vector3" then
                                local x, y = offset.X * scale, offset.Y * scale
                                local r2 = math.min(radius * radius * 0.80, x*x + y*y)
                                local z = -math.sqrt(math.max(0.05, radius * radius - r2)) + 0.06 * scale
                                local localPos = Vector3.new(x, y, z)
                                local surfaceNormal = Vector3.new(x, y, z).Unit
                                local worldPos = moonFrame:PointToWorldSpace(localPos)
                                local worldNormal = moonFrame:VectorToWorldSpace(surfaceNormal)
                                crater.CFrame = CFrame.lookAt(worldPos, worldPos + worldNormal)
                            end
                        end
                    end
                end
            end
        end

        -- Update each frame so the preview does not jump or trail the live avatar.
        updateAvatarPose(dt)

        if state.accumulation then
            snowAccumElapsed += dt
            if snowAccumElapsed >= 1 then
                snowAccumElapsed = 0
                currentSnowDepth = math.min(state.maxSnowDepth, currentSnowDepth + state.accumulationSpeed * 0.00005)
                if state.frost then frostProgress = math.clamp(frostProgress + dt * 0.002, 0, 1) end
                local cold = Color3.fromRGB(207, 223, 250)
                local warm = C.SNOW
                local coverColor = cold:Lerp(warm, frostProgress)
                for _, record in ipairs(surfaceRecords) do
                    if record.cover and record.cover.Parent then
                        local thickness = math.clamp(currentSnowDepth, 0.003, state.maxSnowDepth)
                        record.cover.Size = Vector3.new(record.width, thickness, record.depth)
                        record.cover.CFrame = record.surfaceCFrame * CFrame.new(0, thickness / 2 + 0.00025, 0)
                        record.cover.Color = state.frost and coverColor or C.SNOW
                        record.cover.Reflectance = state.frost and 0.12 * frostProgress or 0
                    end
                end
                updateSnowStatus()
            end
            if os.clock() - lastSnowScan > 3 and not scanRunning then
                lastSnowScan = os.clock()
                if mapBridge and mapBridge:IsA("RemoteFunction") and not serverBridgeFailed then
                    if serverSnowPlanFetched then
                        applyServerTerrainPlansNearPlayer()
                    elseif not serverSnowPlanLoading then
                        loadServerSnowPlan()
                    end
                else
                    startSnowScan()
                end
            end
        end
    end)

    globalConnect(player.CharacterAdded, function(character)
        task.defer(function()
            local humanoid = character:WaitForChild("Humanoid", 5)
            -- Respawn always exits free camera so the view returns to the new character.
            if freeCameraState.enabled then restoreCamera() end
            local camera = workspace.CurrentCamera
            if humanoid and camera then
                camera.CameraType = Enum.CameraType.Custom
                camera.CameraSubject = humanoid
            end
            if state.activeTab == "CAM" and state.menuOpen then
                task.defer(function() populateTab("CAM") end)
            end
            if state.avatarOpen then task.wait(0.25); buildAvatarPreview() end
        end)
    end)

    -- UI state application and interaction.
    launcher.Activated:Connect(function()
        if launcher:GetAttribute("Dragged") then launcher:SetAttribute("Dragged", false); return end
        state.menuOpen = not state.menuOpen
        panel.Visible = state.menuOpen and not state.photoMode
    end)
    closeButton.Activated:Connect(function() state.menuOpen = false; panel.Visible = false end)
    photoHotspot.Activated:Connect(function() setPhotoMode(false) end)

    -- Touch-friendly manual adjustment for panel and launcher, with correct absolute-position clamping.
    -- A resize change repositions FPS and the avatar preview without altering their saved corner settings.
    local function onViewportChanged()
        local camera = workspace.CurrentCamera
        if not camera then return end
        local view = camera.ViewportSize
        if view.X < 520 or view.Y < 520 then
            local panelWidth = math.max(260, math.min(340, view.X - 20))
            local panelHeight = math.max(270, math.min(440, view.Y - 64))
            panel.Size = UDim2.fromOffset(panelWidth, panelHeight)
            if not dragState then panel.Position = UDim2.fromOffset(8, math.min(58, math.max(8, view.Y - panelHeight - 8))) end
            if not dragState then launcher.Position = UDim2.fromOffset(10, 12) end
        else
            panel.Size = UDim2.fromOffset(340, 420)
            if not dragState then panel.Position = UDim2.new(0, 66, 0.5, -210) end
            if not dragState then launcher.Position = UDim2.new(0, 12, 0.5, -19) end
        end
        local safeX, safeY = clampPosition(panel, panel.AbsolutePosition.X, panel.AbsolutePosition.Y)
        panel.Position = UDim2.fromOffset(safeX, safeY)
        setFpsPosition(state.fpsCorner)
        aimUpButton.Position = UDim2.fromOffset(10, 108)
        aimDownButton.Position = UDim2.new(0, 10, 1, -68)
        if state.avatarOpen then positionAvatar(state.avatarCorner) end
    end
    onViewportChanged()
    if workspace.CurrentCamera then
        globalConnect(workspace.CurrentCamera:GetPropertyChangedSignal("ViewportSize"), onViewportChanged)
    end
    globalConnect(workspace:GetPropertyChangedSignal("CurrentCamera"), function()
        task.defer(onViewportChanged)
    end)

    SYSTEM_CFG_STAGE = "populating first menu tab"
    -- Page setup after every component exists, so boot errors don't leave a blank window.
    populateTab("AIM")
    aimUpButton.Visible = state.aimButtonsVisible
    aimDownButton.Visible = state.aimButtonsVisible
    setFpsPosition("TR")
    mainAim.Visible = state.crosshair
    fpsWidget.Visible = state.fpsVisible
    setClickerButtonsVisible(state.autoClickButtonsVisible)
    print("[SYSTEM CFG RGB v28] Menu created successfully.")
    if SYSTEM_CFG_STARTUP_GUI then
        SYSTEM_CFG_STARTUP_GUI:Destroy()
        SYSTEM_CFG_STARTUP_GUI = nil
    end

    -- Cleanup is optional if the LocalScript instance is unavailable; Roblox Studio normally exposes it.
    SYSTEM_CFG_STAGE = "registering optional cleanup"
    local cleanupScript = nil
    pcall(function() cleanupScript = script end)
    if cleanupScript then
        pcall(function()
            cleanupScript.Destroying:Connect(function()
                stopBothClickers()
                if freeCameraState.enabled then restoreCamera() end
                applyNight(false)
                applyFog(false)
                setSnowfall(false)
                setAccumulation(false)
                for _, connection in ipairs(globalConnections) do pcall(function() connection:Disconnect() end) end
                if moonFolder then moonFolder:Destroy() end
                if weatherFolder then weatherFolder:Destroy() end
                if coverFolder then coverFolder:Destroy() end
                if fogAtmosphere then fogAtmosphere:Destroy() end
                if nightCC then nightCC:Destroy() end
                if frostCC then frostCC:Destroy() end
                if colorEffect then colorEffect:Destroy() end
                if bloomEffect then bloomEffect:Destroy() end
                if sunRaysEffect then sunRaysEffect:Destroy() end
                if dofEffect then dofEffect:Destroy() end
                for sound, originalVolume in pairs(soundOriginalVolumes) do
                    if sound and sound.Parent then pcall(function() sound.Volume = originalVolume end) end
                end
            end)
        end)
    end
end

SYSTEM_CFG_STAGE = "calling SYSTEM_CFG_START"
local ok, err = pcall(SYSTEM_CFG_START)
if not ok then
    if SYSTEM_CFG_STARTUP_GUI then
        pcall(function() SYSTEM_CFG_STARTUP_GUI:Destroy() end)
        SYSTEM_CFG_STARTUP_GUI = nil
    end
    warn("[SYSTEM CFG RGB] STARTUP ERROR at " .. tostring(SYSTEM_CFG_STAGE) .. ": " .. tostring(err))
    local PlayersService = game:GetService("Players")
    local localPlayer = PlayersService.LocalPlayer
    local guiParent = localPlayer and localPlayer:FindFirstChildOfClass("PlayerGui")
    if guiParent then
        local partial = guiParent:FindFirstChild("SYSTEM_CFG_RGB")
        if partial then partial:Destroy() end
        local prior = guiParent:FindFirstChild("SYSTEM_CFG_BOOT_ERROR")
        if prior then prior:Destroy() end
        local rescue = Instance.new("ScreenGui")
        rescue.Name = "SYSTEM_CFG_BOOT_ERROR"
        rescue.ResetOnSpawn = false
        rescue.DisplayOrder = 10000
        rescue.Parent = guiParent
        local box = Instance.new("Frame")
        box.Size = UDim2.fromOffset(320, 145)
        box.Position = UDim2.new(0.5, -160, 0.5, -72)
        box.BackgroundColor3 = Color3.fromRGB(8, 10, 18)
        box.BorderSizePixel = 0
        box.Parent = rescue
        Instance.new("UICorner", box).CornerRadius = UDim.new(0, 10)
        local border = Instance.new("UIStroke", box)
        border.Color = Color3.fromRGB(255, 0, 127)
        border.Thickness = 2
        local label = Instance.new("TextLabel")
        label.BackgroundTransparency = 1
        label.Position = UDim2.fromOffset(12, 8)
        label.Size = UDim2.new(1, -24, 1, -16)
        label.Font = Enum.Font.Code
        label.TextSize = 12
        label.TextWrapped = true
        label.TextColor3 = Color3.fromRGB(255, 120, 180)
        label.Text = "SYSTEM CFG STARTUP ERROR\nStage: " .. tostring(SYSTEM_CFG_STAGE) .. "\n\n" .. tostring(err) .. "\n\nEnvÃ­a una captura de este mensaje."
        label.Parent = box
    end
end
