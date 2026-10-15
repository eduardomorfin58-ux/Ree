-- ================================================================
-- GOY FREE v1.5.7: REMOTE SPY (diagnóstico de solo lectura)
-- Copia con conexión de configuración GitHub (config.json estático, sin ejecutar código remoto).
-- Panel compacto y movible. Registra llamadas del cliente y, opcionalmente,
-- RemoteEvents servidor->cliente bajo ReplicatedStorage.
-- Permite buscar, revisar historial y copiar/exportar reportes para analizarlos.
-- No ejecuta ni bloquea remotos: evita repetir llamadas accidentalmente.
-- Límite: no puede leer scripts privados del servidor ni conectarse directamente
-- a ChatGPT desde el juego. Copia el reporte y pégalo en el chat.
-- Requiere executor con hookmetamethod/getnamecallmethod/newcclosure/checkcaller
-- para registrar las llamadas salientes del cliente.
-- ================================================================
task.spawn(function()
    local Players = game:GetService("Players")
    local UIS = game:GetService("UserInputService")
    local CoreGui = game:GetService("CoreGui")
    local player = Players.LocalPlayer
    if not player then return end

    -- Limpieza si se ejecuta otra vez
    if _G.GOY_RSPY_CLEANUP then pcall(_G.GOY_RSPY_CLEANUP) end

    local HIST_MAX = 30
    local S = {
        capture = true,
        includeOwn = false,
        ignored = setmetatable({}, {__mode = "k"}),
        captureIncoming = false,
        record = function() end,
    }
    _G.GOY_RSPY = S

    ----------------------------------------------------------------
    -- Hook (se instala una sola vez; siempre consulta _G.GOY_RSPY)
    ----------------------------------------------------------------
    local hasHook = type(hookmetamethod) == "function" and type(getnamecallmethod) == "function"
        and type(checkcaller) == "function" and type(newcclosure) == "function"
    if hasHook and not _G.GOY_RSPY_HOOKED then
        local old
        local ok = pcall(function()
            old = hookmetamethod(game, "__namecall", newcclosure(function(self, ...)
                local method = getnamecallmethod()
                if (method == "FireServer" or method == "InvokeServer") and typeof(self) == "Instance" then
                    local cls = self.ClassName
                    if cls == "RemoteEvent" or cls == "RemoteFunction" or cls == "UnreliableRemoteEvent" then
                        local st = _G.GOY_RSPY
                        if st then
                            local own = checkcaller()
                            if st.capture and not st.ignored[self] and (st.includeOwn or not own) then
                                task.defer(st.record, self, method, table.pack(...))
                            end
                        end
                    end
                end
                return old(self, ...)
            end))
        end)
        if ok then _G.GOY_RSPY_HOOKED = true end
    end

    ----------------------------------------------------------------
    -- Serialización de argumentos a código Lua
    ----------------------------------------------------------------
    local function isIdent(s) return type(s) == "string" and s:match("^[%a_][%w_]*$") ~= nil end

    local function instPath(inst)
        if inst == game then return "game" end
        local segs, cur = {}, inst
        while cur and cur ~= game do
            table.insert(segs, 1, cur)
            cur = cur.Parent
        end
        if cur ~= game then
            return ("nil --[[%s (fuera del juego)]]"):format(inst.Name)
        end
        local first = segs[1]
        local out
        local okSvc, svc = pcall(game.GetService, game, first.ClassName)
        if okSvc and svc == first then
            out = ('game:GetService(%q)'):format(first.ClassName)
        else
            out = ('game[%q]'):format(first.Name)
        end
        for i = 2, #segs do
            local name = segs[i].Name
            if i == 2 and okSvc and first.ClassName == "Players" and segs[i] == player then
                out = out .. ".LocalPlayer"
            elseif isIdent(name) then
                out = out .. "." .. name
            else
                out = out .. ("[%q]"):format(name)
            end
        end
        return out
    end

    local function ser(v, depth, seen)
        depth = depth or 0
        local t = typeof(v)
        if t == "string" then return string.format("%q", v)
        elseif t == "number" then
            if v ~= v then return "0/0" end
            if v == math.huge then return "math.huge" end
            if v == -math.huge then return "-math.huge" end
            if v == math.floor(v) and math.abs(v) < 1e15 then return string.format("%d", v) end
            return string.format("%.14g", v)
        elseif t == "boolean" or t == "nil" then return tostring(v)
        elseif t == "Vector3" then return ("Vector3.new(%s, %s, %s)"):format(ser(v.X), ser(v.Y), ser(v.Z))
        elseif t == "Vector2" then return ("Vector2.new(%s, %s)"):format(ser(v.X), ser(v.Y))
        elseif t == "CFrame" then
            local c = {v:GetComponents()}
            for i = 1, #c do c[i] = ser(c[i]) end
            return "CFrame.new(" .. table.concat(c, ", ") .. ")"
        elseif t == "Color3" then return ("Color3.new(%s, %s, %s)"):format(ser(v.R), ser(v.G), ser(v.B))
        elseif t == "UDim2" then return ("UDim2.new(%s, %s, %s, %s)"):format(ser(v.X.Scale), ser(v.X.Offset), ser(v.Y.Scale), ser(v.Y.Offset))
        elseif t == "UDim" then return ("UDim.new(%s, %s)"):format(ser(v.Scale), ser(v.Offset))
        elseif t == "BrickColor" then return ("BrickColor.new(%q)"):format(v.Name)
        elseif t == "EnumItem" then return tostring(v)
        elseif t == "Instance" then return instPath(v)
        elseif t == "table" then
            seen = seen or {}
            if seen[v] then return "nil --[[tabla cíclica]]" end
            if depth >= 5 then return "{} --[[profundidad máxima]]" end
            seen[v] = true
            local parts, n = {}, 0
            for k, val in pairs(v) do
                n = n + 1
                if n > 60 then parts[#parts + 1] = "--[[... más elementos ...]]" break end
                local key
                if isIdent(k) then key = k
                else key = "[" .. ser(k, depth + 1, seen) .. "]" end
                parts[#parts + 1] = key .. " = " .. ser(val, depth + 1, seen)
            end
            seen[v] = nil
            return "{" .. table.concat(parts, ", ") .. "}"
        end
        return ("nil --[[%s no serializable]]"):format(t)
    end

    local function describeCall(e, call)
        local lines = {
            "-- GOY Remote Spy: reporte de diagnóstico (no ejecutable)",
            "Remote: " .. tostring(e.name),
            "Clase: " .. tostring(e.class),
            "Ruta: " .. instPath(e.remote),
            "Dirección/método: " .. tostring(call.method),
            "Argumentos (" .. tostring(call.args.n) .. "):"
        }
        for i = 1, call.args.n do
            lines[#lines + 1] = ("  [%d] %s"):format(i, ser(call.args[i]))
        end
        return table.concat(lines, "\n")
    end

    ----------------------------------------------------------------
    -- Datos del registro
    ----------------------------------------------------------------
    local logs, order = {}, {}
    local selected, viewIdx = nil, 0
    local listDirty = false

    S.record = function(remote, method, args)
        if not remote or not remote.Parent then
            -- remotes ya destruidos: igual los registramos por nombre
        end
        local e = logs[remote]
        if not e then
            e = {remote = remote, name = remote.Name, class = remote.ClassName, hist = {}, count = 0}
            logs[remote] = e
            order[#order + 1] = e
        end
        e.count = e.count + 1
        e.hist[#e.hist + 1] = {method = method, args = args, t = os.clock()}
        if #e.hist > HIST_MAX then table.remove(e.hist, 1) end
        listDirty = true
    end

    ----------------------------------------------------------------
    -- Observador opcional de eventos entrantes (solo ReplicatedStorage)
    ----------------------------------------------------------------
    local ReplicatedStorage = game:GetService("ReplicatedStorage")
    local incomingConnections = {}
    local incomingAddedConnection
    local MAX_INCOMING_REMOTES = 800
    local function disconnectIncoming()
        if incomingAddedConnection then
            pcall(function() incomingAddedConnection:Disconnect() end)
            incomingAddedConnection = nil
        end
        for remote, connection in pairs(incomingConnections) do
            pcall(function() connection:Disconnect() end)
            incomingConnections[remote] = nil
        end
    end
    local function observeIncoming(remote)
        if not S.captureIncoming or not remote:IsA("RemoteEvent") or incomingConnections[remote] then return end
        local count = 0
        for _ in pairs(incomingConnections) do count = count + 1 end
        if count >= MAX_INCOMING_REMOTES then return end
        local ok, connection = pcall(function()
            return remote.OnClientEvent:Connect(function(...)
                local st = _G.GOY_RSPY
                if st and st.captureIncoming and not st.ignored[remote] then
                    st.record(remote, "OnClientEvent (servidor → cliente)", table.pack(...))
                end
            end)
        end)
        if ok and connection then incomingConnections[remote] = connection end
    end
    local function configureIncoming(enabled)
        disconnectIncoming()
        if not enabled then return end
        -- Conectar primero al alta dinámica y después recorrer los que ya existen.
        incomingAddedConnection = ReplicatedStorage.DescendantAdded:Connect(function(inst)
            if inst:IsA("RemoteEvent") then observeIncoming(inst) end
        end)
        local scanned = 0
        for _, inst in ipairs(ReplicatedStorage:GetDescendants()) do
            if inst:IsA("RemoteEvent") then
                observeIncoming(inst)
                scanned = scanned + 1
                if scanned >= MAX_INCOMING_REMOTES then break end
            end
        end
    end

    ----------------------------------------------------------------
    -- Interfaz
    ----------------------------------------------------------------
    local C = {
        panel = Color3.fromRGB(17, 39, 59),
        panel2 = Color3.fromRGB(24, 52, 75),
        button = Color3.fromRGB(40, 72, 96),
        selected = Color3.fromRGB(63, 112, 151),
        border = Color3.fromRGB(106, 176, 220),
        white = Color3.fromRGB(244, 249, 255),
        muted = Color3.fromRGB(191, 211, 228),
        red = Color3.fromRGB(255, 140, 140),
        code = Color3.fromRGB(10, 22, 34),
    }

    local uiParent
    pcall(function() if type(gethui) == "function" then uiParent = gethui() end end)
    uiParent = uiParent or CoreGui
    if not pcall(function() return uiParent.Name end) then uiParent = player:WaitForChild("PlayerGui") end

    local old = uiParent:FindFirstChild("GOY_FREE_REMOTE_SPY")
    if old then pcall(function() old:Destroy() end) end

    local gui = Instance.new("ScreenGui")
    gui.Name = "GOY_FREE_REMOTE_SPY"
    gui.ResetOnSpawn = false
    gui.DisplayOrder = 10050
    gui.ZIndexBehavior = Enum.ZIndexBehavior.Sibling
    gui.Parent = uiParent

    local function round(obj, r, strokeTr)
        local c = Instance.new("UICorner"); c.CornerRadius = UDim.new(0, r); c.Parent = obj
        if strokeTr then
            local s = Instance.new("UIStroke"); s.Color = C.border; s.Transparency = strokeTr; s.Thickness = 1; s.Parent = obj
        end
    end

    local function mkButton(parent, text, callback)
        local b = Instance.new("TextButton")
        b.AutoButtonColor = true
        b.BackgroundColor3 = C.button
        b.BorderSizePixel = 0
        b.Text = text
        b.Font = Enum.Font.GothamSemibold
        b.TextSize = 12
        b.TextColor3 = C.white
        b.Parent = parent
        round(b, 6, 0.55)
        b.Activated:Connect(callback)
        return b
    end

    local function mkLabel(parent, text, size, color)
        local l = Instance.new("TextLabel")
        l.BackgroundTransparency = 1
        l.Text = text
        l.Font = Enum.Font.Gotham
        l.TextSize = size or 12
        l.TextColor3 = color or C.muted
        l.TextXAlignment = Enum.TextXAlignment.Left
        l.TextTruncate = Enum.TextTruncate.AtEnd
        l.Parent = parent
        return l
    end

    local vp = workspace.CurrentCamera and workspace.CurrentCamera.ViewportSize or Vector2.new(390, 844)
    local W = math.clamp(math.floor(vp.X - 20), 280, 430)
    local H = math.clamp(math.floor(vp.Y - 36), 200, 350)
    local listWidth = math.clamp(math.floor(W * 0.32), 88, 140)
    local rightX = listWidth + 12

    local panel = Instance.new("Frame")
    panel.Name = "Panel"
    panel.Size = UDim2.fromOffset(W, H)
    panel.AnchorPoint = Vector2.new(0.5, 0.5)
    panel.Position = UDim2.fromOffset(math.floor(vp.X / 2), math.floor(vp.Y / 2))
    panel.BackgroundColor3 = C.panel
    panel.BackgroundTransparency = 0.04
    panel.BorderSizePixel = 0
    panel.Visible = true
    panel.Active = true
    panel.Parent = gui
    round(panel, 8, 0.2)

    local title = Instance.new("Frame")
    title.Size = UDim2.new(1, 0, 0, 28)
    title.BackgroundColor3 = C.panel2
    title.BorderSizePixel = 0
    title.Parent = panel
    round(title, 8)
    local titleText = mkLabel(title, "GOY Remote Spy  ·  visor de eventos", 12, C.white)
    titleText.Font = Enum.Font.GothamBold
    titleText.Position = UDim2.new(0, 10, 0, 0)
    titleText.Size = UDim2.new(1, -70, 1, 0)

    local toggleObj -- toggle de Orion (se asigna después)
    local opener -- declarado antes de los callbacks que lo usan
    title.Active = true
    local closeBtn = mkButton(title, "×", function()
        panel.Visible = false
        opener.Visible = true
        if toggleObj then pcall(function() toggleObj:Set(false) end) end
    end)
    closeBtn.Size = UDim2.fromOffset(24, 22)
    closeBtn.Position = UDim2.new(1, -28, 0, 3)
    closeBtn.TextSize = 16

    -- Arrastrar
    local dragging, dragStart, startPos
    local conns = {}
    title.InputBegan:Connect(function(i)
        if i.UserInputType == Enum.UserInputType.MouseButton1 or i.UserInputType == Enum.UserInputType.Touch then
            dragging, dragStart, startPos = true, i.Position, panel.Position
            i.Changed:Connect(function()
                if i.UserInputState == Enum.UserInputState.End then dragging = false end
            end)
        end
    end)
    conns[#conns + 1] = UIS.InputChanged:Connect(function(i)
        if dragging and (i.UserInputType == Enum.UserInputType.MouseMovement or i.UserInputType == Enum.UserInputType.Touch) then
            local d = i.Position - dragStart
            local cam = workspace.CurrentCamera
            local size = cam and cam.ViewportSize or vp
            local x = math.clamp(startPos.X.Offset + d.X, W / 2 + 4, math.max(W / 2 + 4, size.X - W / 2 - 4))
            local y = math.clamp(startPos.Y.Offset + d.Y, H / 2 + 4, math.max(H / 2 + 4, size.Y - H / 2 - 4))
            panel.Position = UDim2.fromOffset(x, y)
        end
    end)

    -- Botón flotante para volver a abrir el panel después de cerrarlo.
    opener = mkButton(gui, "R", function()
        panel.Visible = not panel.Visible
        opener.Visible = not panel.Visible
        if panel.Visible then listDirty = true end
    end)
    opener.Name = "RemoteSpyOpener"
    opener.Size = UDim2.fromOffset(42, 42)
    opener.AnchorPoint = Vector2.new(1, 1)
    opener.Position = UDim2.new(1, -14, 1, -90)
    opener.TextSize = 18
    opener.Visible = false

    -- Barra de herramientas
    local toolbar = Instance.new("Frame")
    toolbar.BackgroundTransparency = 1
    toolbar.Position = UDim2.new(0, 6, 0, 32)
    toolbar.Size = UDim2.new(1, -12, 0, 26)
    toolbar.Parent = panel

    local pauseBtn, clearBtn
    local setStatus
    local searchText = ""
    pauseBtn = mkButton(toolbar, "TX ON", function()
        S.capture = not S.capture
        pauseBtn.Text = S.capture and "TX ON" or "TX OFF"
    end)
    pauseBtn.Size = UDim2.fromOffset(60, 26)

    local rowButtons = {}
    local function clearAll()
        for _, b in pairs(rowButtons) do pcall(function() b:Destroy() end) end
        rowButtons = {}
        logs, order = {}, {}
        selected, viewIdx = nil, 0
        listDirty = true
    end
    clearBtn = mkButton(toolbar, "Limpiar", clearAll)
    clearBtn.Size = UDim2.fromOffset(54, 26)
    clearBtn.Position = UDim2.fromOffset(64, 0)

    local rxBtn
    rxBtn = mkButton(toolbar, "RX OFF", function()
        S.captureIncoming = not S.captureIncoming
        rxBtn.Text = S.captureIncoming and "RX ON" or "RX OFF"
        configureIncoming(S.captureIncoming)
        setStatus(S.captureIncoming and "Observando RemoteEvents entrantes en ReplicatedStorage." or "Captura entrante apagada.")
    end)
    rxBtn.Size = UDim2.fromOffset(54, 26)
    rxBtn.Position = UDim2.fromOffset(122, 0)

    local search = Instance.new("TextBox")
    search.PlaceholderText = "Buscar remote..."
    search.Text = ""
    search.ClearTextOnFocus = false
    search.Font = Enum.Font.Gotham
    search.TextSize = 12
    search.TextColor3 = C.white
    search.PlaceholderColor3 = C.muted
    search.BackgroundColor3 = C.code
    search.BorderSizePixel = 0
    search.Position = UDim2.fromOffset(180, 0)
    search.Size = UDim2.new(1, -180, 0, 26)
    search.Parent = toolbar
    round(search, 6, 0.55)
    search:GetPropertyChangedSignal("Text"):Connect(function()
        searchText = string.lower(search.Text)
        listDirty = true
    end)

    -- Lista de remotes
    local list = Instance.new("ScrollingFrame")
    list.Position = UDim2.new(0, 6, 0, 62)
    list.Size = UDim2.new(0, listWidth, 1, -68)
    list.BackgroundColor3 = C.code
    list.BackgroundTransparency = 0.15
    list.BorderSizePixel = 0
    list.ScrollBarThickness = 4
    list.CanvasSize = UDim2.new()
    list.AutomaticCanvasSize = Enum.AutomaticSize.Y
    list.Parent = panel
    round(list, 6, 0.55)
    local listLayout = Instance.new("UIListLayout")
    listLayout.Padding = UDim.new(0, 2)
    listLayout.SortOrder = Enum.SortOrder.LayoutOrder
    listLayout.Parent = list

    -- Área derecha
    local right = Instance.new("Frame")
    right.BackgroundTransparency = 1
    right.Position = UDim2.new(0, rightX, 0, 62)
    right.Size = UDim2.new(1, -(rightX + 6), 1, -68)
    right.Parent = panel

    local info = mkLabel(right, "Selecciona un evento para ver sus datos.", 11, C.muted)
    info.Size = UDim2.new(1, 0, 0, 30)
    info.TextWrapped = true
    info.TextTruncate = Enum.TextTruncate.None
    info.TextYAlignment = Enum.TextYAlignment.Top

    local codeScroll = Instance.new("ScrollingFrame")
    codeScroll.Position = UDim2.new(0, 0, 0, 32)
    codeScroll.Size = UDim2.new(1, 0, 1, -32 - 16 - 58)
    codeScroll.BackgroundColor3 = C.code
    codeScroll.BorderSizePixel = 0
    codeScroll.ScrollBarThickness = 4
    codeScroll.CanvasSize = UDim2.new()
    codeScroll.AutomaticCanvasSize = Enum.AutomaticSize.Y
    codeScroll.Parent = right
    round(codeScroll, 6, 0.55)

    local codeBox = Instance.new("TextBox")
    codeBox.Size = UDim2.new(1, -8, 0, 0)
    codeBox.Position = UDim2.fromOffset(4, 4)
    codeBox.AutomaticSize = Enum.AutomaticSize.Y
    codeBox.BackgroundTransparency = 1
    codeBox.MultiLine = true
    codeBox.ClearTextOnFocus = false
    codeBox.TextEditable = true
    codeBox.TextWrapped = true
    codeBox.TextXAlignment = Enum.TextXAlignment.Left
    codeBox.TextYAlignment = Enum.TextYAlignment.Top
    codeBox.Font = Enum.Font.Code
    codeBox.TextSize = 12
    codeBox.TextColor3 = C.white
    codeBox.Text = ""
    codeBox.PlaceholderText = "Detalles del evento aquí. Copia el reporte y pégalo en ChatGPT para analizarlo."
    codeBox.PlaceholderColor3 = C.muted
    codeBox.Parent = codeScroll

    local status = mkLabel(right, "", 11, C.muted)
    status.Position = UDim2.new(0, 0, 1, -72)
    status.Size = UDim2.new(1, 0, 0, 16)
    setStatus = function(t) status.Text = t end

    local btnRow = Instance.new("Frame")
    btnRow.BackgroundTransparency = 1
    btnRow.Position = UDim2.new(0, 0, 1, -54)
    btnRow.Size = UDim2.new(1, 0, 0, 54)
    btnRow.Parent = right
    local grid = Instance.new("UIGridLayout")
    grid.CellSize = UDim2.new(0.32, -4, 0, 24)
    grid.CellPadding = UDim2.fromOffset(3, 3)
    grid.SortOrder = Enum.SortOrder.LayoutOrder
    grid.Parent = btnRow

    local function currentCall()
        if selected and selected.hist[viewIdx] then return selected, selected.hist[viewIdx] end
    end

    local function showCall()
        local e, call = currentCall()
        if not e then
            codeBox.Text = ""
            info.Text = "Nada seleccionado."
            return
        end
        local ok, report = pcall(describeCall, e, call)
        codeBox.Text = ok and report or ("-- Error al crear el reporte: " .. tostring(report))
        info.Text = ("%s  ·  %s  ·  %d/%d  ·  total %d"):format(
            e.name, call.method, viewIdx, #e.hist, e.count)
    end

    local function refreshList()
        listDirty = false
        for i, e in ipairs(order) do
            local b = rowButtons[e]
            if not b then
                b = Instance.new("TextButton")
                b.Size = UDim2.new(1, -6, 0, 24)
                b.AutoButtonColor = true
                b.BorderSizePixel = 0
                b.Font = Enum.Font.Gotham
                b.TextSize = 11
                b.TextXAlignment = Enum.TextXAlignment.Left
                b.TextTruncate = Enum.TextTruncate.AtEnd
                b.Parent = list
                round(b, 5)
                local pad = Instance.new("UIPadding"); pad.PaddingLeft = UDim.new(0, 6); pad.Parent = b
                b.Activated:Connect(function()
                    selected = e
                    viewIdx = #e.hist
                    listDirty = true
                    showCall()
                    setStatus("")
                end)
                rowButtons[e] = b
            end
            b.LayoutOrder = i
            b.Text = ("%s  ×%d"):format(e.name, e.count)
            b.Visible = searchText == "" or string.find(string.lower(e.name), searchText, 1, true) ~= nil
            b.BackgroundColor3 = (e == selected) and C.selected or C.button
            b.TextColor3 = C.white
        end
    end

    -- Botones de historial y exportación. No hay botón de ejecutar ni bloquear.
    local function copyText(textToCopy, successText)
        if type(setclipboard) == "function" then
            local ok = pcall(setclipboard, textToCopy)
            if ok then setStatus(successText) else setStatus("No se pudo abrir el portapapeles en este executor.") end
        else
            setStatus("Tu executor no tiene setclipboard. Selecciona y copia el texto manualmente.")
        end
    end

    local function exportLog()
        local lines = {"GOY Remote Spy | Reporte de diagnóstico", "No ejecutar: solo datos observados.", ""}
        local total, limit = 0, 120
        for _, e in ipairs(order) do
            for _, call in ipairs(e.hist) do
                if total >= limit then
                    lines[#lines + 1] = "... reporte truncado al límite de " .. limit .. " registros ..."
                    copyText(table.concat(lines, "\n"), "Reporte copiado. Pégalo aquí en ChatGPT para analizarlo.")
                    return
                end
                total = total + 1
                lines[#lines + 1] = describeCall(e, call)
                lines[#lines + 1] = "---"
            end
        end
        if total == 0 then lines[#lines + 1] = "No hay eventos registrados todavía." end
        copyText(table.concat(lines, "\n"), "Reporte copiado. Pégalo aquí en ChatGPT para analizarlo.")
    end

    mkButton(btnRow, "◀ Ant.", function()
        if selected and viewIdx > 1 then viewIdx = viewIdx - 1; showCall() end
    end).LayoutOrder = 1
    mkButton(btnRow, "Sig. ▶", function()
        if selected and viewIdx < #selected.hist then viewIdx = viewIdx + 1; showCall() end
    end).LayoutOrder = 2
    mkButton(btnRow, "Último", function()
        if selected then viewIdx = #selected.hist; showCall() end
    end).LayoutOrder = 3
    mkButton(btnRow, "Copiar", function()
        copyText(codeBox.Text, "Reporte del evento copiado. Pégalo aquí en ChatGPT para analizarlo.")
    end).LayoutOrder = 4
    mkButton(btnRow, "Exportar", exportLog).LayoutOrder = 5
    mkButton(btnRow, "Ignorar", function()
        local e = selected
        if not e then return end
        S.ignored[e.remote] = true
        logs[e.remote] = nil
        for i, v in ipairs(order) do if v == e then table.remove(order, i) break end end
        if rowButtons[e] then rowButtons[e]:Destroy(); rowButtons[e] = nil end
        selected, viewIdx = nil, 0
        showCall()
        setStatus("Se dejaron de registrar llamadas de este remoto.")
        listDirty = true
    end).LayoutOrder = 6

    local alive = true
    task.spawn(function()
        while alive and gui.Parent do
            if panel.Visible and listDirty then pcall(refreshList) end
            task.wait(0.3)
        end
    end)

    _G.GOY_RSPY_CLEANUP = function()
        alive = false
        disconnectIncoming()
        for _, c in ipairs(conns) do pcall(function() c:Disconnect() end) end
        pcall(function() gui:Destroy() end)
        if _G.GOY_RSPY == S then
            S.capture = false
            S.captureIncoming = false
        end
    end

    ----------------------------------------------------------------
    -- Integración opcional con la pestaña Orion del menú GOY
    ----------------------------------------------------------------
    local tab
    pcall(function()
        tab = v646:MakeTab({Name = "Remote Spy", Icon = "rbxassetid://7733964640", PremiumOnly = false})
    end)
    tab = tab or PremiumInfo_Tab
    if tab then
        pcall(function()
            local sec = tab:AddSection({Name = "Remote Spy"})
            toggleObj = sec:AddToggle({
                Name = "Abrir panel de Remote Spy",
                Default = true,
                Callback = function(v)
                    panel.Visible = v
                    opener.Visible = not v
                    if v then listDirty = true end
                end,
            })
            sec:AddToggle({
                Name = "Capturar llamadas salientes",
                Default = true,
                Callback = function(v)
                    S.capture = v
                    pauseBtn.Text = v and "TX ON" or "TX OFF"
                end,
            })
            sec:AddToggle({
                Name = "Incluir llamadas de scripts del executor",
                Default = false,
                Callback = function(v) S.includeOwn = v end,
            })
            sec:AddToggle({
                Name = "Capturar RemoteEvents entrantes (ReplicatedStorage)",
                Default = false,
                Callback = function(v)
                    S.captureIncoming = v
                    rxBtn.Text = v and "RX ON" or "RX OFF"
                    configureIncoming(v)
                end,
            })
            sec:AddButton({Name = "Limpiar registro", Callback = clearAll})
            sec:AddButton({Name = "Restaurar remotes ignorados", Callback = function()
                S.ignored = setmetatable({}, {__mode = "k"})
                listDirty = true
            end})
            sec:AddParagraph("Cómo usarlo", "Abre el panel, realiza una acción y selecciona un evento. RX ON observa RemoteEvents entrantes bajo ReplicatedStorage. Usa Exportar o Copiar y pega el reporte en ChatGPT para recibir una explicación.")
            sec:AddParagraph("Límites", "No lee scripts del servidor. No ejecuta ni bloquea llamadas remotas. La conexión directa a ChatGPT no está disponible desde este script; exporta los registros manualmente.")
        end)
    end
    if not hasHook then
        setStatus("Aviso: este executor no ofrece las funciones hook requeridas; la interfaz abre, pero no capturará llamadas salientes.")
    end

    ----------------------------------------------------------------
    -- [GitHub] Configuración remota estática (solo lectura)
    -- Lee GOY_RemoteSpr/config.json una vez al abrir el panel.
    -- Solo usa valores informativos (nombre visible y versión).
    -- Nunca ejecuta contenido descargado. Si falla, el menú sigue igual.
    ----------------------------------------------------------------
    local GITHUB_CONFIG_URL = "https://raw.githubusercontent.com/eduardomorfin58-ux/Ree/main/GOY_RemoteSpr/config.json"
    local GITHUB_TIMEOUT = 15      -- segundos antes de avisar "sin respuesta"
    local GITHUB_CACHE_TTL = 120   -- reutiliza la última lectura si se re-ejecuta el script
    local GITHUB_MAX_BYTES = 8192  -- config.json es pequeño; algo mayor se rechaza

    local function githubHttpGet(url)
        -- 1) Funciones tipo request del executor (devuelven StatusCode)
        local reqFn
        pcall(function()
            reqFn = (type(request) == "function" and request)
                or (type(http_request) == "function" and http_request)
                or (type(syn) == "table" and type(syn.request) == "function" and syn.request)
                or (type(http) == "table" and type(http.request) == "function" and http.request)
                or (type(fluxus) == "table" and type(fluxus.request) == "function" and fluxus.request)
        end)
        if reqFn then
            local ok, res = pcall(reqFn, {Url = url, Method = "GET"})
            if ok and type(res) == "table" then
                local code = tonumber(res.StatusCode)
                if code and code ~= 200 then
                    return nil, ("GitHub respondió HTTP %d"):format(code)
                end
                if type(res.Body) == "string" then return res.Body end
            end
        end
        -- 2) game:HttpGet como alternativa
        local hasHttpGet = false
        pcall(function() hasHttpGet = type(game.HttpGet) == "function" end)
        if not reqFn and not hasHttpGet then
            return nil, "este executor no permite solicitudes HTTP"
        end
        if hasHttpGet then
            local okGet, body = pcall(function() return game:HttpGet(url) end)
            if okGet and type(body) == "string" then return body end
        end
        return nil, "sin conexión o descarga fallida"
    end

    local function githubClean(s, maxLen)
        if type(s) ~= "string" then return nil end
        s = s:gsub("[%c<>]", ""):match("^%s*(.-)%s*$")
        if s == "" or #s > maxLen then return nil end
        return s
    end

    -- Convierte el JSON en tabla Lua y valida los campos esenciales.
    local function githubParseConfig(body)
        if type(body) ~= "string" or body == "" then return nil, "respuesta vacía" end
        if #body > GITHUB_MAX_BYTES then return nil, "config.json demasiado grande" end
        local okDecode, data = pcall(function()
            return game:GetService("HttpService"):JSONDecode(body)
        end)
        if not okDecode or type(data) ~= "table" then return nil, "config.json mal formado (JSON inválido)" end
        local cfg = {
            project = githubClean(data.project, 64),
            displayName = githubClean(data.displayName, 48),
            version = githubClean(data.version, 20),
        }
        if cfg.project ~= "GOY_RemoteSpr" then return nil, "campo 'project' ausente o distinto" end
        if not cfg.displayName then return nil, "campo 'displayName' ausente o inválido" end
        if not (cfg.version and cfg.version:match("^%d+%.%d+%.%d+$")) then
            return nil, "campo 'version' inválido (se espera X.Y.Z)"
        end
        if data.configVersion ~= 1 then return nil, "configVersion no compatible" end
        if data.sourceMode ~= "static-config-only" then return nil, "sourceMode no permitido" end
        if data.allowRemoteCodeExecution == true or data.allowRemoteReplay == true
            or data.allowRemoteBlocking == true then
            return nil, "configuración rechazada: pide permisos que este script no admite"
        end
        return cfg
    end

    local function githubApply(cfg, err)
        if not (alive and gui.Parent) then return end -- panel cerrado o re-ejecutado
        local hookNote = hasHook and "" or "  ·  sin hooks: no captura salientes"
        if cfg then
            titleText.Text = cfg.displayName .. "  ·  visor de eventos"
            setStatus(("GitHub conectado | v%s%s"):format(cfg.version, hookNote))
        else
            setStatus(("GitHub no disponible: %s. El menú sigue funcionando.%s"):format(tostring(err), hookNote))
        end
    end

    local cached = _G.GOY_RSPY_REMOTE_CONFIG
    if type(cached) == "table" and type(cached.t) == "number" and os.clock() - cached.t < GITHUB_CACHE_TTL then
        githubApply(cached.cfg, cached.err) -- evita otra descarga al re-ejecutar en poco tiempo
    else
        local finished = false
        task.delay(GITHUB_TIMEOUT, function()
            if not finished then githubApply(nil, "GitHub no responde (tiempo agotado)") end
        end)
        task.spawn(function()
            local okAll, cfg, err = pcall(function()
                local body, httpErr = githubHttpGet(GITHUB_CONFIG_URL)
                if not body then return nil, httpErr end
                return githubParseConfig(body)
            end)
            if not okAll then cfg, err = nil, "error inesperado al leer la configuración" end
            finished = true
            _G.GOY_RSPY_REMOTE_CONFIG = {t = os.clock(), cfg = cfg, err = err}
            githubApply(cfg, err)
        end)
    end


end)
