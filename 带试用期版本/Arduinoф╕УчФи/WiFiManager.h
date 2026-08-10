#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Config.h>

// ============================================================
// Settings struct - shared between WiFiManager and main.cpp
// ============================================================
struct PetSettings {
    int screenFPS = DEFAULT_SCREEN_FPS;
    int gyroRateHz = DEFAULT_GYRO_RATE_HZ;
    int motionCooldown = DEFAULT_MOTION_COOLDOWN;
    unsigned long idleTimeout = DEFAULT_IDLE_TIMEOUT;
    bool autoDim = DEFAULT_AUTO_DIM;
    bool ecoMode = DEFAULT_ECO_MODE;
};

class PetWiFiManager {
private:
    WebServer server;
    Preferences prefs;
    bool isConfigured = false;
    bool isConnected = false;
    bool isAPMode = false;
    String savedSSID = "";
    String savedPassword = "";

    PetSettings settings;

    // Load settings from Preferences
    void loadSettings() {
        prefs.begin("pet-settings", false);
        settings.screenFPS = prefs.getInt("fps", DEFAULT_SCREEN_FPS);
        settings.gyroRateHz = prefs.getInt("gyro", DEFAULT_GYRO_RATE_HZ);
        settings.motionCooldown = prefs.getInt("cooldown", DEFAULT_MOTION_COOLDOWN);
        settings.idleTimeout = prefs.getULong("idle", DEFAULT_IDLE_TIMEOUT);
        settings.autoDim = prefs.getBool("autodim", DEFAULT_AUTO_DIM);
        settings.ecoMode = prefs.getBool("eco", DEFAULT_ECO_MODE);
        prefs.end();
    }

    // Save settings to Preferences
    void saveSettings() {
        prefs.begin("pet-settings", false);
        prefs.putInt("fps", settings.screenFPS);
        prefs.putInt("gyro", settings.gyroRateHz);
        prefs.putInt("cooldown", settings.motionCooldown);
        prefs.putULong("idle", settings.idleTimeout);
        prefs.putBool("autodim", settings.autoDim);
        prefs.putBool("eco", settings.ecoMode);
        prefs.end();
        Serial.println("Settings saved to flash");
    }

    String getSettingsJSON() {
        String json = "{";
        json += "\"fps\":" + String(settings.screenFPS) + ",";
        json += "\"gyro\":" + String(settings.gyroRateHz) + ",";
        json += "\"cooldown\":" + String(settings.motionCooldown) + ",";
        json += "\"idle\":" + String(settings.idleTimeout) + ",";
        json += "\"autodim\":" + String(settings.autoDim ? "true" : "false") + ",";
        json += "\"eco\":" + String(settings.ecoMode ? "true" : "false");
        json += "}";
        return json;
    }

    // HTML page for WiFi setup + Settings
    String getSetupPage() {
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Pet Robot Setup</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh; display: flex; align-items: center;
            justify-content: center; padding: 20px;
        }
        .container {
            background: white; border-radius: 16px; padding: 28px;
            max-width: 440px; width: 100%; box-shadow: 0 20px 60px rgba(0,0,0,0.3);
        }
        .pet-icon { text-align: center; font-size: 40px; margin-bottom: 8px; }
        h1 { color: #333; font-size: 22px; text-align: center; margin-bottom: 4px; }
        .subtitle { color: #666; font-size: 13px; text-align: center; margin-bottom: 20px; }
        .tabs { display: flex; gap: 4px; margin-bottom: 20px; }
        .tab {
            flex: 1; padding: 10px; text-align: center; border-radius: 8px 8px 0 0;
            cursor: pointer; font-size: 14px; font-weight: 600; transition: all 0.2s;
            background: #f0f0f0; color: #666;
        }
        .tab.active { background: #667eea; color: white; }
        .tab-content { display: none; }
        .tab-content.active { display: block; }
        .wifi-list {
            max-height: 220px; overflow-y: auto; margin-bottom: 16px;
            border: 1px solid #e0e0e0; border-radius: 8px;
        }
        .wifi-item {
            padding: 10px 14px; border-bottom: 1px solid #f0f0f0; cursor: pointer;
            transition: background 0.2s; display: flex; align-items: center;
            justify-content: space-between;
        }
        .wifi-item:hover { background: #f5f5ff; }
        .wifi-item.selected { background: #e8e8ff; border-left: 3px solid #667eea; }
        .wifi-item:last-child { border-bottom: none; }
        .wifi-name { font-size: 14px; color: #333; font-weight: 500; }
        .signal-bars { display: inline-flex; gap: 2px; align-items: flex-end; height: 14px; }
        .signal-bar { width: 3px; background: #ccc; border-radius: 1px; }
        .signal-bar.active { background: #667eea; }
        .form-group { margin-bottom: 14px; }
        label { display: block; font-size: 13px; color: #555; margin-bottom: 5px; font-weight: 500; }
        input[type="text"], input[type="password"], select {
            width: 100%; padding: 10px 12px; border: 1px solid #ddd;
            border-radius: 8px; font-size: 14px; transition: border-color 0.2s;
        }
        input:focus, select:focus { outline: none; border-color: #667eea; }
        .btn {
            width: 100%; padding: 12px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white; border: none; border-radius: 8px; font-size: 16px;
            font-weight: 600; cursor: pointer; transition: transform 0.2s, box-shadow 0.2s;
        }
        .btn:hover { transform: translateY(-1px); box-shadow: 0 4px 12px rgba(102,126,234,0.4); }
        .btn:disabled { opacity: 0.6; cursor: not-allowed; transform: none; }
        .btn-secondary { background: #f0f0f0; color: #333; margin-top: 8px; }
        .btn-secondary:hover { background: #e0e0e0; box-shadow: none; }
        .status {
            margin-top: 12px; padding: 10px; border-radius: 8px; text-align: center;
            font-size: 13px; display: none;
        }
        .status.success { background: #e8f5e9; color: #2e7d32; display: block; }
        .status.error { background: #ffebee; color: #c62828; display: block; }
        .status.loading { background: #e3f2fd; color: #1565c0; display: block; }
        .refresh-btn {
            background: none; border: 1px solid #ddd; padding: 6px 12px;
            border-radius: 6px; font-size: 12px; color: #666; cursor: pointer; margin-bottom: 10px;
        }
        .refresh-btn:hover { background: #f5f5f5; }
        .toggle { display: flex; align-items: center; justify-content: space-between; margin-bottom: 14px; }
        .toggle-switch { position: relative; width: 44px; height: 24px; }
        .toggle-switch input { opacity: 0; width: 0; height: 0; }
        .toggle-slider {
            position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0;
            background: #ccc; border-radius: 24px; transition: 0.3s;
        }
        .toggle-slider:before {
            content: ""; position: absolute; height: 18px; width: 18px;
            left: 3px; bottom: 3px; background: white; border-radius: 50%; transition: 0.3s;
        }
        .toggle-switch input:checked + .toggle-slider { background: #667eea; }
        .toggle-switch input:checked + .toggle-slider:before { transform: translateX(20px); }
        .note {
            background: #fff3e0; border-left: 3px solid #ff9800; padding: 10px 14px;
            border-radius: 4px; font-size: 12px; color: #e65100; margin-bottom: 16px; line-height: 1.5;
        }
        .setting-row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
        .setting-label { font-size: 13px; color: #555; font-weight: 500; }
        select { width: 140px; padding: 8px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="pet-icon">&#x1F916;</div>
        <h1>Pet Robot Setup</h1>
        <p class="subtitle">WiFi & Power Settings</p>

        <div class="tabs">
            <div class="tab active" onclick="switchTab('wifi')">WiFi</div>
            <div class="tab" onclick="switchTab('settings')">Settings</div>
        </div>

        <!-- WiFi Tab -->
        <div class="tab-content active" id="wifiTab">
            <button class="refresh-btn" onclick="scanNetworks()">&#x1F504; Scan Networks</button>
            <div class="wifi-list" id="wifiList">
                <div class="wifi-item" style="justify-content:center; color:#999;">
                    Click "Scan Networks" to find WiFi
                </div>
            </div>
            <div class="form-group">
                <label>WiFi Network</label>
                <input type="text" id="ssid" placeholder="Select network above" readonly>
            </div>
            <div class="form-group">
                <label>Password</label>
                <input type="password" id="password" placeholder="Enter WiFi password">
            </div>
            <button class="btn" id="connectBtn" onclick="connectWiFi()">Connect & Sync Time</button>
            <button class="btn btn-secondary" onclick="skipSetup()">Skip (No WiFi)</button>
            <div class="status" id="status"></div>
        </div>

        <!-- Settings Tab -->
        <div class="tab-content" id="settingsTab">
            <div class="note">
                &#9889; Power Saving: Lower frequencies = longer battery life.<br>
                RTC continues running even when powered off.
            </div>
            <div class="setting-row">
                <span class="setting-label">Screen Refresh Rate</span>
                <select id="fps">
                    <option value="10">10 FPS (Max Save)</option>
                    <option value="20">20 FPS</option>
                    <option value="30">30 FPS</option>
                    <option value="40">40 FPS</option>
                    <option value="50">50 FPS (Smoothest)</option>
                </select>
            </div>
            <div class="setting-row">
                <span class="setting-label">Gyro Sample Rate</span>
                <select id="gyro">
                    <option value="10">10 Hz (Max Save)</option>
                    <option value="20">20 Hz</option>
                    <option value="50">50 Hz</option>
                    <option value="100">100 Hz (Most Responsive)</option>
                </select>
            </div>
            <div class="setting-row">
                <span class="setting-label">Motion Cooldown</span>
                <select id="cooldown">
                    <option value="500">0.5s (Sensitive)</option>
                    <option value="1000">1s</option>
                    <option value="2000">2s</option>
                    <option value="3000">3s (Calm)</option>
                </select>
            </div>
            <div class="setting-row">
                <span class="setting-label">Idle Timeout</span>
                <select id="idle">
                    <option value="15000">15s</option>
                    <option value="30000">30s</option>
                    <option value="60000">60s</option>
                    <option value="120000">120s</option>
                </select>
            </div>
            <div class="toggle">
                <span class="setting-label">Auto Dim Screen</span>
                <label class="toggle-switch">
                    <input type="checkbox" id="autodim">
                    <span class="toggle-slider"></span>
                </label>
            </div>
            <div class="toggle">
                <span class="setting-label">Eco Mode (Aggressive Saving)</span>
                <label class="toggle-switch">
                    <input type="checkbox" id="eco">
                    <span class="toggle-slider"></span>
                </label>
            </div>
            <button class="btn" onclick="saveSettings()">Save Settings</button>
            <div class="status" id="settingsStatus"></div>
        </div>
    </div>

    <script>
        let selectedSSID = '';

        function switchTab(tab) {
            document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
            if (tab === 'wifi') {
                document.querySelector('.tab').classList.add('active');
                document.getElementById('wifiTab').classList.add('active');
            } else {
                document.querySelectorAll('.tab')[1].classList.add('active');
                document.getElementById('settingsTab').classList.add('active');
                loadSettings();
            }
        }

        async function scanNetworks() {
            const list = document.getElementById('wifiList');
            list.innerHTML = '<div class="wifi-item" style="justify-content:center;color:#999;">Scanning...</div>';
            try {
                const resp = await fetch('/scan');
                const networks = await resp.json();
                if (networks.length === 0) {
                    list.innerHTML = '<div class="wifi-item" style="justify-content:center;color:#999;">No networks found</div>';
                    return;
                }
                list.innerHTML = '';
                networks.forEach(net => {
                    const bars = getSignalBars(net.rssi);
                    const item = document.createElement('div');
                    item.className = 'wifi-item';
                    item.onclick = () => selectNetwork(net.ssid, item);
                    item.innerHTML = '<span class="wifi-name">' + net.ssid + '</span><span class="signal-bars">' + bars + '</span>';
                    list.appendChild(item);
                });
            } catch (e) {
                list.innerHTML = '<div class="wifi-item" style="justify-content:center;color:#c62828;">Scan failed</div>';
            }
        }

        function getSignalBars(rssi) {
            let bars = '';
            let level = rssi > -50 ? 4 : rssi > -60 ? 3 : rssi > -70 ? 2 : 1;
            for (let i = 1; i <= 4; i++) {
                const h = i * 3 + 2;
                bars += '<div class="signal-bar ' + (i <= level ? 'active' : '') + '" style="height:' + h + 'px"></div>';
            }
            return bars;
        }

        function selectNetwork(ssid, element) {
            document.querySelectorAll('.wifi-item').forEach(el => el.classList.remove('selected'));
            element.classList.add('selected');
            selectedSSID = ssid;
            document.getElementById('ssid').value = ssid;
        }

        async function connectWiFi() {
            const ssid = document.getElementById('ssid').value;
            const password = document.getElementById('password').value;
            const status = document.getElementById('status');
            const btn = document.getElementById('connectBtn');
            if (!ssid) {
                status.className = 'status error';
                status.textContent = 'Please select a WiFi network';
                return;
            }
            btn.disabled = true;
            status.className = 'status loading';
            status.textContent = 'Connecting & syncing time...';
            try {
                const resp = await fetch('/connect', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body: 'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password)
                });
                const result = await resp.json();
                if (result.success) {
                    status.className = 'status success';
                    status.textContent = '\u2705 Connected! Time synced. You can close this page.';
                    btn.textContent = 'Connected!';
                } else {
                    status.className = 'status error';
                    status.textContent = '\u274C ' + result.message;
                    btn.disabled = false;
                }
            } catch (e) {
                status.className = 'status error';
                status.textContent = '\u274C Connection failed';
                btn.disabled = false;
            }
        }

        function skipSetup() {
            fetch('/skip', {method: 'POST'}).then(() => {
                document.querySelector('.container').innerHTML =
                    '<div style="text-align:center;padding:40px;">' +
                    '<div style="font-size:64px;margin-bottom:16px;">\uD83D\uDC4B</div>' +
                    '<h1 style="color:#333;margin-bottom:12px;">Setup Skipped</h1>' +
                    '<p style="color:#666;">Pet robot will run without WiFi.<br>Time will sync next time you connect.</p>' +
                    '<p style="margin-top:16px;font-size:13px;color:#999;">You can close this page.</p></div>';
            });
        }

        async function loadSettings() {
            try {
                const resp = await fetch('/settings');
                const s = await resp.json();
                document.getElementById('fps').value = s.fps;
                document.getElementById('gyro').value = s.gyro;
                document.getElementById('cooldown').value = s.cooldown;
                document.getElementById('idle').value = s.idle;
                document.getElementById('autodim').checked = s.autodim;
                document.getElementById('eco').checked = s.eco;
            } catch (e) {}
        }

        async function saveSettings() {
            const status = document.getElementById('settingsStatus');
            status.className = 'status loading';
            status.textContent = 'Saving...';
            const data = 'fps=' + document.getElementById('fps').value +
                '&gyro=' + document.getElementById('gyro').value +
                '&cooldown=' + document.getElementById('cooldown').value +
                '&idle=' + document.getElementById('idle').value +
                '&autodim=' + (document.getElementById('autodim').checked ? '1' : '0') +
                '&eco=' + (document.getElementById('eco').checked ? '1' : '0');
            try {
                const resp = await fetch('/settings', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body: data
                });
                const result = await resp.json();
                if (result.success) {
                    status.className = 'status success';
                    status.textContent = '\u2705 Settings saved! Reboot to apply.';
                } else {
                    status.className = 'status error';
                    status.textContent = '\u274C Save failed';
                }
            } catch (e) {
                status.className = 'status error';
                status.textContent = '\u274C Save failed';
            }
        }
    </script>
</body>
</html>
)rawliteral";
        return html;
    }

public:
    PetWiFiManager() : server(80) {}

    // Load settings and WiFi credentials from flash WITHOUT connecting
    // Used when RTC time is valid and WiFi is not needed
    void loadConfig() {
        loadSettings();
        Serial.printf("Settings loaded: FPS=%d, Gyro=%dHz, Cooldown=%dms, Idle=%lus, Dim=%d, Eco=%d\n",
            settings.screenFPS, settings.gyroRateHz, settings.motionCooldown,
            settings.idleTimeout / 1000, settings.autoDim, settings.ecoMode);

        prefs.begin("wifi-config", false);
        savedSSID = prefs.getString("ssid", "");
        savedPassword = prefs.getString("password", "");
        prefs.end();
    }

    void begin(bool forceAP = false) {
        // Load settings from flash first
        loadSettings();
        Serial.printf("Settings loaded: FPS=%d, Gyro=%dHz, Cooldown=%dms, Idle=%lus, Dim=%d, Eco=%d\n",
            settings.screenFPS, settings.gyroRateHz, settings.motionCooldown,
            settings.idleTimeout / 1000, settings.autoDim, settings.ecoMode);

        prefs.begin("wifi-config", false);
        savedSSID = prefs.getString("ssid", "");
        savedPassword = prefs.getString("password", "");
        prefs.end();

        if (forceAP || savedSSID.length() == 0) {
            startAPMode();
            return;
        }

        // Try to connect to saved WiFi (10-second timeout)
        tryAutoConnect(10000);
    }

    // Try to connect to saved WiFi with configurable timeout.
    // Returns true if connected, false if timeout or no saved credentials.
    // Does NOT start AP mode on failure (caller decides next step).
    bool tryAutoConnect(unsigned long timeoutMs) {
        if (savedSSID.length() == 0) {
            Serial.println("No saved WiFi credentials - cannot auto-connect");
            return false;
        }

        Serial.printf("Auto-connecting to saved WiFi: %s (timeout: %lums)\n", savedSSID.c_str(), timeoutMs);
        WiFi.mode(WIFI_STA);
        WiFi.begin(savedSSID.c_str(), savedPassword.c_str());

        unsigned long startTime = millis();
        int dots = 0;
        while (WiFi.status() != WL_CONNECTED && (millis() - startTime < timeoutMs)) {
            delay(500);
            Serial.print(".");
            dots++;
            if (dots % 10 == 0) Serial.printf(" %lus\n", (millis() - startTime) / 1000);
        }

        if (WiFi.status() == WL_CONNECTED) {
            isConnected = true;
            isConfigured = true;
            Serial.printf("\nConnected! IP: %s (took %lums)\n",
                WiFi.localIP().toString().c_str(), millis() - startTime);
            return true;
        } else {
            Serial.printf("\nWiFi connection timeout after %lums\n", millis() - startTime);
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            isConnected = false;
            return false;
        }
    }

    void startAPMode() {
        isAPMode = true;
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASSWORD);
        delay(100);

        Serial.printf("AP Mode started. SSID: %s, IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());

        // Setup web server routes
        server.on("/", HTTP_GET, [this]() {
            server.send(200, "text/html", getSetupPage());
        });
        server.on("/scan", HTTP_GET, [this]() { handleScan(); });
        server.on("/connect", HTTP_POST, [this]() { handleConnect(); });
        server.on("/skip", HTTP_POST, [this]() { handleSkip(); });
        server.on("/settings", HTTP_GET, [this]() {
            server.send(200, "application/json", getSettingsJSON());
        });
        server.on("/settings", HTTP_POST, [this]() { handleSettings(); });

        server.begin();
        Serial.println("Web server started on http://192.168.4.1");
    }

    void handleClient() {
        if (isAPMode) server.handleClient();
    }

    void handleScan() {
        int n = WiFi.scanNetworks();
        String json = "[";
        for (int i = 0; i < n; i++) {
            if (i > 0) json += ",";
            json += "{";
            String ssid = WiFi.SSID(i);
            ssid.replace("\\", "\\\\");
            ssid.replace("\"", "\\\"");
            json += "\"ssid\":\"" + ssid + "\",";
            json += "\"rssi\":" + String(WiFi.RSSI(i));
            json += "}";
        }
        json += "]";
        server.send(200, "application/json", json);
    }

    void handleConnect() {
        if (server.hasArg("ssid") && server.hasArg("password")) {
            String ssid = server.arg("ssid");
            String password = server.arg("password");

            Serial.printf("Connecting to WiFi: %s\n", ssid.c_str());

            WiFi.softAPdisconnect(true);
            delay(500);
            WiFi.mode(WIFI_STA);
            WiFi.begin(ssid.c_str(), password.c_str());

            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                delay(500);
                Serial.print(".");
                attempts++;
            }

            if (WiFi.status() == WL_CONNECTED) {
                prefs.begin("wifi-config", false);
                prefs.putString("ssid", ssid);
                prefs.putString("password", password);
                prefs.end();
                savedSSID = ssid;
                savedPassword = password;
                isConnected = true;
                isConfigured = true;
                isAPMode = false;

                Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
                server.send(200, "application/json", "{\"success\":true}");
            } else {
                Serial.println("\nConnection failed!");
                server.send(200, "application/json", "{\"success\":false,\"message\":\"Connection failed. Check password.\"}");
                delay(1000);
                startAPMode();
            }
        } else {
            server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing parameters\"}");
        }
    }

    void handleSkip() {
        Serial.println("WiFi setup skipped by user");
        isAPMode = false;
        isConnected = false;
        isConfigured = false;
        server.send(200, "application/json", "{\"success\":true}");
    }

    void handleSettings() {
        if (server.hasArg("fps")) {
            settings.screenFPS = server.arg("fps").toInt();
            settings.gyroRateHz = server.arg("gyro").toInt();
            settings.motionCooldown = server.arg("cooldown").toInt();
            settings.idleTimeout = (unsigned long)server.arg("idle").toInt();
            settings.autoDim = (server.arg("autodim").toInt() == 1);
            settings.ecoMode = (server.arg("eco").toInt() == 1);
            saveSettings();
            server.send(200, "application/json", "{\"success\":true}");
        } else {
            server.send(200, "application/json", getSettingsJSON());
        }
    }

    // Getters
    bool getIsConnected() { return isConnected; }
    bool getIsConfigured() { return isConfigured; }
    bool getIsAPMode() { return isAPMode; }
    String getIPAddress() { return WiFi.localIP().toString(); }
    PetSettings& getSettings() { return settings; }

    // Disconnect WiFi to save power (after NTP sync)
    void disconnectWiFi() {
        if (isConnected) {
            Serial.println("Disconnecting WiFi to save power...");
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            isConnected = false;
            Serial.println("WiFi turned off");
        }
    }

    void enterAPMode() {
        if (isConnected) WiFi.disconnect();
        startAPMode();
    }
};

#endif // WIFI_MANAGER_H
