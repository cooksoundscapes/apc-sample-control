-- Receives a file path and outputs a Bang when the file is found.
-- useful for notifying when a a [writesf~] finishes recording to disk.

local file_watcher = pd.Class:new():register("file_watcher")

function file_watcher:initialize(sel, atoms)
    self.inlets = 1
    self.outlets = 1
    self.time = 50
    self.clock = pd.Clock:new():register(self, "watch")
    return true
end 

function file_watcher:in_1_symbol(filepath)
    pd.post(string.format("file_watcher: searching for %s...", filepath))
    self.searchpath = filepath
    self.clock:delay(self.time) 
end

function file_watcher:watch()
    local file = io.open(self.searchpath, "r")
    if file ~= nil then
        io.close(file)
        self:outlet(1, "bang", {})
    else
        self.clock:delay(self.time)
    end
end

function file_watcher:finalize()
    self.clock:destruct()
end


