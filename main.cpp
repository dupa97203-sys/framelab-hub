/* ==========================================================================
   FrameLab Hub — C++ + Dear ImGui Application
   ========================================================================== */

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <thread>
#include <map>
#include <fstream>
#include <cstdio>
#include <cstdlib>

// Dear ImGui & Backends
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// GLFW
#include <GLFW/glfw3.h>

// Enums
enum Tab {
    TAB_DASHBOARD,
    TAB_APPS,
    TAB_ASSETS,
    TAB_SETTINGS,
    TAB_SIMULATOR
};

enum Theme {
    THEME_PURPLE,
    THEME_CYAN,
    THEME_GREEN,
    THEME_ORANGE
};

enum AppStatus {
    STATUS_INSTALLED,
    STATUS_UPDATE,
    STATUS_NOT_INSTALLED,
    STATUS_TRIAL
};

enum ShapeType {
    SHAPE_RECT,
    SHAPE_CIRCLE,
    SHAPE_TEXT
};

enum DesignTool {
    TOOL_SELECT,
    TOOL_RECT,
    TOOL_CIRCLE,
    TOOL_TEXT
};

// Struktury Danych
struct AppInfo {
    std::string id;
    std::string name;
    std::string category;
    std::string categoryName;
    std::string tagline;
    std::string description;
    std::string version;
    std::string size;
    AppStatus status;
    ImVec4 color;
    std::string iconText;
    float progress = 0.0f;
    bool isInstalling = false;
};

struct Project {
    int id;
    std::string name;
    std::string appId;
    std::string size;
    std::string lastModified;
};

struct Asset {
    int id;
    std::string name;
    std::string type; // image, video, vector, audio
    std::string size;
    std::string date;
    ImVec4 color; // kolor dla podglądu zastępczego
};

struct VectorShape {
    int id;
    ShapeType type;
    ImVec2 pos;
    ImVec2 size;
    ImVec4 color;
    float opacity;
    std::string name;
};

struct Toast {
    std::string message;
    std::string type; // success, warning, info
    float timer = 4.0f; // czas trwania w sekundach
};

// Globalne Zmienne Stanu
ImFont* fontRegular = nullptr;
ImFont* fontBold = nullptr;
Tab currentTab = TAB_DASHBOARD;
Theme currentTheme = THEME_PURPLE;
std::string activeSimAppId = "";
std::string activeSimProjectName = "Bez tytulu.flp";

// Konfiguracja Firebase
// For Firebase JS SDK v7.20.0 and later, measurementId is optional
/*
const firebaseConfig = {
  apiKey: "AIzaSyB9MAeztrI-MT56gPfhUq3HhgVlOu18foo",
  authDomain: "framelab-hub.firebaseapp.com",
  projectId: "framelab-hub",
  storageBucket: "framelab-hub.firebasestorage.app",
  messagingSenderId: "8787494414",
  appId: "1:8787494414:web:ce261de254afe1b78e9e5f",
  measurementId: "G-7LHE04B0R3"
};
*/
const std::string FIREBASE_API_KEY = "AIzaSyB9MAeztrI-MT56gPfhUq3HhgVlOu18foo";
const std::string FIREBASE_DATABASE_URL = "https://framelab-hub-default-rtdb.firebaseio.com/";

// Stan sesji Firebase
bool isLoggedIn = false;
bool isOfflineMode = false;
bool isSubscribed = false;
std::string fbIdToken = "";
std::string fbLocalId = "";
std::string fbSessionHash = "";
bool rememberMe = true;

struct AuroraCircle {
    ImVec2 pos;
    ImVec2 vel;
    float radius;
    ImVec4 color;
};
std::vector<AuroraCircle> auroraCircles;

struct LoginParticle {
    ImVec2 pos;
    ImVec2 vel;
    float radius;
    ImVec4 color;
    float alpha;
    float speed;
};
std::vector<LoginParticle> loginParticles;

std::vector<AppInfo> apps;
std::vector<Project> projects;
std::vector<Asset> assets;
std::vector<Toast> toasts;

// Zmienne edytorów
// --- Design ---
std::vector<VectorShape> designShapes;
int selectedShapeId = -1;
DesignTool currentDesignTool = TOOL_SELECT;

// --- Motions ---
bool motionsPlay = false;
float motionsProgress = 0.0f;

// --- Studio ---
bool studioPlay = false;
float studioProgress = 0.0f;
int studioSelectedClip = 0;
std::vector<std::string> studioClips = { "beach_sunset.mp4", "mountain_peak.mp4", "forest_mist.mp4" };
std::vector<ImVec4> studioClipColors = {
    ImVec4(0.9f, 0.45f, 0.2f, 1.0f),  // zachód słońca
    ImVec4(0.2f, 0.6f, 0.8f, 1.0f),   // szczyt górski
    ImVec4(0.15f, 0.5f, 0.2f, 1.0f)   // las we mgle
};

// Dane profilu
char profileName[128] = "Aleksandra Kowalska";
char profileEmail[128] = "aleksandra@framelab.io";
bool optAutoUpdate = true;
bool optGpuAccel = true;
bool optRunAtStartup = false;
bool notifDropdownOpen = false;

// Zmienne formularza logowania/rejestracji
char loginEmail[128] = "";
char loginPassword[128] = "";
char registerName[128] = "";
char registerEmail[128] = "";
char registerPassword[128] = "";
bool isRegisteringMode = false;
std::string loginErrorMsg = "";
bool loginLoading = false;

// Deklaracje zapowiadające
void deleteShape(int id, void* event);

// Pomocnicze funkcje powiadomień toast
void ShowToast(const std::string& message, const std::string& type = "success") {
    toasts.push_back({ message, type, 4.0f });
}

// Bezpieczne zapytanie HTTPS za pomocą systemowego cURL (bezpośrednie wywołanie)
std::string PerformHTTPSRequest(const std::string& method, const std::string& url, const std::string& payload, const std::string& contentType = "application/json") {
    std::string tempFilename = "temp_fb_req.json";
    {
        std::ofstream tempFile(tempFilename);
        if (tempFile.is_open()) {
            tempFile << payload;
        }
    }

    std::string cmd;
#ifdef _WIN32
    cmd = "curl -s -X " + method + " -H \"Content-Type: " + contentType + "\" -d @" + tempFilename + " \"" + url + "\"";
#else
    cmd = "curl -s -X " + method + " -H 'Content-Type: " + contentType + "' -d @" + tempFilename + " '" + url + "'";
#endif

    std::string result;
    char buffer[256];
#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif

    if (pipe) {
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
#ifdef _WIN32
        _pclose(pipe);
#else
        pclose(pipe);
#endif
    }

    std::remove(tempFilename.c_str());
    return result;
}

// Prosty parser JSON
std::string GetJSONValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return "";

    size_t colonPos = json.find(":", keyPos + searchKey.length());
    if (colonPos == std::string::npos) return "";

    size_t valStart = json.find_first_not_of(" \t\r\n", colonPos + 1);
    if (valStart == std::string::npos) return "";

    if (json[valStart] == '\"') {
        size_t valEnd = json.find("\"", valStart + 1);
        if (valEnd == std::string::npos) return "";
        return json.substr(valStart + 1, valEnd - valStart - 1);
    } else {
        size_t valEnd = json.find_first_of(",}", valStart);
        if (valEnd == std::string::npos) {
            valEnd = json.length();
        }
        std::string val = json.substr(valStart, valEnd - valStart);
        val.erase(std::remove_if(val.begin(), val.end(), ::isspace), val.end());
        return val;
    }
}

// Generowanie losowego hasha sesji (sliding session token)
std::string GenerateRandomHash() {
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string hash = "";
    unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::srand(seed);
    for (int i = 0; i < 32; ++i) {
        hash += alphabet[std::rand() % (sizeof(alphabet) - 1)];
    }
    return hash;
}

// Zapisanie sesji w bazie Firebase Realtime Database oraz lokalnym pliku session.txt
void SaveSession(const std::string& uid, const std::string& email, const std::string& refreshToken) {
    if (refreshToken.empty()) return;
    
    std::string sessionHash = GenerateRandomHash();
    
    // Zapisz sesję w bazie danych (ścieżka /sessions/[hash])
    std::string dbUrl = FIREBASE_DATABASE_URL + "sessions/" + sessionHash + ".json";
    std::string dbPayload = "{\"uid\":\"" + uid + "\",\"email\":\"" + email + "\",\"refreshToken\":\"" + refreshToken + "\"}";
    PerformHTTPSRequest("PUT", dbUrl, dbPayload);
    
    // Zapisz sesję lokalnie w pliku
    std::ofstream sessionFile("session.txt");
    if (sessionFile.is_open()) {
        sessionFile << uid << "\n" << sessionHash << "\n";
        sessionFile.close();
    }
    
    fbSessionHash = sessionHash;
}

// Skasowanie lokalnej sesji i usunięcie jej z bazy danych
void DeleteLocalSession() {
    if (!fbSessionHash.empty()) {
        // Usuń sesję z bazy danych
        std::string dbUrl = FIREBASE_DATABASE_URL + "sessions/" + fbSessionHash + ".json";
        PerformHTTPSRequest("DELETE", dbUrl, "");
        fbSessionHash = "";
    }
    std::remove("session.txt");
}

// Odczytanie i weryfikacja zapisanej sesji (rotacja tokenu)
bool CheckSavedSession() {
    if (FIREBASE_API_KEY == "AIzaSyDummyKeyReplaceThisWithYourOwn") {
        return false;
    }

    std::ifstream sessionFile("session.txt");
    if (!sessionFile.is_open()) return false;
    
    std::string uid, sessionHash;
    if (!std::getline(sessionFile, uid) || !std::getline(sessionFile, sessionHash)) {
        sessionFile.close();
        std::remove("session.txt");
        return false;
    }
    sessionFile.close();
    
    // Pobierz dane sesji z Firebase Realtime Database
    std::string dbUrl = FIREBASE_DATABASE_URL + "sessions/" + sessionHash + ".json";
    std::string dbResponse = PerformHTTPSRequest("GET", dbUrl, "");
    
    std::string dbUid = GetJSONValue(dbResponse, "uid");
    std::string dbRefreshToken = GetJSONValue(dbResponse, "refreshToken");
    std::string dbEmail = GetJSONValue(dbResponse, "email");
    
    if (dbUid.empty() || dbRefreshToken.empty() || dbUid != uid) {
        std::remove("session.txt");
        return false;
    }
    
    // Natychmiast usuwamy starą sesję z bazy danych (rotacja tokenu!)
    PerformHTTPSRequest("DELETE", dbUrl, "");
    
    // Odświeżamy ID token za pomocą Firebase Auth Refresh Token API
    std::string refreshUrl = "https://securetoken.googleapis.com/v1/token?key=" + FIREBASE_API_KEY;
    std::string refreshPayload = "grant_type=refresh_token&refresh_token=" + dbRefreshToken;
    
    std::string refreshResponse = PerformHTTPSRequest("POST", refreshUrl, refreshPayload, "application/x-www-form-urlencoded");
    
    std::string newIdToken = GetJSONValue(refreshResponse, "id_token");
    std::string newRefreshToken = GetJSONValue(refreshResponse, "refresh_token");
    
    if (newIdToken.empty() || newRefreshToken.empty()) {
        std::remove("session.txt");
        return false;
    }
    
    // Zapisz nową sesję z nowym, wyrotowanym hashem
    SaveSession(uid, dbEmail, newRefreshToken);
    
    // Zaloguj użytkownika do aplikacji
    isLoggedIn = true;
    fbIdToken = newIdToken;
    fbLocalId = uid;
    
    // Pobierz dane profilu
    std::string userDbUrl = FIREBASE_DATABASE_URL + "users/" + uid + ".json?auth=" + newIdToken;
    std::string userDbResponse = PerformHTTPSRequest("GET", userDbUrl, "");
    
    std::string dbName = GetJSONValue(userDbResponse, "name");
    if (!dbName.empty()) {
        snprintf(profileName, sizeof(profileName), "%s", dbName.c_str());
    }
    snprintf(profileEmail, sizeof(profileEmail), "%s", dbEmail.c_str());
    
    std::string dbSub = GetJSONValue(userDbResponse, "subscribed");
    isSubscribed = (dbSub == "true");
    
    ShowToast("Witaj z powrotem!", "success");
    return true;
}

// Uwierzytelnianie Firebase: Rejestracja
bool FirebaseSignUp(const std::string& email, const std::string& password, const std::string& name, std::string& errorMsg) {
    if (FIREBASE_API_KEY == "AIzaSyDummyKeyReplaceThisWithYourOwn") {
        errorMsg = "Firebase nie jest skonfigurowany. Uzyj Trybu Offline.";
        return false;
    }
    std::string url = "https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=" + FIREBASE_API_KEY;
    std::string payload = "{\"email\":\"" + email + "\",\"password\":\"" + password + "\",\"returnSecureToken\":true}";
    
    std::string response = PerformHTTPSRequest("POST", url, payload);
    std::string errorVal = GetJSONValue(response, "message");
    if (!errorVal.empty()) {
        if (errorVal == "EMAIL_EXISTS") errorMsg = "Ten e-mail jest juz zarejestrowany.";
        else if (errorVal == "INVALID_EMAIL") errorMsg = "Niepoprawny format adresu e-mail.";
        else if (errorVal == "WEAK_PASSWORD") errorMsg = "Haslo musi miec co najmniej 6 znakow.";
        else errorMsg = "Blad rejestracji: " + errorVal;
        return false;
    }

    std::string localId = GetJSONValue(response, "localId");
    std::string idToken = GetJSONValue(response, "idToken");
    std::string refreshToken = GetJSONValue(response, "refreshToken");
    if (localId.empty() || idToken.empty()) {
        errorMsg = "Nieznany blad serwera Firebase.";
        return false;
    }

    fbLocalId = localId;
    fbIdToken = idToken;

    // Zapisz profil (imię i e-mail) do Realtime Database
    std::string dbUrl = FIREBASE_DATABASE_URL + "users/" + localId + ".json?auth=" + idToken;
    std::string dbPayload = "{\"name\":\"" + name + "\",\"email\":\"" + email + "\",\"subscribed\":false}";
    PerformHTTPSRequest("PUT", dbUrl, dbPayload);

    isSubscribed = false;
    snprintf(profileName, sizeof(profileName), "%s", name.c_str());
    snprintf(profileEmail, sizeof(profileEmail), "%s", email.c_str());

    if (rememberMe) {
        SaveSession(localId, email, refreshToken);
    } else {
        DeleteLocalSession();
    }

    return true;
}

// Uwierzytelnianie Firebase: Logowanie
bool FirebaseSignIn(const std::string& email, const std::string& password, std::string& errorMsg) {
    if (FIREBASE_API_KEY == "AIzaSyDummyKeyReplaceThisWithYourOwn") {
        errorMsg = "Firebase nie jest skonfigurowany. Uzyj Trybu Offline.";
        return false;
    }
    std::string url = "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=" + FIREBASE_API_KEY;
    std::string payload = "{\"email\":\"" + email + "\",\"password\":\"" + password + "\",\"returnSecureToken\":true}";
    
    std::string response = PerformHTTPSRequest("POST", url, payload);
    std::string errorVal = GetJSONValue(response, "message");
    if (!errorVal.empty()) {
        if (errorVal == "EMAIL_NOT_FOUND" || errorVal == "INVALID_LOGIN_CREDENTIALS" || errorVal == "INVALID_PASSWORD") {
            errorMsg = "Bledny e-mail lub haslo.";
        } else if (errorVal == "USER_DISABLED") {
            errorMsg = "To konto zostalo zablokowane.";
        } else {
            errorMsg = "Blad logowania: " + errorVal;
        }
        return false;
    }

    std::string localId = GetJSONValue(response, "localId");
    std::string idToken = GetJSONValue(response, "idToken");
    std::string refreshToken = GetJSONValue(response, "refreshToken");
    if (localId.empty() || idToken.empty()) {
        errorMsg = "Nieznany blad serwera Firebase.";
        return false;
    }

    fbLocalId = localId;
    fbIdToken = idToken;

    // Pobierz profil z bazy danych
    std::string dbUrl = FIREBASE_DATABASE_URL + "users/" + localId + ".json?auth=" + idToken;
    std::string dbResponse = PerformHTTPSRequest("GET", dbUrl, "");
    
    std::string dbName = GetJSONValue(dbResponse, "name");
    if (!dbName.empty()) {
        snprintf(profileName, sizeof(profileName), "%s", dbName.c_str());
    }
    snprintf(profileEmail, sizeof(profileEmail), "%s", email.c_str());

    std::string dbSub = GetJSONValue(dbResponse, "subscribed");
    isSubscribed = (dbSub == "true");

    if (rememberMe) {
        SaveSession(localId, email, refreshToken);
    } else {
        DeleteLocalSession();
    }

    return true;
}

void InitParticles(ImVec2 displaySize) {
    loginParticles.clear();
    for (int i = 0; i < 90; ++i) {
        float r = 1.2f + (std::rand() % 150) / 100.0f; // 1.2 to 2.7 px
        float alpha = 0.2f + (std::rand() % 50) / 100.0f; // 0.2 to 0.7
        ImVec4 col = (std::rand() % 2 == 0) ? 
            ImVec4(0.54f, 0.36f, 0.96f, alpha) : // fioletowa
            ImVec4(0.02f, 0.71f, 0.83f, alpha);   // cyjanowa
        
        float px = std::rand() % (int)(displaySize.x > 0 ? displaySize.x : 1280);
        float py = std::rand() % (int)(displaySize.y > 0 ? displaySize.y : 800);
        float vx = ((std::rand() % 100) - 50) * 0.4f; // -20 to 20 px/s
        float vy = ((std::rand() % 100) - 50) * 0.4f; // -20 to 20 px/s
        float speed = 0.8f + (std::rand() % 100) / 100.0f;
        loginParticles.push_back({ ImVec2(px, py), ImVec2(vx, vy), r, col, alpha, speed });
    }
}

// Ekran logowania/rejestracji ImGui z animowaną zorzą w tle (Aurora background) i płynnym resizingiem
void DrawAuroraBackground(ImVec2 displaySize, float deltaTime) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    
    // Rysowanie głębokiego ciemnego tła
    drawList->AddRectFilled(ImVec2(0, 0), displaySize, IM_COL32(6, 6, 10, 255));
    
    // Aktualizacja i rysowanie kul zorzy z rozmyciem kołowym
    for (auto& circle : auroraCircles) {
        circle.pos.x += circle.vel.x * deltaTime;
        circle.pos.y += circle.vel.y * deltaTime;
        
        // Odbicia od krawędzi ekranu z uwzględnieniem promienia
        if (circle.pos.x - circle.radius < 0) { circle.pos.x = circle.radius; circle.vel.x = -circle.vel.x; }
        if (circle.pos.x + circle.radius > displaySize.x) { circle.pos.x = displaySize.x - circle.radius; circle.vel.x = -circle.vel.x; }
        if (circle.pos.y - circle.radius < 0) { circle.pos.y = circle.radius; circle.vel.y = -circle.vel.y; }
        if (circle.pos.y + circle.radius > displaySize.y) { circle.pos.y = displaySize.y - circle.radius; circle.vel.y = -circle.vel.y; }
        
        int steps = 12;
        for (int i = 0; i < steps; ++i) {
            float r = circle.radius * (1.0f - (float)i / steps);
            float alpha = circle.color.w * ((float)i / steps);
            ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(circle.color.x, circle.color.y, circle.color.z, alpha));
            drawList->AddCircleFilled(circle.pos, r, col, 64);
        }
    }

    // Aktualizacja i rysowanie interaktywnych cząsteczek
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    bool hasMouse = ImGui::IsMousePosValid(&mousePos);

    for (auto& p : loginParticles) {
        // Ruch własny cząsteczki
        p.pos.x += p.vel.x * deltaTime;
        p.pos.y += p.vel.y * deltaTime;

        // Reakcja na kursor myszy (przyciąganie)
        if (hasMouse) {
            float dx = mousePos.x - p.pos.x;
            float dy = mousePos.y - p.pos.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            
            if (dist < 250.0f && dist > 5.0f) {
                float force = (250.0f - dist) / 250.0f; // siła wzrasta bliżej kursora
                p.vel.x += (dx / dist) * force * deltaTime * 240.0f * p.speed;
                p.vel.y += (dy / dist) * force * deltaTime * 240.0f * p.speed;
            }
            
            // Rysowanie delikatnych linii połączeń z myszką dla bardzo bliskich cząsteczek
            if (dist < 120.0f) {
                float lineAlpha = (1.0f - (dist / 120.0f)) * 0.15f * p.alpha;
                ImU32 lineCol = ImGui::ColorConvertFloat4ToU32(ImVec4(p.color.x, p.color.y, p.color.z, lineAlpha));
                drawList->AddLine(p.pos, mousePos, lineCol, 1.0f);
            }
        }

        // Tłumienie prędkości (tarcia), aby cząsteczki nie przyspieszały w nieskończoność
        p.vel.x -= p.vel.x * deltaTime * 0.5f;
        p.vel.y -= p.vel.y * deltaTime * 0.5f;

        // Ograniczenie maksymalnej prędkości
        float speedVal = std::sqrt(p.vel.x * p.vel.x + p.vel.y * p.vel.y);
        float maxSpeed = 100.0f * p.speed;
        if (speedVal > maxSpeed) {
            p.vel.x = (p.vel.x / speedVal) * maxSpeed;
            p.vel.y = (p.vel.y / speedVal) * maxSpeed;
        }

        // Zawijanie na krawędziach ekranu
        if (p.pos.x < -10.0f) p.pos.x = displaySize.x + 10.0f;
        if (p.pos.x > displaySize.x + 10.0f) p.pos.x = -10.0f;
        if (p.pos.y < -10.0f) p.pos.y = displaySize.y + 10.0f;
        if (p.pos.y > displaySize.y + 10.0f) p.pos.y = -10.0f;

        // Rysowanie cząsteczki
        ImU32 pCol = ImGui::ColorConvertFloat4ToU32(p.color);
        drawList->AddCircleFilled(p.pos, p.radius, pCol, 8);
    }
}

void RenderLoginScreen(ImVec2 displaySize, float deltaTime) {
    static bool particlesInitialized = false;
    if (!particlesInitialized) {
        InitParticles(displaySize);
        particlesInitialized = true;
    }

    // Rysowanie animowanej zorzy w tle logowania
    DrawAuroraBackground(displaySize, deltaTime);

    // Obliczanie płynnej wysokości okna (smooth damp) w zależności od wybranego trybu
    float targetHeight = isRegisteringMode ? 440.0f : 340.0f;
    static float currentHeight = targetHeight;
    currentHeight += (targetHeight - currentHeight) * deltaTime * 10.0f;

    ImGui::SetNextWindowPos(ImVec2(displaySize.x / 2.0f, displaySize.y / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(420, currentHeight));
    
    // Glassmorphism - obniżona przezroczystość (0.80f) tła okna
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.12f, 0.80f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    
    ImGui::Begin("Logowanie / Rejestracja", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // Rysowanie pulsującej neonowej obwódki (neon border glow)
    ImDrawList* windowDrawList = ImGui::GetWindowDrawList();
    ImVec2 minPos = ImGui::GetWindowPos();
    ImVec2 maxPos = ImVec2(minPos.x + ImGui::GetWindowWidth(), minPos.y + ImGui::GetWindowHeight());
    
    static float glowTime = 0.0f;
    glowTime += deltaTime;
    float pulse = 0.5f + 0.5f * sin(glowTime * 3.5f);
    ImVec4 glowColor = ImVec4(0.54f, 0.36f, 0.96f, 0.25f + pulse * 0.25f);
    windowDrawList->AddRect(minPos, maxPos, ImGui::ColorConvertFloat4ToU32(glowColor), 10.0f, 0, 2.5f);

    ImGui::Spacing();
    
    // Animowany nagłówek z przechodzącym kolorem (neon transition title)
    float windowWidth = ImGui::GetWindowWidth();
    std::string titleStr = "F R A M E L A B  H U B";
    ImVec2 titleSize = fontBold ? fontBold->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, titleStr.c_str()) : ImGui::CalcTextSize(titleStr.c_str());
    ImGui::SetCursorPosX((windowWidth - titleSize.x) / 2.0f);
    
    if (fontBold) ImGui::PushFont(fontBold);
    float hueBlend = 0.5f + 0.5f * sin(glowTime * 2.0f);
    ImVec4 titleCol = ImVec4(0.75f + hueBlend * 0.2f, 0.35f, 0.96f - hueBlend * 0.15f, 1.0f);
    ImGui::TextColored(titleCol, "%s", titleStr.c_str());
    if (fontBold) ImGui::PopFont();
    
    std::string subTitleStr = "Uwierzytelnianie konta w chmurze";
    ImVec2 subSize = ImGui::CalcTextSize(subTitleStr.c_str());
    ImGui::SetCursorPosX((windowWidth - subSize.x) / 2.0f);
    ImGui::TextDisabled("%s", subTitleStr.c_str());
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (!isRegisteringMode) {
        ImGui::TextDisabled("Adres E-mail");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputTextWithHint("##login_email", "np. jan.kowalski@email.com", loginEmail, IM_ARRAYSIZE(loginEmail));
        ImGui::Spacing();

        ImGui::TextDisabled("Hasło dostępu");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputTextWithHint("##login_pass", "Wprowadz haslo", loginPassword, IM_ARRAYSIZE(loginPassword), ImGuiInputTextFlags_Password);
        ImGui::Spacing();

        ImGui::Checkbox("Zapamietaj mnie na tym urzadzeniu", &rememberMe);
        ImGui::Spacing();

        if (!loginLoading) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.54f, 0.36f, 0.96f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.48f, 0.98f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.28f, 0.88f, 1.0f));
            
            if (ImGui::Button("ZALOGUJ SIE", ImVec2(ImGui::GetContentRegionAvail().x, 38))) {
                loginLoading = true;
                loginErrorMsg = "";
                std::string err;
                if (FirebaseSignIn(loginEmail, loginPassword, err)) {
                    isLoggedIn = true;
                    ShowToast("Zalogowano pomyslnie!", "success");
                } else {
                    loginErrorMsg = err;
                }
                loginLoading = false;
            }
            ImGui::PopStyleColor(3);
        } else {
            ImGui::Button("Logowanie...", ImVec2(ImGui::GetContentRegionAvail().x, 38));
        }

        ImGui::Spacing();
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Nie masz konta? Zarejestruj sie").x) / 2.0f);
        if (ImGui::Selectable("Nie masz konta? Zarejestruj sie", false, 0, ImGui::CalcTextSize("Nie masz konta? Zarejestruj sie"))) {
            isRegisteringMode = true;
            loginErrorMsg = "";
        }
    } else {
        ImGui::TextDisabled("Nazwa użytkownika / Imię");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputTextWithHint("##reg_name", "np. Aleksandra", registerName, IM_ARRAYSIZE(registerName));
        ImGui::Spacing();

        ImGui::TextDisabled("Adres E-mail");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputTextWithHint("##reg_email", "twoj@email.com", registerEmail, IM_ARRAYSIZE(registerEmail));
        ImGui::Spacing();

        ImGui::TextDisabled("Hasło (min. 6 znaków)");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputTextWithHint("##reg_pass", "Wprowadz haslo", registerPassword, IM_ARRAYSIZE(registerPassword), ImGuiInputTextFlags_Password);
        ImGui::Spacing();

        if (!loginLoading) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.54f, 0.36f, 0.96f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.48f, 0.98f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.28f, 0.88f, 1.0f));
            
            if (ImGui::Button("ZAREJESTRUJ SIE", ImVec2(ImGui::GetContentRegionAvail().x, 38))) {
                loginLoading = true;
                loginErrorMsg = "";
                std::string err;
                if (FirebaseSignUp(registerEmail, registerPassword, registerName, err)) {
                    isLoggedIn = true;
                    ShowToast("Konto utworzone i zalogowane!", "success");
                } else {
                    loginErrorMsg = err;
                }
                loginLoading = false;
            }
            ImGui::PopStyleColor(3);
        } else {
            ImGui::Button("Rejestracja...", ImVec2(ImGui::GetContentRegionAvail().x, 38));
        }

        ImGui::Spacing();
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Masz juz konto? Zaloguj sie").x) / 2.0f);
        if (ImGui::Selectable("Masz juz konto? Zaloguj sie", false, 0, ImGui::CalcTextSize("Masz juz konto? Zaloguj sie"))) {
            isRegisteringMode = false;
            loginErrorMsg = "";
        }
    }

    if (!loginErrorMsg.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "%s", loginErrorMsg.c_str());
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();
}

// Zmiana motywu w locie
void ApplyTheme(Theme theme) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4 prim, sec;
    
    if (theme == THEME_PURPLE) {
        prim = ImVec4(0.545f, 0.360f, 0.964f, 1.0f); // #8b5cf6
        sec = ImVec4(0.925f, 0.282f, 0.599f, 1.0f);  // #ec4899
    } else if (theme == THEME_CYAN) {
        prim = ImVec4(0.023f, 0.713f, 0.831f, 1.0f); // #06b6d4
        sec = ImVec4(0.231f, 0.509f, 0.964f, 1.0f);  // #3b82f6
    } else if (theme == THEME_GREEN) {
        prim = ImVec4(0.062f, 0.725f, 0.505f, 1.0f); // #10b981
        sec = ImVec4(0.019f, 0.588f, 0.411f, 1.0f);  // #059669
    } else { // THEME_ORANGE
        prim = ImVec4(0.960f, 0.620f, 0.043f, 1.0f); // #f59e0b
        sec = ImVec4(0.937f, 0.266f, 0.266f, 1.0f);  // #ef4444
    }

    style.Colors[ImGuiCol_Text] = ImVec4(0.97f, 0.98f, 0.99f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.08f, 0.98f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.11f, 1.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.14f, 0.98f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.25f, 0.30f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.23f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
    
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.11f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.08f, 0.11f, 1.00f);
    
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.11f, 0.30f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.18f, 0.18f, 0.23f, 0.80f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.22f, 0.22f, 0.28f, 0.80f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.26f, 0.26f, 0.32f, 0.80f);
    
    style.Colors[ImGuiCol_CheckMark] = prim;
    style.Colors[ImGuiCol_SliderGrab] = prim;
    style.Colors[ImGuiCol_SliderGrabActive] = sec;
    
    style.Colors[ImGuiCol_Button] = ImVec4(prim.x, prim.y, prim.z, 0.70f);
    style.Colors[ImGuiCol_ButtonHovered] = prim;
    style.Colors[ImGuiCol_ButtonActive] = sec;
    
    style.Colors[ImGuiCol_Header] = ImVec4(prim.x, prim.y, prim.z, 0.40f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(prim.x, prim.y, prim.z, 0.60f);
    style.Colors[ImGuiCol_HeaderActive] = sec;
    
    style.Colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.25f, 0.30f);
    style.Colors[ImGuiCol_SeparatorHovered] = prim;
    style.Colors[ImGuiCol_SeparatorActive] = sec;
    
    style.Colors[ImGuiCol_Tab] = ImVec4(prim.x, prim.y, prim.z, 0.40f);
    style.Colors[ImGuiCol_TabHovered] = prim;
    style.Colors[ImGuiCol_TabActive] = sec;
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.11f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
}

// Inicjalizacja danych początkowych
void InitData() {
    // Inicjalizacja kul zorzy (Aurora) do tła logowania
    auroraCircles.push_back({ ImVec2(200, 200), ImVec2(30.0f, 25.0f), 180.0f, ImVec4(0.54f, 0.36f, 0.96f, 0.15f) }); // fioletowa
    auroraCircles.push_back({ ImVec2(800, 500), ImVec2(-25.0f, 40.0f), 220.0f, ImVec4(0.02f, 0.71f, 0.83f, 0.12f) }); // cyjanowa
    auroraCircles.push_back({ ImVec2(500, 300), ImVec2(15.0f, -30.0f), 150.0f, ImVec4(0.92f, 0.28f, 0.60f, 0.10f) }); // różowa

    // Aplikacje
    apps.push_back({ "motions", "FrameLab Motions", "video", "Wideo i Animacja", "Efekty wizualne i ruch", "Profesjonalne srodowisko do animacji wektorowej i efektów wizualnych.", "v2.4.1", "1.8 GB", STATUS_INSTALLED, ImVec4(0.54f, 0.36f, 0.96f, 1.0f), "Mo" });
    apps.push_back({ "studio", "FrameLab Studio", "video", "Montaz Wideo", "Nieliniowy montaz wideo", "Kompleksowy edytor wideo do montazu wielosciezkowego i gradacji barw.", "v4.2.0", "3.1 GB", STATUS_UPDATE, ImVec4(0.02f, 0.71f, 0.83f, 1.0f), "St" });
    apps.push_back({ "design", "FrameLab Design", "design", "Projektowanie i UI", "Grafika wektorowa i UI", "Szybkie, wektorowe narzedzie do projektowania interfejsów i ilustracji.", "v1.1.0", "920 MB", STATUS_NOT_INSTALLED, ImVec4(0.92f, 0.28f, 0.60f, 1.0f), "De" });
    apps.push_back({ "effects", "FrameLab Effects", "design", "Korekcja Barwna", "Color grading i postprodukcja", "Standard postprodukcji wideo, LUT i śledzenie obiektów.", "v3.0.1", "1.5 GB", STATUS_NOT_INSTALLED, ImVec4(0.96f, 0.62f, 0.04f, 1.0f), "Ef" });
    apps.push_back({ "audio", "FrameLab Audio", "video", "Sound Design", "Cyfrowa konsola dzwieku", "Studio dzwieku przeznaczone do masteringu, miksowania i podcastów.", "v2.0.0", "640 MB", STATUS_TRIAL, ImVec4(0.06f, 0.72f, 0.50f, 1.0f), "Au" });

    // Projekty
    projects.push_back({ 1, "Animacja_Logo_2026.flp", "motions", "12.4 MB", "2 godziny temu" });
    projects.push_back({ 2, "Film_Wakacyjny_Korfu.flp", "studio", "2.1 GB", "Wczoraj" });
    projects.push_back({ 3, "Nowy_Layout_Mobile.flp", "design", "4.8 MB", "3 dni temu" });

    // Zasoby
    assets.push_back({ 1, "tlo_cyberpunk.jpg", "image", "3.4 MB", "Wczoraj", ImVec4(0.9f, 0.2f, 0.4f, 1.0f) });
    assets.push_back({ 2, "wideo_z_klifu.mp4", "video", "48.2 MB", "2 dni temu", ImVec4(0.2f, 0.5f, 0.9f, 1.0f) });
    assets.push_back({ 3, "ikony_interfejsu.svg", "vector", "240 KB", "Tydzien temu", ImVec4(0.2f, 0.8f, 0.5f, 1.0f) });
    assets.push_back({ 4, "soundtrack_ambient.mp3", "audio", "6.2 MB", "10 dni temu", ImVec4(0.7f, 0.3f, 0.8f, 1.0f) });

    // Kształty do edytora wektorowego (Design)
    designShapes.push_back({ 1, SHAPE_RECT, ImVec2(50.0f, 60.0f), ImVec2(100.0f, 80.0f), ImVec4(0.54f, 0.36f, 0.96f, 1.0f), 1.0f, "Prostokat 1" });
    designShapes.push_back({ 2, SHAPE_CIRCLE, ImVec2(200.0f, 120.0f), ImVec2(90.0f, 90.0f), ImVec4(0.92f, 0.28f, 0.60f, 1.0f), 0.9f, "Kolo 1" });
}

// Obsługa paska postępu instalacji
void UpdateInstallations(float deltaTime) {
    for (auto& app : apps) {
        if (app.isInstalling) {
            app.progress += deltaTime * 0.2f; // ~5 sekund instalacji
            if (app.progress >= 1.0f) {
                app.progress = 0.0f;
                app.isInstalling = false;
                app.status = STATUS_INSTALLED;
                ShowToast("Pobrano i zainstalowano " + app.name + "!", "success");
            }
        }
    }
}

// Uruchamianie aplikacji w symulatorze
void LaunchAppSim(const std::string& appId, const std::string& projName = "Bez tytulu") {
    activeSimAppId = appId;
    activeSimProjectName = projName;
    
    // Dodaj odpowiednie rozszerzenie do nazwy projektu jeśli brak
    std::string ext = (appId == "motions") ? ".flp" : ((appId == "studio") ? ".fsp" : ".fdp");
    if (activeSimProjectName.find(ext) == std::string::npos) {
        activeSimProjectName += ext;
    }

    currentTab = TAB_SIMULATOR;

    // Resetowanie zmiennych symulatorów
    motionsPlay = false;
    studioPlay = false;
    
    ShowToast("Uruchomiono edytor dla: " + appId, "info");
}

int main(int, char**) {
    // Inicjalizacja GLFW
    if (!glfwInit()) {
        std::cerr << "Nie udalo sie zainicjalizowac GLFW!" << std::endl;
        return 1;
    }

    // Konfiguracja kontekstu OpenGL
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Stworzenie okna
    GLFWwindow* window = glfwCreateWindow(1280, 800, "FrameLab Hub", nullptr, nullptr);
    if (!window) {
        std::cerr << "Nie udalo sie stworzyc okna GLFW!" << std::endl;
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // V-Sync

    // Konfiguracja ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Załadowanie czcionek systemowych z obsługą polskich znaków
    fontRegular = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf", 16.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    if (!fontRegular) {
        fontRegular = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 15.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    }

    fontBold = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf", 20.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    if (!fontBold) {
        fontBold = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 19.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    }

    // Inicjalizacja stylów i czcionek
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 10.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 10.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 6.0f;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;

    ApplyTheme(currentTheme);

    // Inicjalizacja backendów
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    InitData();
    CheckSavedSession();

    // Główna pętla programu
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Obliczenie deltaTime
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        UpdateInstallations(deltaTime);

        // Uaktualnienie timerów powiadomień toast
        for (auto it = toasts.begin(); it != toasts.end();) {
            it->timer -= deltaTime;
            if (it->timer <= 0.0f) {
                it = toasts.erase(it);
            } else {
                ++it;
            }
        }

        // Początek klatki ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Pokaż powiadomienia toast w prawym dolnym rogu
        if (!toasts.empty()) {
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 20, io.DisplaySize.y - 20), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
            ImGui::Begin("Toasts", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
            for (const auto& toast : toasts) {
                ImVec4 borderCol = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                if (toast.type == "success") borderCol = ImVec4(0.06f, 0.72f, 0.50f, 1.0f);
                else if (toast.type == "warning") borderCol = ImVec4(0.96f, 0.62f, 0.04f, 1.0f);
                else if (toast.type == "info") borderCol = ImVec4(0.02f, 0.71f, 0.83f, 1.0f);

                ImGui::PushStyleColor(ImGuiCol_Border, borderCol);
                ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.0f);
                ImGui::BeginChild(toast.message.c_str(), ImVec2(280, 50), true, ImGuiWindowFlags_NoScrollbar);
                
                // Ikona i tekst
                if (toast.type == "success") ImGui::TextColored(borderCol, "[ OK ]");
                else if (toast.type == "warning") ImGui::TextColored(borderCol, "[ ! ]");
                else ImGui::TextColored(borderCol, "[ i ]");
                ImGui::SameLine();
                ImGui::TextWrapped("%s", toast.message.c_str());
                
                ImGui::EndChild();
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
                ImGui::Spacing();
            }
            ImGui::End();
        }

        // Nowy warunek uwierzytelniania
        if (!isLoggedIn && !isOfflineMode) {
            RenderLoginScreen(io.DisplaySize, deltaTime);
        } else {
            // Przełączenie widoku - Symulator zajmuje całe okno
            if (currentTab == TAB_SIMULATOR) {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGui::Begin("SimulatorWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar);

            // Górny pasek menu symulatora
            if (ImGui::BeginMenuBar()) {
                std::string appName = "Niezidentyfikowana Aplikacja";
                ImVec4 appColor = ImVec4(1, 1, 1, 1);
                for (const auto& app : apps) {
                    if (app.id == activeSimAppId) {
                        appName = app.name;
                        appColor = app.color;
                        break;
                    }
                }

                // Kolorowy indykator na pasku menu
                ImGui::TextColored(appColor, "[ %s ]", appName.c_str());
                ImGui::Separator();
                
                if (ImGui::BeginMenu("Plik")) {
                    if (ImGui::MenuItem("Nowy projekt")) {}
                    if (ImGui::MenuItem("Zapisz projekt")) { ShowToast("Projekt zostal zapisany!", "success"); }
                    if (ImGui::MenuItem("Zamknij program")) { currentTab = TAB_DASHBOARD; }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edycja")) { ImGui::EndMenu(); }
                if (ImGui::BeginMenu("Widok")) { ImGui::EndMenu(); }
                if (ImGui::BeginMenu("Efekty")) { ImGui::EndMenu(); }
                
                ImGui::Separator();
                ImGui::TextDisabled("Aktywny plik: %s", activeSimProjectName.c_str());

                // Przycisk zamknięcia symulacji na samym końcu paska
                float closeBtnPos = io.DisplaySize.x - 100;
                ImGui::SetCursorPosX(closeBtnPos);
                if (ImGui::Button("Wróć do Hub", ImVec2(90, 0))) {
                    currentTab = TAB_DASHBOARD;
                    ShowToast("Zamknieto symulator", "info");
                }

                ImGui::EndMenuBar();
            }

            // Renderowanie specyficznego obszaru roboczego
            if (activeSimAppId == "motions") {
                // SYMULATOR MOTIONS (Kula i oś czasu)
                ImGui::Columns(2, "motions_cols", true);
                ImGui::SetColumnWidth(0, 200);

                // Lewa kolumna: Warstwy
                ImGui::Text("Warstwy animacji");
                ImGui::Separator();
                ImGui::Selectable("Kula Swiecaca (Wektor)", true);
                
                ImGui::NextColumn();

                // Prawa kolumna: Podgląd i Oś Czasu (Podział w pionie)
                ImVec2 space = ImGui::GetContentRegionAvail();
                float previewHeight = space.y - 180.0f;
                
                // Płótno animacji
                ImGui::BeginChild("PreviewCanvas", ImVec2(0, previewHeight), true);
                ImGui::Text("Podglad klatki (Frame Preview)");
                
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 canvasPos = ImGui::GetCursorScreenPos();
                ImVec2 canvasSize = ImGui::GetContentRegionAvail();

                // Tło siatki
                ImU32 gridCol = IM_COL32(50, 50, 60, 40);
                for (float x = 0; x < canvasSize.x; x += 30.0f) {
                    drawList->AddLine(ImVec2(canvasPos.x + x, canvasPos.y), ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y), gridCol);
                }
                for (float y = 0; y < canvasSize.y; y += 30.0f) {
                    drawList->AddLine(ImVec2(canvasPos.x, canvasPos.y + y), ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + y), gridCol);
                }

                // Pozycja kuli (ruch wahadłowy w ósemkę na podstawie motionsProgress)
                if (motionsPlay) {
                    motionsProgress += deltaTime * 20.0f;
                    if (motionsProgress >= 100.0f) motionsProgress = 0.0f;
                }
                float rad = (motionsProgress / 100.0f) * 3.14159f * 2.0f;
                float ballX = canvasPos.x + (canvasSize.x / 2.0f) + (canvasSize.x / 3.0f) * std::cos(rad);
                float ballY = canvasPos.y + (canvasSize.y / 2.0f) + (canvasSize.y / 4.0f) * std::sin(rad * 2.0f);
                float radius = 30.0f + 8.0f * std::sin(rad * 3.0f);

                // Rysowanie kuli z neonową poświatą
                drawList->AddCircleFilled(ImVec2(ballX, ballY), radius + 15.0f, IM_COL32(139, 92, 246, 50));
                drawList->AddCircleFilled(ImVec2(ballX, ballY), radius + 5.0f, IM_COL32(236, 72, 153, 100));
                drawList->AddCircleFilled(ImVec2(ballX, ballY), radius, IM_COL32(255, 255, 255, 255));

                ImGui::EndChild();

                // Oś Czasu na dole
                ImGui::BeginChild("Timeline", ImVec2(0, 170), true);
                
                // Kontrolki Play/Pause
                if (ImGui::Button(motionsPlay ? "Pause" : "Play")) {
                    motionsPlay = !motionsPlay;
                }
                ImGui::SameLine();
                if (ImGui::Button("Stop")) {
                    motionsPlay = false;
                    motionsProgress = 0.0f;
                }
                ImGui::SameLine();
                ImGui::Text("Czas: %.1fs / 5.0s | Klatka: %d", (motionsProgress / 100.0f) * 5.0f, (int)(motionsProgress * 3.0f));

                ImGui::Separator();
                
                // Rysowanie toru osi czasu
                ImVec2 tlPos = ImGui::GetCursorScreenPos();
                float tlWidth = ImGui::GetContentRegionAvail().x - 40;
                ImDrawList* tlDraw = ImGui::GetWindowDrawList();
                
                // Tło ścieżki
                tlDraw->AddRectFilled(ImVec2(tlPos.x, tlPos.y + 10), ImVec2(tlPos.x + tlWidth, tlPos.y + 40), IM_COL32(30, 30, 40, 255), 4.0f);
                
                // Blok klipu animacji
                float clipStart = tlPos.x + tlWidth * 0.1f;
                float clipEnd = tlPos.x + tlWidth * 0.9f;
                tlDraw->AddRectFilled(ImVec2(clipStart, tlPos.y + 15), ImVec2(clipEnd, tlPos.y + 35), IM_COL32(139, 92, 246, 180), 4.0f);
                tlDraw->AddText(ImVec2(clipStart + 10, tlPos.y + 18), IM_COL32(255, 255, 255, 255), "Transformacja Ruchu");

                // Klatki kluczowe
                tlDraw->AddCircleFilled(ImVec2(clipStart, tlPos.y + 25), 5.0f, IM_COL32(255, 255, 255, 255));
                tlDraw->AddCircleFilled(ImVec2(clipEnd, tlPos.y + 25), 5.0f, IM_COL32(255, 255, 255, 255));

                // Czerwony suwak (Playhead)
                float playheadX = tlPos.x + tlWidth * (motionsProgress / 100.0f);
                tlDraw->AddLine(ImVec2(playheadX, tlPos.y), ImVec2(playheadX, tlPos.y + 50), IM_COL32(236, 72, 153, 255), 2.5f);
                tlDraw->AddCircleFilled(ImVec2(playheadX, tlPos.y), 6.0f, IM_COL32(236, 72, 153, 255));

                // Kliknięcie na osi czasu by przesunąć czas
                ImGui::SetCursorScreenPos(ImVec2(tlPos.x, tlPos.y + 10));
                ImGui::InvisibleButton("timeline_click", ImVec2(tlWidth, 30));
                if (ImGui::IsItemActive()) {
                    ImVec2 mousePos = ImGui::GetMousePos();
                    motionsProgress = std::clamp((mousePos.x - tlPos.x) / tlWidth * 100.0f, 0.0f, 100.0f);
                }

                ImGui::EndChild();

                ImGui::Columns(1);
            } 
            else if (activeSimAppId == "studio") {
                // SYMULATOR STUDIO (Montaż Wideo)
                ImGui::Columns(2, "studio_cols", true);
                ImGui::SetColumnWidth(0, 240);

                // Lewa kolumna: Media Pool
                ImGui::Text("Media Pool");
                ImGui::Separator();
                
                for (int i = 0; i < (int)studioClips.size(); ++i) {
                    bool isSelected = (studioSelectedClip == i);
                    if (ImGui::Selectable(studioClips[i].c_str(), isSelected)) {
                        studioSelectedClip = i;
                        ShowToast("Zaladowano klip: " + studioClips[i], "info");
                    }
                    // Mini podgląd koloru obok nazwy
                    ImGui::SameLine(180);
                    ImGui::ColorButton(studioClips[i].c_str(), studioClipColors[i], ImGuiColorEditFlags_NoTooltip, ImVec2(20, 15));
                }

                ImGui::NextColumn();

                // Prawa kolumna: Podgląd Monitora i Wielościeżkowa oś czasu
                ImVec2 space = ImGui::GetContentRegionAvail();
                float previewHeight = space.y - 180.0f;
                
                // Monitor
                ImGui::BeginChild("MonitorView", ImVec2(0, previewHeight), true);
                
                if (studioPlay) {
                    studioProgress += deltaTime * 10.0f;
                    if (studioProgress >= 100.0f) studioProgress = 0.0f;
                }

                // Zmiana aktywnego klipu w podglądzie zależna od playheada
                int activeVisualClip = studioSelectedClip;
                if (studioPlay) {
                    if (studioProgress < 33.3f) activeVisualClip = 0;
                    else if (studioProgress < 66.6f) activeVisualClip = 1;
                    else activeVisualClip = 2;
                }

                ImGui::Text("Monitor Programowy");
                
                // Renderowanie monitora z dynamicznym kolorem reprezentującym wideo
                ImVec2 monitorSize = ImVec2(previewHeight * 1.77f, previewHeight - 40.0f); // 16:9
                if (monitorSize.x > ImGui::GetContentRegionAvail().x) {
                    monitorSize.x = ImGui::GetContentRegionAvail().x;
                    monitorSize.y = monitorSize.x / 1.77f;
                }
                
                ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - monitorSize.x) / 2.0f);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, studioClipColors[activeVisualClip]);
                ImGui::BeginChild("MonitorDisplay", monitorSize, true);
                
                // Wyświetlanie nazwy renderowanego klipu na środku
                std::string frameText = "Odtwarzanie: " + studioClips[activeVisualClip];
                ImVec2 textSize = ImGui::CalcTextSize(frameText.c_str());
                ImGui::SetCursorPos(ImVec2((monitorSize.x - textSize.x) / 2.0f, (monitorSize.y - textSize.y) / 2.0f));
                ImGui::Text("%s", frameText.c_str());

                // Kod czasowy w lewym dolnym rogu
                float curTime = (studioProgress / 100.0f) * 10.0f;
                int min = (int)(curTime / 60.0f);
                int sec = (int)curTime % 60;
                int frames = (int)((curTime - std::floor(curTime)) * 25.0f);
                
                ImGui::SetCursorPos(ImVec2(10, monitorSize.y - 25));
                ImGui::Text("REC 00:%02d:%02d:%02d", min, sec, frames);

                ImGui::EndChild();
                ImGui::PopStyleColor();

                ImGui::EndChild();

                // Dolny panel: Oś czasu
                ImGui::BeginChild("StudioTimeline", ImVec2(0, 170), true);
                if (ImGui::Button(studioPlay ? "Pause" : "Play")) {
                    studioPlay = !studioPlay;
                }
                ImGui::SameLine();
                if (ImGui::Button("Stop")) {
                    studioPlay = false;
                    studioProgress = 0.0f;
                }
                ImGui::SameLine();
                ImGui::Text("Czas osi czasu: %.1fs / 10.0s", (studioProgress / 100.0f) * 10.0f);

                ImGui::Separator();

                // Wielościeżkowy widok klipów
                ImVec2 trackPos = ImGui::GetCursorScreenPos();
                float trackWidth = ImGui::GetContentRegionAvail().x - 40;
                ImDrawList* trackDraw = ImGui::GetWindowDrawList();

                // Rysowanie 2 ścieżek (Wideo i Audio)
                for (int t = 0; t < 2; ++t) {
                    float y = trackPos.y + 10 + t * 45;
                    trackDraw->AddRectFilled(ImVec2(trackPos.x, y), ImVec2(trackPos.x + trackWidth, y + 35), IM_COL32(35, 35, 45, 255), 4.0f);
                    
                    if (t == 0) {
                        // Klipy wideo
                        float w1 = trackWidth * 0.333f;
                        trackDraw->AddRectFilled(ImVec2(trackPos.x + 2, y + 2), ImVec2(trackPos.x + w1 - 2, y + 33), IM_COL32(6, 182, 212, 180), 3.0f);
                        trackDraw->AddRectFilled(ImVec2(trackPos.x + w1 + 2, y + 2), ImVec2(trackPos.x + 2 * w1 - 2, y + 33), IM_COL32(16, 185, 129, 180), 3.0f);
                        trackDraw->AddRectFilled(ImVec2(trackPos.x + 2 * w1 + 2, y + 2), ImVec2(trackPos.x + trackWidth - 2, y + 33), IM_COL32(245, 158, 11, 180), 3.0f);
                        
                        trackDraw->AddText(ImVec2(trackPos.x + 10, y + 10), IM_COL32(255,255,255,255), "beach.mp4");
                        trackDraw->AddText(ImVec2(trackPos.x + w1 + 10, y + 10), IM_COL32(255,255,255,255), "mountain.mp4");
                        trackDraw->AddText(ImVec2(trackPos.x + 2 * w1 + 10, y + 10), IM_COL32(255,255,255,255), "forest.mp4");
                    } else {
                        // Klip audio rozciągnięty na całość
                        trackDraw->AddRectFilled(ImVec2(trackPos.x + 2, y + 2), ImVec2(trackPos.x + trackWidth - 2, y + 33), IM_COL32(100, 100, 120, 100), 3.0f);
                        trackDraw->AddText(ImVec2(trackPos.x + 10, y + 10), IM_COL32(200, 200, 200, 255), "ambient_soundtrack.mp3 [Caly projekt]");
                    }
                }

                // Suwak odtwarzania
                float playheadX = trackPos.x + trackWidth * (studioProgress / 100.0f);
                trackDraw->AddLine(ImVec2(playheadX, trackPos.y), ImVec2(playheadX, trackPos.y + 100), IM_COL32(236, 72, 153, 255), 2.5f);
                trackDraw->AddCircleFilled(ImVec2(playheadX, trackPos.y), 6.0f, IM_COL32(236, 72, 153, 255));

                // Zmiana pozycji kliknięciem
                ImGui::SetCursorScreenPos(ImVec2(trackPos.x, trackPos.y + 10));
                ImGui::InvisibleButton("studio_track_click", ImVec2(trackWidth, 80));
                if (ImGui::IsItemActive()) {
                    ImVec2 mousePos = ImGui::GetMousePos();
                    studioProgress = std::clamp((mousePos.x - trackPos.x) / trackWidth * 100.0f, 0.0f, 100.0f);
                }

                ImGui::EndChild();

                ImGui::Columns(1);
            } 
            else if (activeSimAppId == "design") {
                // SYMULATOR DESIGN (Edytor Wektorowy + Drag & Drop!)
                ImGui::Columns(3, "design_cols", true);
                ImGui::SetColumnWidth(0, 180);
                ImGui::SetColumnWidth(1, 800);

                // 1. Lewa kolumna: Narzędzia i Warstwy
                ImGui::Text("Narzedzia");
                ImGui::Separator();
                
                if (ImGui::Button("Zaznacz", ImVec2(160, 30))) currentDesignTool = TOOL_SELECT;
                if (currentDesignTool == TOOL_SELECT) { ImGui::SameLine(); ImGui::TextColored(ImVec4(0, 1, 0, 1), "*"); }
                
                if (ImGui::Button("Prostokat", ImVec2(160, 30))) currentDesignTool = TOOL_RECT;
                if (currentDesignTool == TOOL_RECT) { ImGui::SameLine(); ImGui::TextColored(ImVec4(0, 1, 0, 1), "*"); }

                if (ImGui::Button("Kolo", ImVec2(160, 30))) currentDesignTool = TOOL_CIRCLE;
                if (currentDesignTool == TOOL_CIRCLE) { ImGui::SameLine(); ImGui::TextColored(ImVec4(0, 1, 0, 1), "*"); }

                if (ImGui::Button("Tekst", ImVec2(160, 30))) currentDesignTool = TOOL_TEXT;
                if (currentDesignTool == TOOL_TEXT) { ImGui::SameLine(); ImGui::TextColored(ImVec4(0, 1, 0, 1), "*"); }

                ImGui::Spacing();
                ImGui::Text("Lista Warstw");
                ImGui::Separator();
                for (int i = 0; i < (int)designShapes.size(); ++i) {
                    bool isSelected = (selectedShapeId == designShapes[i].id);
                    if (ImGui::Selectable(designShapes[i].name.c_str(), isSelected)) {
                        selectedShapeId = designShapes[i].id;
                        currentDesignTool = TOOL_SELECT; // automatyczny powrót do narzędzia wyboru
                    }
                    ImGui::SameLine(140);
                    // Przycisk usuwania
                    std::string deleteLabel = "X##" + std::to_string(designShapes[i].id);
                    if (ImGui::Button(deleteLabel.c_str())) {
                        if (selectedShapeId == designShapes[i].id) selectedShapeId = -1;
                        designShapes.erase(designShapes.begin() + i);
                        --i;
                    }
                }

                ImGui::NextColumn();

                // 2. Środkowa kolumna: Obszar Roboczy (Canvas)
                ImGui::Text("Plótno robocze (Artboard) - Kliknij aby dodac ksztalt / Przeciagnij by przesunac");
                ImGui::BeginChild("ArtboardCanvas", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);
                
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 artboardPos = ImGui::GetCursorScreenPos();
                ImVec2 artboardSize = ImGui::GetContentRegionAvail();

                // Rysowanie białego tła deski
                ImVec2 artboardCenter = ImVec2(artboardPos.x + artboardSize.x / 2.0f, artboardPos.y + artboardSize.y / 2.0f);
                ImVec2 size = ImVec2(500, 400);
                ImVec2 leftTop = ImVec2(artboardCenter.x - size.x / 2.0f, artboardCenter.y - size.y / 2.0f);
                ImVec2 rightBottom = ImVec2(artboardCenter.x + size.x / 2.0f, artboardCenter.y + size.y / 2.0f);
                
                // Rysowanie deski
                drawList->AddRectFilled(leftTop, rightBottom, IM_COL32(25, 25, 30, 255), 4.0f);
                drawList->AddRect(leftTop, rightBottom, IM_COL32(60, 60, 70, 255), 4.0f, 0, 1.5f);

                // Renderowanie kształtów
                for (auto& shape : designShapes) {
                    ImVec2 shapeMin = ImVec2(leftTop.x + shape.pos.x, leftTop.y + shape.pos.y);
                    ImVec2 shapeMax = ImVec2(shapeMin.x + shape.size.x, shapeMin.y + shape.size.y);
                    
                    ImU32 shapeCol = ImGui::ColorConvertFloat4ToU32(ImVec4(shape.color.x, shape.color.y, shape.color.z, shape.color.w * shape.opacity));
                    
                    // Rysowanie kształtu
                    if (shape.type == SHAPE_RECT) {
                        drawList->AddRectFilled(shapeMin, shapeMax, shapeCol, 6.0f);
                    } else if (shape.type == SHAPE_CIRCLE) {
                        drawList->AddCircleFilled(ImVec2(shapeMin.x + shape.size.x/2.0f, shapeMin.y + shape.size.y/2.0f), shape.size.x/2.0f, shapeCol, 36);
                    } else { // SHAPE_TEXT
                        drawList->AddText(shapeMin, shapeCol, "FRAME DESIGN C++");
                        // Obrys dla tekstu w celu przeciągania
                        drawList->AddRect(shapeMin, ImVec2(shapeMin.x + 120, shapeMin.y + 20), IM_COL32(255,255,255,50), 0, 0, 1.0f);
                    }

                    // Obwódka zaznaczenia
                    if (shape.id == selectedShapeId) {
                        drawList->AddRect(ImVec2(shapeMin.x - 3, shapeMin.y - 3), ImVec2(shapeMax.x + 3, shapeMax.y + 3), IM_COL32(139, 92, 246, 255), 0, 0, 2.0f);
                    }

                    // Interakcja przeciągania (Drag & Drop) na płótnie
                    if (currentDesignTool == TOOL_SELECT) {
                        ImGui::SetCursorScreenPos(shapeMin);
                        std::string invisibleBtnId = "drag_btn_" + std::to_string(shape.id);
                        ImGui::InvisibleButton(invisibleBtnId.c_str(), shape.size);
                        if (ImGui::IsItemClicked()) {
                            selectedShapeId = shape.id;
                        }
                        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                            ImVec2 mouseDragDelta = ImGui::GetIO().MouseDelta;
                            shape.pos.x += mouseDragDelta.x;
                            shape.pos.y += mouseDragDelta.y;

                            // Ograniczenie pozycji do rozmiaru deski
                            shape.pos.x = std::clamp(shape.pos.x, 0.0f, size.x - shape.size.x);
                            shape.pos.y = std::clamp(shape.pos.y, 0.0f, size.y - shape.size.y);
                        }
                    }
                }

                // Dodawanie nowych kształtów po kliknięciu na deskę
                if (currentDesignTool != TOOL_SELECT) {
                    ImGui::SetCursorScreenPos(leftTop);
                    ImGui::InvisibleButton("canvas_add_click", size);
                    if (ImGui::IsItemClicked()) {
                        ImVec2 clickPos = ImGui::GetMousePos();
                        ImVec2 localClick = ImVec2(clickPos.x - leftTop.x - 40, clickPos.y - leftTop.y - 40);

                        // Tworzenie kształtu
                        int newId = designShapes.empty() ? 1 : (std::max_element(designShapes.begin(), designShapes.end(), [](const VectorShape& a, const VectorShape& b){ return a.id < b.id; })->id + 1);
                        ShapeType newType = (currentDesignTool == TOOL_RECT) ? SHAPE_RECT : ((currentDesignTool == TOOL_CIRCLE) ? SHAPE_CIRCLE : SHAPE_TEXT);
                        std::string newName = (newType == SHAPE_RECT) ? "Prostokat " : ((newType == SHAPE_CIRCLE) ? "Kolo " : "Tekst ");
                        newName += std::to_string(newId);

                        ImVec4 col = ImVec4(0.54f, 0.36f, 0.96f, 1.0f);
                        if (currentTheme == THEME_CYAN) col = ImVec4(0.02f, 0.71f, 0.83f, 1.0f);
                        else if (currentTheme == THEME_GREEN) col = ImVec4(0.06f, 0.72f, 0.50f, 1.0f);

                        designShapes.push_back({ newId, newType, localClick, ImVec2(80, 80), col, 1.0f, newName });
                        selectedShapeId = newId;
                        currentDesignTool = TOOL_SELECT; // Powrót do narzędzia wyboru
                        ShowToast("Utworzono ksztalt: " + newName, "success");
                    }
                }

                ImGui::EndChild();

                ImGui::NextColumn();

                // 3. Prawa kolumna: Inspektor właściwości
                ImGui::Text("Inspektor Wlasciwosci");
                ImGui::Separator();
                
                VectorShape* selShape = nullptr;
                for (auto& shape : designShapes) {
                    if (shape.id == selectedShapeId) {
                        selShape = &shape;
                        break;
                    }
                }

                if (selShape) {
                    ImGui::Text("Nazwa: %s", selShape->name.c_str());
                    ImGui::Spacing();
                    
                    // Rozmiary
                    float w = selShape->size.x;
                    float h = selShape->size.y;
                    if (ImGui::DragFloat("Szerokosc (W)", &w, 1.0f, 10.0f, 400.0f, "%.0f")) {
                        selShape->size.x = w;
                    }
                    if (ImGui::DragFloat("Wysokosc (H)", &h, 1.0f, 10.0f, 400.0f, "%.0f")) {
                        selShape->size.y = h;
                    }

                    // Pozycje
                    ImGui::DragFloat("Poz X", &selShape->pos.x, 1.0f, 0.0f, 500.0f, "%.0f");
                    ImGui::DragFloat("Poz Y", &selShape->pos.y, 1.0f, 0.0f, 400.0f, "%.0f");

                    // Kolor
                    ImGui::ColorEdit3("Kolor wypelnienia", &selShape->color.x);
                    
                    // Przezroczystość
                    ImGui::SliderFloat("Opacitosc", &selShape->opacity, 0.0f, 1.0f, "%.2f");

                    ImGui::Spacing();
                    ImGui::Separator();
                    if (ImGui::Button("Usun Ksztalt", ImVec2(ImGui::GetContentRegionAvail().x, 30))) {
                        deleteShape(selShape->id, nullptr);
                    }
                } else {
                    ImGui::TextDisabled("Wybierz warstwe na liscie lub kliknij na ksztalt, aby edytowac właściwości.");
                }

                ImGui::Columns(1);
            }

            ImGui::End();
        } 
        else {
            // GŁÓWNY UKŁAD HUBA (Sidebar + Main Area)
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGui::Begin("FrameLabHubMain", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

            // Kolumny: 1. Sidebar (220px), 2. Main Content
            ImGui::Columns(2, "main_layout", true);
            ImGui::SetColumnWidth(0, 240);

            // ==========================================
            // COLUMN 0: SIDEBAR
            // ==========================================
            ImGui::BeginChild("SidebarArea", ImVec2(0, 0), false);
            
            // Logo
            ImGui::Spacing();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
            if (fontBold) ImGui::PushFont(fontBold);
            ImGui::TextColored(ImVec4(0.85f, 0.35f, 0.9f, 1.0f), "  F R A M E L A B  H U B");
            if (fontBold) ImGui::PopFont();
            ImGui::TextDisabled("       Centrum Kreatywnosci");
            ImGui::PopStyleVar();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Przyciski nawigacyjne
            auto DrawNavBtn = [](const std::string& label, Tab tabTarget, const std::string& icon) {
                bool isActive = (currentTab == tabTarget);
                if (isActive) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                }

                std::string btnText = icon + "  " + label;
                if (ImGui::Button(btnText.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 10, 40))) {
                    currentTab = tabTarget;
                }
                ImGui::PopStyleColor();
            };

            DrawNavBtn("Pulpit", TAB_DASHBOARD, "[#]");
            ImGui::Spacing();
            DrawNavBtn("Aplikacje", TAB_APPS, "[A]");
            ImGui::Spacing();
            DrawNavBtn("Zasoby", TAB_ASSETS, "[L]");
            ImGui::Spacing();
            DrawNavBtn("Ustawienia", TAB_SETTINGS, "[S]");

            // Dolna część panelu bocznego: Zużycie chmury i Profil
            float sidebarHeight = ImGui::GetContentRegionAvail().y;
            if (sidebarHeight > 180) {
                // Przesunięcie na sam dół
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + sidebarHeight - 120.0f);
                
                ImGui::Separator();
                ImGui::Spacing();
                
                // Profil użytkownika
                ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "Avatar  %s", profileName);
                if (isSubscribed) {
                    ImGui::TextColored(ImVec4(0.85f, 0.35f, 0.9f, 1.0f), "Plan Premium Creator");
                } else {
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Konto Darmowe");
                }
                ImGui::Spacing();
                if (ImGui::Button(isOfflineMode ? "Zaloguj sie" : "Wyloguj sie", ImVec2(ImGui::GetContentRegionAvail().x - 10, 24))) {
                    isLoggedIn = false;
                    isOfflineMode = false;
                    isSubscribed = false;
                    fbIdToken = "";
                    fbLocalId = "";
                    DeleteLocalSession();
                    snprintf(profileName, sizeof(profileName), "Aleksandra Kowalska");
                    snprintf(profileEmail, sizeof(profileEmail), "aleksandra@framelab.io");
                    ShowToast("Wylogowano.", "info");
                }
            }

            ImGui::EndChild();

            // ==========================================
            // COLUMN 1: MAIN AREA
            // ==========================================
            ImGui::NextColumn();

            // Nagłówek (Header) na samej górze
            ImGui::BeginChild("TopHeader", ImVec2(0, 60), false);
            ImGui::Spacing();
            
            // Wyszukiwarka
            static char searchBuf[128] = "";
            ImGui::SetNextItemWidth(350);
            if (ImGui::InputTextWithHint("##GlobalSearch", "Przeszukaj aplikacje, projekty...", searchBuf, IM_ARRAYSIZE(searchBuf))) {
                if (currentTab != TAB_APPS && currentTab != TAB_DASHBOARD) {
                    currentTab = TAB_APPS;
                }
            }

            // Powiadomienia w prawym górnym rogu
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 220);
            
            if (ImGui::Button("Powiadomienia (3)")) {
                notifDropdownOpen = !notifDropdownOpen;
            }
            if (notifDropdownOpen) {
                ImGui::OpenPopup("notif_popup");
            }
            if (ImGui::BeginPopup("notif_popup")) {
                ImGui::Text("Ostatnie powiadomienia");
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1,1,0,1), "* Aktualizacja FrameLab Motions");
                ImGui::TextDisabled("5 minut temu");
                ImGui::TextColored(ImVec4(0,1,0,1), "* Wyrenderowano wideo");
                ImGui::TextDisabled("1 godzina temu");
                ImGui::Text("* Pobrano nowa czcionke: Outfit");
                ImGui::TextDisabled("Wczoraj");
                ImGui::EndPopup();
            } else {
                notifDropdownOpen = false;
            }

            ImGui::SameLine();
            if (ImGui::Button("Nowy Projekt")) {
                // Szybki dialog tworzenia projektu
                ImGui::OpenPopup("new_project_popup");
            }
            
            if (ImGui::BeginPopupModal("new_project_popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Utwórz nowy projekt kreatywny");
                ImGui::Separator();
                static char newProjName[128] = "Nienazwany Projekt";
                ImGui::InputText("Nazwa pliku", newProjName, IM_ARRAYSIZE(newProjName));
                
                static int appSelectIndex = 0;
                const char* selectAppsList[] = { "FrameLab Motions", "FrameLab Studio", "FrameLab Design" };
                ImGui::Combo("Aplikacja docelowa", &appSelectIndex, selectAppsList, IM_ARRAYSIZE(selectAppsList));

                ImGui::Spacing();
                if (ImGui::Button("Stwórz i Otwórz", ImVec2(140, 30))) {
                    std::string appKeys[] = { "motions", "studio", "design" };
                    int newId = projects.size() + 1;
                    
                    std::string nameStr(newProjName);
                    projects.push_back({ newId, nameStr, appKeys[appSelectIndex], "1.0 MB", "Przed chwila" });
                    
                    ImGui::CloseCurrentPopup();
                    LaunchAppSim(appKeys[appSelectIndex], nameStr);
                }
                ImGui::SameLine();
                if (ImGui::Button("Anuluj", ImVec2(100, 30))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::EndChild();
            ImGui::Separator();

            // Dynamiczna zawartość na podstawie zakładki
            ImGui::BeginChild("DynamicBody", ImVec2(0, 0), false);
            
            if (currentTab == TAB_DASHBOARD) {
                // ==========================================
                // ZAKŁADKA: DASHBOARD (PULPIT)
                // ==========================================
                
                // Powitalny banner (stylizowany)
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.10f, 0.18f, 1.0f));
                ImGui::BeginChild("WelcomeBanner", ImVec2(0, 100), true);
                if (fontBold) ImGui::PushFont(fontBold);
                ImGui::TextColored(ImVec4(0.9f, 0.8f, 1.0f, 1.0f), "Witaj z powrotem, %s!", profileName);
                if (fontBold) ImGui::PopFont();
                ImGui::TextDisabled("Twoje studio kreatywne jest gotowe do pracy. Uwolnij swoja kreatywnosc.");
                
                // Zegar
                time_t t = time(nullptr);
                tm* now = localtime(&t);
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 150);
                ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_CheckMark], "Zegar: %02d:%02d:%02d", now->tm_hour, now->tm_min, now->tm_sec);
                
                ImGui::EndChild();
                ImGui::PopStyleColor();

                ImGui::Spacing();
                ImGui::Columns(2, "dashboard_cols", false);
                ImGui::SetColumnWidth(0, 480);

                // Kolumna 0: Kołowy wykres zasobów i statystyki
                ImGui::BeginChild("SubscriptionWidget", ImVec2(0, 180), true);
                if (fontBold) ImGui::PushFont(fontBold);
                ImGui::Text("Subskrypcja FrameLab Premium");
                if (fontBold) ImGui::PopFont();
                ImGui::Separator();
                ImGui::Spacing();

                if (isSubscribed) {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "[ AKTYWNA ] Masz pelny dostep do pakietu Premium!");
                    ImGui::Spacing();
                    ImGui::BulletText("Odblokowane wszystkie narzedzia (Motions, Studio, Design)");
                    ImGui::BulletText("Nieograniczony eksport projektow");
                    ImGui::BulletText("Priorytetowe renderowanie GPU");
                    ImGui::Spacing();
                    ImGui::TextDisabled("Dziekujemy, ze jestes z nami!");
                } else {
                    ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.1f, 1.0f), "Uzyskaj dostep do wszystkich funkcji premium.");
                    ImGui::Spacing();
                    
                    if (ImGui::Button("KUP SUBSKRYPCJE PREMIUM ($9.99/mc)", ImVec2(ImGui::GetContentRegionAvail().x - 10, 40))) {
                        if (isOfflineMode) {
                            isSubscribed = true;
                            ShowToast("Kupiono subskrypcje (Tryb Offline)", "success");
                        } else if (!fbLocalId.empty()) {
                            std::string dbUrl = FIREBASE_DATABASE_URL + "users/" + fbLocalId + ".json?auth=" + fbIdToken;
                            std::string dbPayload = "{\"subscribed\":true}";
                            PerformHTTPSRequest("PATCH", dbUrl, dbPayload);
                            isSubscribed = true;
                            ShowToast("Subskrypcja zakupiona pomyslnie!", "success");
                        } else {
                            ShowToast("Musisz byc zalogowany, aby kupic subskrypcje.", "warning");
                        }
                    }
                    ImGui::Spacing();
                    ImGui::TextDisabled("* Odblokuj os czasu, wiecej warstw i eksport projektów.");
                }

                ImGui::EndChild();

                // Kolumna 1: Szybki Start
                ImGui::NextColumn();
                ImGui::BeginChild("QuickLaunch", ImVec2(0, 180), true);
                if (fontBold) ImGui::PushFont(fontBold);
                ImGui::Text("Szybki start aplikacji");
                if (fontBold) ImGui::PopFont();
                ImGui::Separator();

                for (const auto& app : apps) {
                    if (app.status == STATUS_INSTALLED) {
                        ImGui::PushStyleColor(ImGuiCol_Button, app.color);
                        std::string launchLabel = "Otwórz " + app.name;
                        if (ImGui::Button(launchLabel.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 10, 32))) {
                            LaunchAppSim(app.id);
                        }
                        ImGui::PopStyleColor();
                        ImGui::Spacing();
                    }
                }
                ImGui::EndChild();

                ImGui::Columns(1);
                ImGui::Spacing();

                // Ostatnio edytowane projekty
                if (fontBold) ImGui::PushFont(fontBold);
                ImGui::Text("Ostatnio edytowane projekty");
                if (fontBold) ImGui::PopFont();
                ImGui::Separator();
                
                ImGui::Columns(3, "projects_grid", false);
                for (const auto& proj : projects) {
                    ImGui::BeginChild(proj.name.c_str(), ImVec2(0, 140), true);
                    
                    // Dobranie koloru w zależności od aplikacji
                    ImVec4 appColor = ImVec4(1,1,1,1);
                    std::string appName = "";
                    for (const auto& app : apps) {
                        if (app.id == proj.appId) {
                            appColor = app.color;
                            appName = app.name;
                            break;
                        }
                    }

                    ImGui::TextColored(appColor, "%s", proj.name.c_str());
                    ImGui::TextDisabled("%s", appName.c_str());
                    ImGui::Text("Rozmiar: %s", proj.size.c_str());
                    ImGui::TextDisabled("Mod: %s", proj.lastModified.c_str());
                    
                    ImGui::Spacing();
                    std::string openProjBtnLabel = "Otwórz##" + std::to_string(proj.id);
                    if (ImGui::Button(openProjBtnLabel.c_str(), ImVec2(80, 24))) {
                        LaunchAppSim(proj.appId, proj.name);
                    }
                    ImGui::SameLine();
                    std::string deleteProjBtnLabel = "Usuń##" + std::to_string(proj.id);
                    if (ImGui::Button(deleteProjBtnLabel.c_str(), ImVec2(80, 24))) {
                        // Usuń projekt
                        int pid = proj.id;
                        projects.erase(std::remove_if(projects.begin(), projects.end(), [pid](const Project& p){ return p.id == pid; }), projects.end());
                        ShowToast("Usunięto projekt", "warning");
                        break;
                    }

                    ImGui::EndChild();
                    ImGui::NextColumn();
                }
                ImGui::Columns(1);
            } 
            else if (currentTab == TAB_APPS) {
                // ==========================================
                // ZAKŁADKA: APPLIKACJE (APPS)
                // ==========================================
                if (fontBold) ImGui::PushFont(fontBold);
                ImGui::Text("Wszystkie Aplikacje Kreatywne");
                if (fontBold) ImGui::PopFont();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Columns(2, "apps_grid_system", false);
                ImGui::SetColumnWidth(0, 480);
                
                for (auto& app : apps) {
                    // Wyszukiwanie globalne (filtr tekstowy)
                    if (searchBuf[0] != '\0') {
                        std::string sText(searchBuf);
                        std::transform(sText.begin(), sText.end(), sText.begin(), ::tolower);
                        std::string appNameLower = app.name;
                        std::transform(appNameLower.begin(), appNameLower.end(), appNameLower.begin(), ::tolower);
                        if (appNameLower.find(sText) == std::string::npos) {
                            continue;
                        }
                    }

                    ImGui::BeginChild(app.id.c_str(), ImVec2(0, 160), true);
                    
                    // Nazwa i ikona
                    ImGui::TextColored(app.color, "[ %s ]", app.iconText.c_str());
                    ImGui::SameLine();
                    ImGui::Text("%s", app.name.c_str());
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 120);
                    ImGui::TextDisabled("%s", app.categoryName.c_str());

                    ImGui::TextDisabled("%s", app.tagline.c_str());
                    ImGui::TextWrapped("%s", app.description.c_str());
                    
                    ImGui::Spacing();
                    ImGui::TextDisabled("Wersja: %s | Rozmiar: %s", app.version.c_str(), app.size.c_str());
                    
                    // Pasek pobierania / instalacji
                    if (app.isInstalling) {
                        std::string progressStr = "Pobieranie... " + std::to_string((int)(app.progress * 100)) + "%";
                        ImGui::ProgressBar(app.progress, ImVec2(240, 20), progressStr.c_str());
                    } else {
                        // Przyciski akcji
                        if (app.status == STATUS_INSTALLED) {
                            if (ImGui::Button("Uruchom Program", ImVec2(140, 25))) {
                                LaunchAppSim(app.id);
                            }
                        } else if (app.status == STATUS_UPDATE) {
                            if (ImGui::Button("Aktualizuj", ImVec2(120, 25))) {
                                app.isInstalling = true;
                                app.progress = 0.0f;
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Uruchom (Stara)", ImVec2(140, 25))) {
                                LaunchAppSim(app.id);
                            }
                        } else if (app.status == STATUS_NOT_INSTALLED) {
                            if (ImGui::Button("Zainstaluj", ImVec2(120, 25))) {
                                app.isInstalling = true;
                                app.progress = 0.0f;
                            }
                        } else { // STATUS_TRIAL
                            if (ImGui::Button("Wyprobuj wersje testowa", ImVec2(180, 25))) {
                                app.isInstalling = true;
                                app.progress = 0.0f;
                            }
                        }
                    }

                    ImGui::EndChild();
                    ImGui::NextColumn();
                }
                ImGui::Columns(1);
            } 
            else if (currentTab == TAB_ASSETS) {
                // ==========================================
                // ZAKŁADKA: ZASOBY (ASSETS)
                // ==========================================
                if (fontBold) ImGui::PushFont(fontBold);
                ImGui::Text("Biblioteka Lokalnych Zasobów");
                if (fontBold) ImGui::PopFont();
                ImGui::Separator();
                ImGui::Spacing();

                // Strefa Dropzone
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
                if (ImGui::Button("Kliknij tutaj, aby zaimportowac plik z dysku (Symulacja Dropzone)", ImVec2(ImGui::GetContentRegionAvail().x, 60))) {
                    // Symulacja zaimportowania
                    int newId = assets.size() + 1;
                    std::string newAssetName = "render_final_img_" + std::to_string(newId) + ".png";
                    assets.push_back({ newId, newAssetName, "image", "5.1 MB", "Przed chwila", ImVec4(0.1f, 0.6f, 0.7f, 1.0f) });
                    ShowToast("Zaimportowano plik: " + newAssetName, "success");
                }
                ImGui::PopStyleVar();
                ImGui::Spacing();

                // Siatka zasobów
                ImGui::Columns(4, "assets_grid_sys", false);
                for (const auto& asset : assets) {
                    ImGui::BeginChild(std::to_string(asset.id).c_str(), ImVec2(0, 160), true);

                    // Podgląd zastępczy koloru
                    ImGui::ColorButton(asset.name.c_str(), asset.color, ImGuiColorEditFlags_NoTooltip, ImVec2(ImGui::GetContentRegionAvail().x, 60));
                    
                    ImGui::Spacing();
                    ImGui::TextWrapped("%s", asset.name.c_str());
                    ImGui::TextDisabled("Typ: %s", asset.type.c_str());
                    ImGui::TextDisabled("S: %s | %s", asset.size.c_str(), asset.date.c_str());

                    ImGui::EndChild();
                    ImGui::NextColumn();
                }
                ImGui::Columns(1);
            } 
            else if (currentTab == TAB_SETTINGS) {
                // ==========================================
                // ZAKŁADKA: USTAWIENIA (SETTINGS)
                // ==========================================
                if (fontBold) ImGui::PushFont(fontBold);
                ImGui::Text("Ustawienia Huba i Profilu");
                if (fontBold) ImGui::PopFont();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Columns(2, "settings_cols_sys", false);
                ImGui::SetColumnWidth(0, 480);

                // Profil
                ImGui::BeginChild("ProfileSettings", ImVec2(0, 220), true);
                ImGui::Text("Mój Profil Kreatora");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::InputText("Nazwa użytkownika", profileName, IM_ARRAYSIZE(profileName));
                ImGui::InputText("Adres e-mail", profileEmail, IM_ARRAYSIZE(profileEmail));

                ImGui::Spacing();
                if (ImGui::Button("Zapisz Zmiany", ImVec2(140, 30))) {
                    ShowToast("Profil zaktualizowany!", "success");
                }
                ImGui::EndChild();

                ImGui::NextColumn();

                // Personalizacja
                ImGui::BeginChild("ThemeSettings", ImVec2(0, 220), true);
                ImGui::Text("Stylistyka & Motyw");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Wybierz schemat kolorów neonowych:");
                
                int themeInt = (int)currentTheme;
                if (ImGui::RadioButton("Fioletowy i Rozowy (Motyw Motions)", &themeInt, 0)) {
                    currentTheme = THEME_PURPLE;
                    ApplyTheme(currentTheme);
                    ShowToast("Ustawiono motyw fioletowy", "success");
                }
                if (ImGui::RadioButton("Cyjanowy i Niebieski (Motyw Studio)", &themeInt, 1)) {
                    currentTheme = THEME_CYAN;
                    ApplyTheme(currentTheme);
                    ShowToast("Ustawiono motyw cyjanowy", "success");
                }
                if (ImGui::RadioButton("Szmaragdowy i Zielony (Motyw Audio)", &themeInt, 2)) {
                    currentTheme = THEME_GREEN;
                    ApplyTheme(currentTheme);
                    ShowToast("Ustawiono motyw zielony", "success");
                }
                if (ImGui::RadioButton("Bursztynowy i Czerwony (Motyw Effects)", &themeInt, 3)) {
                    currentTheme = THEME_ORANGE;
                    ApplyTheme(currentTheme);
                    ShowToast("Ustawiono motyw bursztynowy", "success");
                }

                ImGui::EndChild();

                ImGui::Columns(1);
                ImGui::Spacing();

                // Preferencje
                ImGui::BeginChild("SystemPreferences", ImVec2(0, 180), true);
                ImGui::Text("Preferencje Systemowe");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Checkbox("Automatycznie pobieraj i instaluj aktualizacje", &optAutoUpdate);
                ImGui::Checkbox("Wlacz akceleracje GPU dla symulatorów", &optGpuAccel);
                ImGui::Checkbox("Uruchamiaj FrameLab Hub przy starcie systemu", &optRunAtStartup);

                ImGui::EndChild();
            }

            ImGui::EndChild();
            ImGui::End();
        }
        } // Koniec bloku else dla uwierzytelniania

        // Koniec klatki ImGui, rysowanie
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.04f, 0.04f, 0.05f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Czyszczenie
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

void deleteShape(int id, void* event) {
    designShapes.erase(std::remove_if(designShapes.begin(), designShapes.end(), [id](const VectorShape& s){ return s.id == id; }), designShapes.end());
    if (selectedShapeId == id) selectedShapeId = -1;
    ShowToast("Usunieto ksztalt", "warning");
}
