const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const { exec, spawn } = require('child_process');
const path = require('path');
const cors = require('cors');
const fs = require('fs');
const Tail = require('tail').Tail;
const sqlite3 = require('sqlite3').verbose();
const os = require('os'); 

const app = express();
const server = http.createServer(app);
const io = new Server(server, { cors: { origin: "*" } });

app.use(cors());
app.use(express.static(path.join(__dirname, 'public')));

// ==========================================
// 🔐 USUARIOS Y CONTRASEÑAS DEL PANEL WEB
// ==========================================
const ADMIN_USERS = {
    "OhmyWeed": "toito123",
    "Admin2": "otraContrasena123",
    "Widah": "ramiro123",
};

// ==========================================
// 📂 DIRECTORIOS DEL SERVIDOR
// ==========================================
const SERVER_EXE_PATH = "D:\\HB Server\\Helbreath-Heldenian-Project-Development\\Binaries\\Server\\Server.exe"; 
const SERVER_CWD = "D:\\HB Server\\Helbreath-Heldenian-Project-Development\\Binaries\\Server";
const LOG_FOLDER = "D:\\HB Server\\Helbreath-Heldenian-Project-Development\\Binaries\\Server\\gamelogs";
const ACCOUNTS_FOLDER = path.join(SERVER_CWD, "accounts");
const CONFIG_FILE_PATH = path.join(SERVER_CWD, "server_config.json");
const GAMEDATA_DB_PATH = path.join(SERVER_CWD, "gamedata.db");
const GUILDS_DB_PATH = path.join(SERVER_CWD, "guilds.db");
const MAPINFO_DB_PATH = path.join(SERVER_CWD, "MapInfo.db");
const DASHBOARD_LOGS_PATH = path.join(__dirname, "dashboard_logs.json");

// [NUEVO] RUTAS PARA EL AUTO-UPDATER
const UPDATER_DIR = "D:\\HB Server\\Helbreath-Heldenian-Project-Development\\tools\\UpdaterServer";
const UPDATER_LOG = path.join(UPDATER_DIR, 'updater_web.log');
const MANIFEST_LOG = path.join(UPDATER_DIR, 'manifest_web.log');

let updaterProcess = null;
const activeTails = new Map();
const activeWatchers = {};
let knownCategories = [];
const logCache = []; 
const MAX_CACHE = 500;
const onlineCharacters = new Map();
let pendingAccountLogin = ""; 

function broadcastToAdmins(event, data) {
    io.to('admins').emit(event, data);
}

function pushLogToWeb(category, text, isRed = false) {
    if (!text || text.trim() === '') return;
    const cleanText = text.trim();
    const logData = { category: category.toUpperCase(), text: cleanText, isRed: isRed };
    
    logCache.push(logData);
    if (logCache.length > MAX_CACHE) logCache.shift();

    broadcastToAdmins('console-log', logData);
}

function getDbPath(dbId) {
    if (dbId === 'gamedata') return GAMEDATA_DB_PATH;
    if (dbId === 'guilds') return GUILDS_DB_PATH;
    if (dbId === 'mapinfo') return MAPINFO_DB_PATH;
    if (dbId && dbId.startsWith('account:')) {
        const accName = dbId.split(':')[1];
        return path.join(ACCOUNTS_FOLDER, `${accName}.db`);
    }
    return null;
}

function scanLogsForOnlinePlayers() {
    if (!fs.existsSync(LOG_FOLDER)) return;
    onlineCharacters.clear();

    const files = fs.readdirSync(LOG_FOLDER);
    const txtFiles = files.filter(f => f.endsWith('.txt') || f.endsWith('.log'));
    if (txtFiles.length === 0) return;

    let allEvents = [];
    txtFiles.forEach(file => {
        try {
            const filePath = path.join(LOG_FOLDER, file);
            const content = fs.readFileSync(filePath, 'utf8');
            const lines = content.split(/\r?\n/);
            lines.forEach(line => {
                if (line.trim()) {
                    allEvents.push({ text: line });
                }
            });
        } catch (e) {}
    });

    let lastStartupGlobalIndex = -1;
    allEvents.forEach((ev, idx) => {
        if (ev.text.includes('Game server activated') || ev.text.includes('Started:')) {
            lastStartupGlobalIndex = idx;
        }
    });

    const sessionEvents = lastStartupGlobalIndex !== -1 ? allEvents.slice(lastStartupGlobalIndex) : allEvents;
    let pendingAcc = "";
    
    const currentOnlineMap = new Map();

    sessionEvents.forEach(ev => {
        const data = ev.text;

        const accMatch = data.match(/Account Request Login:\s+(.+)/i);
        if (accMatch) pendingAcc = accMatch[1].trim().toLowerCase();

        const loginMatch = data.match(/Request enter Game:\s+([a-zA-Z0-9_]+)/i);
        if (loginMatch) {
            const charName = loginMatch[1].trim();
            const accountName = pendingAcc || charName.toLowerCase();
            const charKey = charName.toLowerCase();

            currentOnlineMap.set(charKey, { name: charName, account: accountName });
        }

        const logoutMatch = data.match(/(?:disconnect|lost|error|timeout|close).*?(?:char|character)=([a-zA-Z0-9_]+)/i);
        if (logoutMatch) {
            const charKey = logoutMatch[1].trim().toLowerCase();
            currentOnlineMap.delete(charKey);
        }
    });

    currentOnlineMap.forEach((info, charKey) => {
        let pData = { name: info.name, level: '?', faction: 'Neutral', class: '?', guild: '-', map: '?', account: info.account };
        onlineCharacters.set(charKey, pData);

        if (info.account) {
            const dbPath = path.join(ACCOUNTS_FOLDER, `${info.account}.db`);
            if (fs.existsSync(dbPath)) {
                try {
                    const db = new sqlite3.Database(dbPath, sqlite3.OPEN_READONLY);
                    db.get("SELECT level, map_name, str, mag, location FROM characters WHERE character_name COLLATE NOCASE = ?", [info.name], (err, row) => {
                        if (!err && row && onlineCharacters.has(charKey)) {
                            pData.level = row.level;
                            pData.map = row.map_name;
                            if (row.location) pData.faction = row.location.charAt(0).toUpperCase() + row.location.slice(1).toLowerCase();
                            pData.class = (row.str > row.mag) ? 'Warrior' : (row.mag > row.str ? 'Mage' : 'Hybrid');
                            onlineCharacters.set(charKey, pData);
                            broadcastToAdmins('online-players-data', Array.from(onlineCharacters.values()));
                        }
                        db.close();
                    });
                } catch (e) {}
            }
        }
    });

    broadcastToAdmins('online-players-data', Array.from(onlineCharacters.values()));
}

function startReadingAllLogs() {
    if (!fs.existsSync(LOG_FOLDER)) return;

    scanLogsForOnlinePlayers();

    const files = fs.readdirSync(LOG_FOLDER);
    const txtFiles = files.filter(f => f.endsWith('.txt') || f.endsWith('.log'));

    knownCategories = txtFiles.map(f => f.replace('.txt', '').replace('.log', '').toUpperCase());
    if (!knownCategories.includes('SISTEMA')) knownCategories.push('SISTEMA');
    if (!knownCategories.includes('COMMANDS')) knownCategories.push('COMMANDS');
    
    broadcastToAdmins('init-categories', knownCategories);

    txtFiles.forEach(file => {
        const filePath = path.join(LOG_FOLDER, file);
        const logName = file.replace('.txt', '').replace('.log', '').toUpperCase();
        try {
            const content = fs.readFileSync(filePath, 'utf8');
            const lines = content.split(/\r?\n/);
            
            let lastActivationIndex = -1;
            for (let i = 0; i < lines.length; i++) {
                if (lines[i].includes('Started:') || lines[i].includes('Game server activated')) {
                    lastActivationIndex = i;
                }
            }

            let currentSessionLines;
            if (lastActivationIndex !== -1) {
                currentSessionLines = lines.slice(lastActivationIndex);
            } else {
                currentSessionLines = lines.slice(-150);
            }

            currentSessionLines.forEach(line => {
                if (line && line.trim() !== '') {
                    logCache.push({ category: logName, text: line.trim(), isRed: false });
                }
            });
        } catch (e) {}
    });

    if (logCache.length > MAX_CACHE) {
        logCache.splice(0, logCache.length - MAX_CACHE);
    }

    txtFiles.forEach(file => {
        if (activeTails.has(file)) return;
        const filePath = path.join(LOG_FOLDER, file);
        const logName = file.replace('.txt', '').replace('.log', '').toUpperCase();

        try {
            const tail = new Tail(filePath, { 
                useWatchFile: true, fsWatchOptions: { interval: 250 }, flushAtEOF: true                  
            });
            
            tail.on("line", function(data) {
                pushLogToWeb(logName, data, false);

                const accMatch = data.match(/Account Request Login:\s+(.+)/i);
                if (accMatch) pendingAccountLogin = accMatch[1].trim().toLowerCase();

                const loginMatch = data.match(/Request enter Game:\s+([a-zA-Z0-9_]+)/i);
                if (loginMatch) {
                    const charName = loginMatch[1].trim();
                    const charKey = charName.toLowerCase();
                    const accountName = pendingAccountLogin || charName.toLowerCase();
                    
                    let pData = { name: charName, level: '?', faction: 'Neutral', class: '?', guild: '-', map: '?', account: accountName };
                    onlineCharacters.set(charKey, pData);
                    broadcastToAdmins('online-players-data', Array.from(onlineCharacters.values()));

                    if (accountName) {
                        const dbPath = path.join(ACCOUNTS_FOLDER, `${accountName}.db`);
                        if (fs.existsSync(dbPath)) {
                            const db = new sqlite3.Database(dbPath, sqlite3.OPEN_READONLY);
                            db.get("SELECT level, map_name, str, mag, location FROM characters WHERE character_name COLLATE NOCASE = ?", [charName], (err, row) => {
                                if (!err && row && onlineCharacters.has(charKey)) {
                                    pData.level = row.level;
                                    pData.map = row.map_name;
                                    if (row.location) pData.faction = row.location.charAt(0).toUpperCase() + row.location.slice(1).toLowerCase();
                                    pData.class = (row.str > row.mag) ? 'Warrior' : (row.mag > row.str ? 'Mage' : 'Hybrid');
                                    
                                    onlineCharacters.set(charKey, pData);
                                    broadcastToAdmins('online-players-data', Array.from(onlineCharacters.values()));
                                }
                                db.close();
                            });
                        }
                    }
                }

                const logoutMatch = data.match(/(?:disconnect|lost|error|timeout|close).*?(?:char|character)=([a-zA-Z0-9_]+)/i);
                if (logoutMatch) {
                    const charKey = logoutMatch[1].trim().toLowerCase();
                    if (onlineCharacters.has(charKey)) {
                        onlineCharacters.delete(charKey);
                        broadcastToAdmins('online-players-data', Array.from(onlineCharacters.values()));
                    }
                }
            });
            activeTails.set(file, tail);
        } catch(e) { console.log(`No se pudo leer ${file}: ${e.message}`); }
    });
}
startReadingAllLogs();

// 🚀 LECTOR DE CHUNKS EN TIEMPO REAL (Para atrapar las barras de carga \r de Python)
function startChunkWatcher(filePath, socketEvent) {
    if (!fs.existsSync(filePath)) fs.writeFileSync(filePath, '');
    let pos = fs.statSync(filePath).size;
    
    if (activeWatchers[filePath]) {
        fs.unwatchFile(filePath);
    }

    fs.watchFile(filePath, { interval: 150 }, (curr, prev) => {
        if (curr.size > pos) {
            const stream = fs.createReadStream(filePath, { start: pos, end: curr.size - 1 });
            stream.on('data', (chunk) => {
                broadcastToAdmins(socketEvent, chunk.toString());
            });
            pos = curr.size;
        } else if (curr.size < pos) {
            pos = curr.size; // El archivo fue vaciado
        }
    });
    activeWatchers[filePath] = true;
}

// 🐍 GENERADOR DE WRAPPERS DE PYTHON EN TIEMPO REAL
function createPythonWrapper(moduleName, logFile) {
    const escapedLogFile = logFile.replace(/\\/g, '\\\\');
    return `
import sys
class Tee:
    def __init__(self, log_path):
        self.terminal = sys.stdout
        self.log = open(log_path, "a", encoding="utf-8")
    def write(self, message):
        self.terminal.write(message)
        self.log.write(message)
        self.log.flush()
    def flush(self):
        self.terminal.flush()
        self.log.flush()
sys.stdout = Tee('${escapedLogFile}')
sys.stderr = sys.stdout
import ${moduleName}
try:
    ${moduleName}.main()
except Exception as e:
    print(e)
except KeyboardInterrupt:
    pass
`;
}


io.on('connection', (socket) => {
    const clientIp = socket.handshake.headers['x-forwarded-for'] || socket.handshake.address;
    console.log(`[WEB] Alguien ha abierto la página web. IP detectada: ${clientIp}`);

    socket.on('login', (data) => {
        let username = "";
        let pass = "";

        if (typeof data === 'object' && data !== null) {
            username = data.username || "";
            pass = data.password || "";
        } else {
            pass = data;
            username = "Desconocido";
        }

        if (ADMIN_USERS[username] && ADMIN_USERS[username] === pass) {
            socket.join('admins'); 
            socket.isAdmin = true;
            console.log(`[SEGURIDAD] ¡Login EXITOSO! El usuario "${username}" se ha conectado desde la IP: ${clientIp}`);
            socket.emit('login-success');

            scanLogsForOnlinePlayers();

            if (knownCategories.length > 0) socket.emit('init-categories', knownCategories);
            
            socket.emit('console-log', { category: 'SISTEMA', text: `Conectado al panel de control como [${username}]...` });

            // Enviar estado inicial del updater
            socket.emit('updater-status', updaterProcess !== null);

            exec('tasklist /FI "IMAGENAME eq Server.exe" /FO CSV', (err, stdout) => {
                const isRunning = stdout ? stdout.toLowerCase().includes('server.exe') : false;
                socket.emit('server-state', { isRunning });

                if (isRunning) {
                    logCache.forEach(log => socket.emit('console-log', log));
                } else {
                    socket.emit('console-log', { category: 'SISTEMA', text: '⚠️ El servidor se encuentra APAGADO.' });
                }

                socket.emit('online-players-data', Array.from(onlineCharacters.values()));
            });
        } else {
            console.log(`[SEGURIDAD] Intento de login FALLIDO (Usuario intentado: "${username}") desde la IP: ${clientIp}`);
            socket.emit('login-failed');
        }
    });

    socket.on('start-server', () => {
        if (!socket.isAdmin) return; 
        onlineCharacters.clear(); 
        broadcastToAdmins('online-players-data', []);
        pushLogToWeb('SISTEMA', '>>> Abriendo el servidor...', false);
        
        exec(`start "" "${SERVER_EXE_PATH}"`, { cwd: SERVER_CWD });
        setTimeout(() => { startReadingAllLogs(); }, 3000);
    });

    socket.on('kick-player', ({ charName }) => {
        if (!socket.isAdmin) return;
        const safeCmd = `kick-player ${charName}`.replace(/'/g, "''");
        pushLogToWeb('COMMANDS', `[WEB-ADMIN] Expulsando al jugador: ${charName}`, true);

        const psCommand = `powershell -command "$p = Get-Process -Name 'Server' -ErrorAction SilentlyContinue | Select-Object -First 1; if ($p) { $ws = New-Object -ComObject WScript.Shell; $ws.AppActivate($p.Id); Start-Sleep -Milliseconds 150; $ws.SendKeys('${safeCmd}'); Start-Sleep -Milliseconds 50; $ws.SendKeys('{ENTER}'); }"`;
        exec(psCommand);
    });

    socket.on('ban-player', ({ charName, duration, reason }) => {
        if (!socket.isAdmin) return;
        if (!charName) return;

        const safeDuration = duration || 'perma';
        const safeReason = reason || 'Sin motivo especificado';
        const safeCmd = `ban-player ${charName} ${safeDuration} ${safeReason}`.replace(/'/g, "''");
        
        pushLogToWeb('COMMANDS', `[WEB-ADMIN] Baneando personaje [${charName}] (${safeDuration}). Motivo: ${safeReason}`, true);

        const psCommand = `powershell -command "$p = Get-Process -Name 'Server' -ErrorAction SilentlyContinue | Select-Object -First 1; if ($p) { $ws = New-Object -ComObject WScript.Shell; $ws.AppActivate($p.Id); Start-Sleep -Milliseconds 150; $ws.SendKeys('${safeCmd}'); Start-Sleep -Milliseconds 50; $ws.SendKeys('{ENTER}'); }"`;
        exec(psCommand);
    });

    socket.on('unban-player', ({ charName }) => {
        if (!socket.isAdmin) return;
        if (!charName) return;

        const safeCmd = `unban-player ${charName}`.replace(/'/g, "''");
        pushLogToWeb('COMMANDS', `[WEB-ADMIN] Desbaneando personaje/cuenta: ${charName}`, true);

        const psCommand = `powershell -command "$p = Get-Process -Name 'Server' -ErrorAction SilentlyContinue | Select-Object -First 1; if ($p) { $ws = New-Object -ComObject WScript.Shell; $ws.AppActivate($p.Id); Start-Sleep -Milliseconds 150; $ws.SendKeys('${safeCmd}'); Start-Sleep -Milliseconds 50; $ws.SendKeys('{ENTER}'); }"`;
        exec(psCommand);
    });

    socket.on('get-banned-players', () => {
        if (!socket.isAdmin) return;
        const bannedList = [];
        if (!fs.existsSync(ACCOUNTS_FOLDER)) {
            socket.emit('banned-players-data', []);
            return;
        }
        const files = fs.readdirSync(ACCOUNTS_FOLDER).filter(f => f.endsWith('.db'));
        let processed = 0;
        if (files.length === 0) {
            socket.emit('banned-players-data', []);
            return;
        }
        files.forEach(file => {
            const accountName = file.replace('.db', '');
            const dbPath = path.join(ACCOUNTS_FOLDER, file);
            const db = new sqlite3.Database(dbPath, sqlite3.OPEN_READONLY);
            db.get("SELECT name FROM sqlite_master WHERE type='table' AND name='account_bans'", (err, row) => {
                if (!err && row) {
                    db.get("SELECT ban_until, reason FROM account_bans", (err2, banRow) => {
                        if (!err2 && banRow) {
                            const now = Math.floor(Date.now() / 1000);
                            if (banRow.ban_until > now) {
                                const isPermanent = banRow.ban_until >= 9999999900;
                                const banDate = isPermanent ? 'Permanente' : new Date(banRow.ban_until * 1000).toLocaleString();
                                bannedList.push({
                                    account: accountName,
                                    banUntil: banDate,
                                    reason: banRow.reason || 'Sin motivo especificado'
                                });
                            }
                        }
                        db.close();
                        checkDone();
                    });
                } else {
                    db.close();
                    checkDone();
                }
            });
        });

        function checkDone() {
            processed++;
            if (processed === files.length) {
                socket.emit('banned-players-data', bannedList);
            }
        }
    });

    socket.on('get-items-list', () => {
        if (!socket.isAdmin) return;
        if (!fs.existsSync(GAMEDATA_DB_PATH)) {
            socket.emit('items-list-data', []);
            return;
        }
        try {
            const db = new sqlite3.Database(GAMEDATA_DB_PATH, sqlite3.OPEN_READONLY);
            db.all("SELECT * FROM items", [], (err, rows) => {
                db.close();
                if (err || !rows) {
                    socket.emit('items-list-data', []);
                } else {
                    socket.emit('items-list-data', rows);
                }
            });
        } catch (e) {
            socket.emit('items-list-data', []);
        }
    });

    socket.on('give-item-to-player', ({ charName, itemId, amount }) => {
        if (!socket.isAdmin || !charName || !itemId) return;
        const safeAmount = amount || 1;
        const safeCmd = `giveitem ${charName} ${itemId} ${safeAmount}`.replace(/'/g, "''");
        
        pushLogToWeb('COMMANDS', `[WEB-ADMIN] Entregando [ID: ${itemId}, Cant: ${safeAmount}] al personaje: ${charName}`, true);

        const psCommand = `powershell -command "$p = Get-Process -Name 'Server' -ErrorAction SilentlyContinue | Select-Object -First 1; if ($p) { $ws = New-Object -ComObject WScript.Shell; $ws.AppActivate($p.Id); Start-Sleep -Milliseconds 150; $ws.SendKeys('${safeCmd}'); Start-Sleep -Milliseconds 50; $ws.SendKeys('{ENTER}'); }"`;
        exec(psCommand);
    });

    const commandResponses = {
        'help': 
`  help         List available commands
  "showchat"     Open live chat viewer in a new terminal
  "broadcast"    Send a broadcast message to all players
  "giveitem"     Give an item to a player
  "reload"       Reload config tables from database or JSON
  "setadmin"     Set a player as admin
  "setcmdlevel"  Set required admin level for a chat command
  "saveall"      Save all connected players' data
  "shutdown"     Save all players and gracefully shut down the server`,
        'showchat': `Chat viewer opened in new terminal.`,
        'broadcast': `Usage: broadcast message`,
        'giveitem': `Usage: giveitem <playername> <item_id> <amount>`,
        'kick-player': `Usage: kick-player <charname>`,
        'reload': `Usage: reload items | magic | skills | npcs | shops | config | formulas | colors | attributes | all`,
        'setadmin': `Usage: setadmin <charname> [resetip]`,
        'setcmdlevel': `Usage: setcmdlevel <command> <level>`,
        'saveall': `>>> [WEB-ADMIN] Comando de guardado global procesado.`,
        'shutdown': `>>> [WEB-ADMIN] Comando de apagado procesado.`
    };

    socket.on('send-command', (cmdText) => {
        if (!socket.isAdmin || !cmdText || cmdText.trim() === '') return;
        
        const cleanCmd = cmdText.trim();
        const safeCmd = cleanCmd.replace(/'/g, "''");
        
        pushLogToWeb('COMMANDS', `[WEB-ADMIN] Ejecutando: ${cleanCmd}`, false);

        const psCommand = `powershell -command "$p = Get-Process -Name 'Server' -ErrorAction SilentlyContinue | Select-Object -First 1; if ($p) { $ws = New-Object -ComObject WScript.Shell; $ws.AppActivate($p.Id); Start-Sleep -Milliseconds 150; $ws.SendKeys('${safeCmd}'); Start-Sleep -Milliseconds 50; $ws.SendKeys('{ENTER}'); }"`;
        exec(psCommand);

        const baseCommand = cleanCmd.split(' ')[0].toLowerCase();
        if (commandResponses[baseCommand]) {
            setTimeout(() => {
                commandResponses[baseCommand].split('\n').forEach(line => {
                    pushLogToWeb('COMMANDS', line, true);
                });
            }, 200);
        }
    });

    socket.on('get-online-players', () => {
        if (!socket.isAdmin) return;
        scanLogsForOnlinePlayers();
        setTimeout(() => {
            socket.emit('online-players-data', Array.from(onlineCharacters.values()));
        }, 150);
    });

    socket.on('get-config', () => {
        if (!socket.isAdmin) return;
        if (fs.existsSync(CONFIG_FILE_PATH)) {
            try {
                socket.emit('config-data', JSON.parse(fs.readFileSync(CONFIG_FILE_PATH, 'utf8')));
            } catch (e) {
                pushLogToWeb('SISTEMA', '[ERROR] Fallo al leer config', false);
            }
        }
    });

    socket.on('save-config', (newConfig) => {
        if (!socket.isAdmin) return;
        try {
            fs.writeFileSync(CONFIG_FILE_PATH, JSON.stringify(newConfig, null, 4), 'utf8');
            pushLogToWeb('SISTEMA', '>>> [CONFIG] actualizado correctamente.', false);
            socket.emit('config-saved', { success: true });
        } catch (e) {
            socket.emit('config-saved', { success: false, error: e.message });
        }
    });

    socket.on('get-db-list', () => {
        if (!socket.isAdmin) return;
        let dbs = [
            { id: 'gamedata', name: '🎮 Gamedata (gamedata.db)' },
            { id: 'guilds', name: '🛡️ Guilds (guilds.db)' },
            { id: 'mapinfo', name: '🗺️ MapInfo (MapInfo.db)' }
        ];

        if (fs.existsSync(ACCOUNTS_FOLDER)) {
            const files = fs.readdirSync(ACCOUNTS_FOLDER).filter(f => f.endsWith('.db'));
            files.forEach(f => {
                dbs.push({ id: `account:${f.replace('.db', '')}`, name: `👤 Cuenta: ${f}` });
            });
        }
        socket.emit('db-list', dbs);
    });

    socket.on('get-db-tables', (dbId) => {
        if (!socket.isAdmin) return;
        const dbPath = getDbPath(dbId);
        if (!dbPath || !fs.existsSync(dbPath)) return socket.emit('db-tables', []);
        
        const db = new sqlite3.Database(dbPath, sqlite3.OPEN_READONLY);
        db.all("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'", [], (err, rows) => {
            db.close();
            if (err) return socket.emit('db-tables', []);
            socket.emit('db-tables', rows.map(r => r.name));
        });
    });

    socket.on('get-table-data', ({ dbId, tableName, searchTerm }) => {
        if (!socket.isAdmin || !tableName) return;
        const dbPath = getDbPath(dbId);
        if (!dbPath || !fs.existsSync(dbPath)) return socket.emit('table-data', { error: 'Base de datos no encontrada.' });

        const db = new sqlite3.Database(dbPath, sqlite3.OPEN_READONLY);
        const safeTable = tableName.replace(/"/g, '""'); 
        
        db.all(`PRAGMA table_info("${safeTable}")`, [], (err, cols) => {
            if (err || cols.length === 0) {
                db.close();
                return socket.emit('table-data', { error: 'No se pudieron leer las columnas de la tabla.' });
            }

            let query = `SELECT rowid as _rowid_, * FROM "${safeTable}"`;
            let params = [];

            if (searchTerm && searchTerm.trim() !== '') {
                const conditions = cols.map(c => `"${c.name}" LIKE ?`).join(' OR ');
                query += ` WHERE ${conditions}`;
                
                const safeTerm = `%${searchTerm.trim()}%`;
                params = cols.map(() => safeTerm); 
            }

            query += ` LIMIT 1000`;

            db.all(query, params, (err, rows) => {
                db.close();
                if (err) return socket.emit('table-data', { error: err.message });
                socket.emit('table-data', { rows, tableName });
            });
        });
    });

    socket.on('update-table-data', ({ dbId, tableName, rowid, column, value }) => {
        if (!socket.isAdmin || !tableName || !column) return;
        const dbPath = getDbPath(dbId);
        if (!dbPath || !fs.existsSync(dbPath)) return socket.emit('update-data-result', { success: false, error: 'DB no encontrada' });

        const db = new sqlite3.Database(dbPath);
        const safeTable = tableName.replace(/"/g, '""');
        const safeColumn = column.replace(/"/g, '""');
        
        let finalValue = value;
        if (!isNaN(value) && value.trim() !== '') {
            finalValue = Number(value); 
        }

        db.run(`UPDATE "${safeTable}" SET "${safeColumn}" = ? WHERE rowid = ?`, [finalValue, rowid], function(err) {
            db.close();
            if (err) {
                socket.emit('update-data-result', { success: false, error: err.message });
            } else {
                socket.emit('update-data-result', { success: true });
                pushLogToWeb('SISTEMA', `[DB-EDITOR] DB: ${dbId} | Tabla: ${tableName} | Columna modificada con éxito.`, false);
            }
        });
    });

    socket.on('get-dashboard-logs', () => {
        if (!socket.isAdmin) return;
        if (fs.existsSync(DASHBOARD_LOGS_PATH)) {
            try {
                const logs = JSON.parse(fs.readFileSync(DASHBOARD_LOGS_PATH, 'utf8'));
                socket.emit('dashboard-logs-data', logs);
            } catch(e) {
                socket.emit('dashboard-logs-data', []);
            }
        } else {
            socket.emit('dashboard-logs-data', []);
        }
    });

    // ==========================================
    // 🌐 SISTEMA DE AUTO-UPDATER (NATIVO WINDOWS)
    // ==========================================
    
    // 1. INICIAR/DETENER UPDATE SERVER
    socket.on('updater-start', () => {
        if (!socket.isAdmin) return;
        if (updaterProcess) return;

        // Vaciamos el log temporal y generamos el wrapper de Python
        fs.writeFileSync(UPDATER_LOG, '');
        const wrapperPath = path.join(UPDATER_DIR, 'run_updater_web.py');
        fs.writeFileSync(wrapperPath, createPythonWrapper('update_server', UPDATER_LOG));

        // Arrancamos el Chunk Watcher para emitir linea por linea a la web
        startChunkWatcher(UPDATER_LOG, 'updater-log');

        // Se usa 'start "" cmd /k' para abrir una consola nativa en el PC del usuario ejecutando nuestro wrapper
        exec('start "HB Updater" cmd /k "title HB Updater & python -u run_updater_web.py"', { cwd: UPDATER_DIR });

        updaterProcess = true; 
        broadcastToAdmins('updater-status', true);
    });

    socket.on('updater-stop', () => {
        if (!socket.isAdmin || !updaterProcess) return;
        
        // Cerramos la ventana por el titulo que le dimos al abrirla
        exec('taskkill /FI "WINDOWTITLE eq HB Updater" /T /F', (err) => {
            updaterProcess = null;
            broadcastToAdmins('updater-status', false);
        });
    });

    socket.on('updater-check-status', () => {
        if (socket.isAdmin) socket.emit('updater-status', updaterProcess !== null);
    });

    // 2. GENERAR MANIFEST
    socket.on('manifest-generate', () => {
        if (!socket.isAdmin) return;

        // Vaciamos el log temporal y generamos el wrapper de Python
        fs.writeFileSync(MANIFEST_LOG, '');
        const wrapperPath = path.join(UPDATER_DIR, 'run_manifest_web.py');
        fs.writeFileSync(wrapperPath, createPythonWrapper('gen_update_manifest', MANIFEST_LOG));

        // Arrancamos el Chunk Watcher
        startChunkWatcher(MANIFEST_LOG, 'manifest-log');
        
        // Usamos cmd /c con "pause" al final para que la ventana se quede abierta
        exec('start "HB Manifest" cmd /c "title HB Manifest & python -u run_manifest_web.py & pause"', { cwd: UPDATER_DIR });
    });
});

// ==========================================
// 📊 SISTEMA DE MONITOREO DINÁMICO Y LOGS 12H
// ==========================================
let lastCpuIdle = 0;
let lastCpuTotal = 0;
const historySize = 30; 
let playerCountHistory = Array(historySize).fill(0);
let historyLabels = Array(historySize).fill('');

let statsAccumulator = {
    cpuSum: 0,
    ramSum: 0,
    count: 0,
    peakPlayers: 0,
    startTime: Date.now()
};

let gameServerStartTime = 0;
let wasRunning = false;

function saveDashboardLog() {
    if (statsAccumulator.count === 0) {
        statsAccumulator.startTime = Date.now(); 
        return;
    }

    let logs = [];
    if (fs.existsSync(DASHBOARD_LOGS_PATH)) {
        try { logs = JSON.parse(fs.readFileSync(DASHBOARD_LOGS_PATH, 'utf8')); } catch(e){}
    }
    const avgCpu = (statsAccumulator.cpuSum / statsAccumulator.count).toFixed(1);
    const avgRam = (statsAccumulator.ramSum / statsAccumulator.count).toFixed(1);
    const now = new Date();
    
    const newLog = {
        id: Date.now(),
        date: now.toLocaleDateString(),
        time: now.toLocaleTimeString(),
        avgCpu: avgCpu,
        avgRam: avgRam,
        peakPlayers: statsAccumulator.peakPlayers,
        periodStart: new Date(statsAccumulator.startTime).toLocaleString(),
        periodEnd: now.toLocaleString()
    };
    
    logs.unshift(newLog);
    fs.writeFileSync(DASHBOARD_LOGS_PATH, JSON.stringify(logs, null, 4), 'utf8');
    
    statsAccumulator = { cpuSum: 0, ramSum: 0, count: 0, peakPlayers: 0, startTime: Date.now() };
    broadcastToAdmins('dashboard-logs-data', logs);
}

setInterval(() => {
    let gamePort = 9907; 
    try {
        if (fs.existsSync(CONFIG_FILE_PATH)) {
            const config = JSON.parse(fs.readFileSync(CONFIG_FILE_PATH, 'utf8'));
            if (config.realm && config.realm.game_listen_port) gamePort = config.realm.game_listen_port;
        }
    } catch (e) {}

    // Check para el Server de juego y para el Updater (por nombre de ventana)
    exec('tasklist /V /FO CSV', (err, stdout) => {
        const outLower = stdout ? stdout.toLowerCase() : '';
        
        // --- 1. Verificación del Server Principal ---
        const isRunning = outLower.includes('server.exe');
        broadcastToAdmins('server-state', { isRunning });

        if (isRunning && !wasRunning) {
            gameServerStartTime = Date.now();
            wasRunning = true;
        } else if (!isRunning && wasRunning) {
            gameServerStartTime = 0;
            wasRunning = false;
            playerCountHistory = Array(historySize).fill(0);
            historyLabels = Array(historySize).fill('');
        }

        // --- 2. Verificación Automática del Updater (Si el usuario cierra la ventana a mano) ---
        const isUpdaterRunning = outLower.includes('hb updater');
        if (isUpdaterRunning !== (updaterProcess !== null)) {
            updaterProcess = isUpdaterRunning ? true : null;
            broadcastToAdmins('updater-status', isUpdaterRunning);
        }

        if (!isRunning) {
            if (onlineCharacters.size > 0) {
                onlineCharacters.clear();
                broadcastToAdmins('online-players-data', []);
            }
            broadcastToAdmins('player-count', 0);
            broadcastToAdmins('dashboard-stats', {
                cpu: 0, ram: 0, playersHistory: playerCountHistory, labelsHistory: historyLabels, uptime: 0
            });
            return; 
        }

        let cpus = os.cpus();
        let idle = 0, total = 0;
        cpus.forEach(cpu => {
            for (let type in cpu.times) total += cpu.times[type];
            idle += cpu.times.idle;
        });
        
        let cpuUsage = 0;
        if (lastCpuTotal > 0) {
            let idleDiff = idle - lastCpuIdle;
            let totalDiff = total - lastCpuTotal;
            cpuUsage = 100 - Math.floor(100 * idleDiff / totalDiff);
        }
        lastCpuIdle = idle;
        lastCpuTotal = total;

        let totalRam = os.totalmem();
        let freeRam = os.freemem();
        let ramUsage = ((totalRam - freeRam) / totalRam * 100).toFixed(1);

        exec('netstat -an', (err, net_stdout) => {
            let activeConnections = 0;
            if (!err) {
                const regex = new RegExp(`TCP\\s+\\S+:${gamePort}\\s+\\S+\\s+ESTABLISHED`, 'i');
                activeConnections = net_stdout.split('\n').filter(line => regex.test(line)).length;
                
                broadcastToAdmins('player-count', activeConnections);

                if (activeConnections === 0 && onlineCharacters.size > 0) {
                    onlineCharacters.clear();
                    broadcastToAdmins('online-players-data', []);
                }
            }

            const now = new Date();
            const timeStr = now.getHours().toString().padStart(2, '0') + ':' + now.getMinutes().toString().padStart(2, '0') + ':' + now.getSeconds().toString().padStart(2, '0');
            
            playerCountHistory.shift();
            historyLabels.shift();
            playerCountHistory.push(activeConnections);
            historyLabels.push(timeStr);

            statsAccumulator.cpuSum += cpuUsage;
            statsAccumulator.ramSum += parseFloat(ramUsage);
            statsAccumulator.count++;
            if (activeConnections > statsAccumulator.peakPlayers) {
                statsAccumulator.peakPlayers = activeConnections;
            }

            if (Date.now() - statsAccumulator.startTime >= 43200000) {
                saveDashboardLog();
            }

            let currentUptime = Math.floor((Date.now() - gameServerStartTime) / 1000);

            broadcastToAdmins('dashboard-stats', {
                cpu: cpuUsage,
                ram: ramUsage,
                playersHistory: playerCountHistory,
                labelsHistory: historyLabels,
                uptime: currentUptime
            });
        });
    });
}, 3000);

server.listen(3000, () => {
    console.log('PANEL ARRANCADO EN PUERTO 3000');
});