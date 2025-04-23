#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib> // Für random()

// *** WIFI- Zugangsdaten ***
const char* ssid = "EngineTestWeb";
const char* password = "abcdefgh";

// *** PIN-DEFINITIONEN ***
const int NUM_MAGNETS = 7; // Nur die steuerbaren Magnete 0-6
const int STATIC_MAGNET_INDEX = 7; // Index für den statischen Magnet 8
const int MAGNET_PINS[NUM_MAGNETS][2] = {{16, 17}, {18, 19}, {21, 22}, {23, 4}, {2, 15}, {13, 12}, {14, 27}};
const int STATIC_MAGNET_PINS[2] = {5, 3}; // Pins für den statischen Magnet 8
const uint8_t MAGNET_CHANNELS[NUM_MAGNETS][2] = {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9}, {10, 11}, {12, 13}};

// L298N Treiber 1 - Steuert Magnet 1 & 4
const int IN1_L298N_1_PIN_A = 21; // Richtung A für Magnet 1
const int IN2_L298N_1_PIN_A = 22; // Richtung B für Magnet 1
const int IN3_L298N_1_PIN_B = 23; // Richtung A für Magnet 4
const int IN4_L298N_1_PIN_B = 4;   // Richtung B für Magnet 4
// const int EN_L298N_1_PIN = -1;   // Enable für Treiber 1 (NICHT VERWENDET)

// L298N Treiber 2 - Steuert Magnet 2 & 5
const int IN1_L298N_2_PIN_A = 2;   // Richtung A für Magnet 2
const int IN2_L298N_2_PIN_A = 15; // Richtung B für Magnet 2
const int IN3_L298N_2_PIN_B = 13; // Richtung A für Magnet 5
const int IN4_L298N_2_PIN_B = 12; // Richtung B für Magnet 5
// const int EN_L298N_2_PIN = -1;   // Enable für Treiber 2 (NICHT VERWENDET)

// L298N Treiber 3 - Steuert Magnet 3 & 6
const int IN1_L298N_3_PIN_A = 14; // Richtung A für Magnet 3
const int IN2_L298N_3_PIN_A = 27; // Richtung B für Magnet 3
const int IN3_L298N_3_PIN_B = 16; // Richtung A für Magnet 6
const int IN4_L298N_3_PIN_B = 17; // Richtung B für Magnet 6
// const int EN_L298N_3_PIN = -1;   // Enable für Treiber 3 (NICHT VERWENDET)

// L298N Treiber 4 - Steuert Magnet 0 & 7 (Magnet 8 wird separat behandelt)
const int IN1_L298N_4_PIN_A = 18; // Richtung A für Magnet 0
const int IN2_L298N_4_PIN_A = 19; // Richtung B für Magnet 0
const int IN3_L298N_4_PIN_B = 5;   // Richtung A für Magnet 7 (Magnet 8)
const int IN4_L298N_4_PIN_B = 3;   // Richtung B für Magnet 7 (Magnet 8)
// const int EN_L298N_4_PIN = -1;   // Enable für Treiber 4 (NICHT VERWENDET)

// *** PWM PIN DEFINITIONS (NICHT VERWENDET, DA KEINE ENABLE-PINS VORHANDEN) ***
const int magnetEnablePins[NUM_MAGNETS + 1] = {-1, -1, -1, -1, -1, -1, -1, -1};

// Richtungspins für die L298N-Treiber (für die steuerbaren Magnete 0-6)
const int magnetDirectionAPins[NUM_MAGNETS] = {
    IN1_L298N_4_PIN_A, IN1_L298N_1_PIN_A, IN1_L298N_2_PIN_A, IN1_L298N_3_PIN_A,
    IN3_L298N_1_PIN_B, IN3_L298N_2_PIN_B, IN3_L298N_3_PIN_B
};

const int magnetDirectionBPins[NUM_MAGNETS] = {
    IN2_L298N_4_PIN_A, IN2_L298N_1_PIN_A, IN2_L298N_2_PIN_A, IN2_L298N_3_PIN_A,
    IN4_L298N_1_PIN_B, IN4_L298N_2_PIN_B, IN4_L298N_3_PIN_B
};

// Richtungspins für den statischen Magnet 8
const int staticMagnetDirectionAPin = IN3_L298N_4_PIN_B;
const int staticMagnetDirectionBPin = IN4_L298N_4_PIN_B;

const int MAGNET_PIN_8_PWM_A = 5; // Richtung A für Magnet 7 (Magnet 8)
const int MAGNET_PIN_8_PWM_B = 3; // Richtung B für Magnet 7 (Magnet 8)

const int staticMagnetChannels[2] = {MAGNET_PIN_8_PWM_A, MAGNET_PIN_8_PWM_B};

// *** PWM PIN FÜR ENABLE (NICHT VERWENDET) ***
const int PWM_ENABLE_PIN_MAGNETS[4] = {-1, -1, -1, -1};

// *** MAGNET PWM ZU TREIBER ZUORDNUNG (NICHT VERWENDET) ***
// Ordnet Magnetindex dem entsprechenden Treiber-Enable-Pin zu
const int magnetToEnablePin[NUM_MAGNETS + 1] = {-1, -1, -1, -1, -1, -1, -1, -1};

// *** JOYSTICK PIN DEFINITIONS ***
const int joystickXPins[] = {36, 39}; // Verwenden nur einen Pin pro Achse für einfache Analogwerte
const int joystickYPins[] = {34, 35}; // Verwenden nur einen Pin pro Achse für einfache Analogwerte
const int joystickZPins[] = {32, 33}; // Wieder aktiviert
const int joystickWPins[] = {25, 26}; // Wieder aktiviert

// *** PWM CONFIGURATION ***
const int PWM_RESOLUTION = 8; // 8-bit Auflösung (0-255)
int pwmFrequency = 120;

// *** OVERRIDE MODES ***
enum class OverrideModus : int {
    KEIN_OVERRIDE = -1,
    ZYKLISCHER_OVERRIDE,
    VORRUECK_OVERRIDE,
    RUHIG_OVERRIDE,
    PULSIEREND_OVERRIDE,
    WELLEN_OVERRIDE,
    ZUFALLS_OVERRIDE,
    FADING_OVERRIDE,
    SEQUENZ_OVERRIDE,
    ALLE_AN_AUS_OVERRIDE,
    HALBE_AN_OVERRIDE,
    MUSTER_OVERRIDE
};

OverrideModus aktiverOverrideModus = OverrideModus::KEIN_OVERRIDE;
int overridePWM = 200;
int globalePWMIntensitaet = 255;
unsigned long grundMusterIntervall = 1000;
unsigned long pulsIntervall = 500;
unsigned long wellenGeschwindigkeit = 200;
unsigned long zufallsIntervallMin = 100;
unsigned long zufallsIntervallMax = 1000;
unsigned long fadingDauer = 1000;
int aktiveSequenz = 0;
unsigned long sequenzIntervall = 500;
int halbeAnModus = 0;
int alleAnAusZustand = 255;
bool joystickSteuertZyklischeGeschwindigkeit = false;
String joystickZyklischeGeschwindigkeitsAchse = "x";
long joystickZyklischeGeschwindigkeitMin = 100;
long joystickZyklischeGeschwindigkeitMax = 5000;
float joystickTotbereichOverride = 0.1f;
float polaritaetsSchwellePositiv = 0.2f;
float polaritaetsSchwelleNegativ = -0.2f;
int halbeAnIntervall = 500;
bool joystickAktiv = false;
bool joystickXInvertieren = false;
bool joystickYInvertieren = false;
int joystickTotbereich = 10;
float joystickSkalierung = 1.0f;
int joystickBasisGeschwindigkeit = 100;
String joystickGeschwindigkeitsAchse = "x";
unsigned long vorrueckIntervall = 500;
int aktuellerOverridePWM = 200;
bool joystickSteuertPWM = false;
bool joystickSteuertIntensitaetZ = false;
bool joystickSteuertGeschwindigkeitW = false;
int aktuellesJoystickMusterIndex = 0;
unsigned long letztesJoystickMusterZeit = 0;
long joystickMusterIntervall = 500;
// *** GLOBALE PWM-WERTE ***
int grundPWM = 150; // Standardwert für Grund-PWM
int joystickMaxIntensitaet = 255;
// *** MODUS-VERWALTUNG ***
enum SteuerungModus {
    GRUNDMUSTER_PWM,
    JOYSTICK_MODUS,
};

SteuerungModus aktuellerModus = GRUNDMUSTER_PWM; // Standardmäßig Grundmuster-Modus

struct PWMMuster {
    String name;
    std::vector<int> intensitaeten; // Hier MUSS 'intensitaeten' stehen
};

std::vector<PWMMuster> grundPWMMuster;
std::vector<PWMMuster> maxIntensivPWMMuster;
std::vector<PWMMuster> joystickPWMMuster;
std::vector<int> halbeAnMuster;

// *** ZEITMESSUNGS-VARIABLEN ***
unsigned long aktuelleZeit = 0;
unsigned long letzteMusterAenderungZeit = 0;
unsigned long letztePulsZeit = 0;
bool pulsZustand = false;
unsigned long fadingStartZeit = 0;
int fadingRichtung[NUM_MAGNETS + 1] = {1};
unsigned long letzteWellenZeit = 0;
unsigned long letzteZufallsAenderungZeit = 0;
unsigned long letzteSequenzZeit = 0;
int sequenzSchritt = 0;
int vorrueckIndex = 0;
unsigned long letzteVorrueckZeit = 0;
int magnetIntensitaeten[NUM_MAGNETS + 1] = {0};

// *** WEB SERVER ***
AsyncWebServer server(80);
Preferences preferences; // Global deklariert

// *** FUNKTIONSDEKLARATIONEN ***
void setMagnetPWM(int magnetIndex, int intensitaet);
void setMagnetRichtung(int magnetIndex, int richtung);
int liesJoystick(const int* pins);
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
String getStatusJson();
void savePatterns();
void loadPatterns();
void handleSet(AsyncWebServerRequest *request);
void updateMagnets();
void handleSaveAll(AsyncWebServerRequest *request); // Deklaration der neuen Handler-Funktion
void modusUmschalten(SteuerungModus neuerModus);

void setMagnetPWM(int magnetIndex, int intensitaet) {
    if (magnetIndex >= 0 && magnetIndex < NUM_MAGNETS) {
        ledcWrite(MAGNET_CHANNELS[magnetIndex][0], intensitaet);
        ledcWrite(MAGNET_CHANNELS[magnetIndex][1], intensitaet);
    } else if (magnetIndex == STATIC_MAGNET_INDEX) {
        ledcWrite(staticMagnetChannels[0], intensitaet);
        ledcWrite(staticMagnetChannels[1], intensitaet);
    }
}

void setMagnetRichtung(int magnetIndex, int richtung) {
    if (magnetIndex >= 0 && magnetIndex < NUM_MAGNETS) {
        digitalWrite(magnetDirectionAPins[magnetIndex], (richtung > 0) ? HIGH : LOW);
        digitalWrite(magnetDirectionBPins[magnetIndex], (richtung > 0) ? LOW : HIGH);
    } else if (magnetIndex == STATIC_MAGNET_INDEX) {
        digitalWrite(staticMagnetDirectionAPin, (richtung > 0) ? HIGH : LOW);
        digitalWrite(staticMagnetDirectionBPin, (richtung > 0) ? LOW : HIGH);
    }
}

int liesJoystick(const int* pins) {
    if (pins[0] != -1) {
        return analogRead(pins[0]);
    }
    return 0;
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

String getStatusJson() {
    DynamicJsonDocument doc(2048);
    doc["override"] = static_cast<int>(aktiverOverrideModus); // Cast to int for JSON
    doc["overridepwm"] = overridePWM;
    doc["pwmfrequency"] = pwmFrequency;
    doc["grundpatterninterval"] = grundMusterIntervall;
    doc["pulsintervall"] = pulsIntervall;
    doc["welle"] = wellenGeschwindigkeit;
    doc["zufallmin"] = zufallsIntervallMin;
    doc["zufallmax"] = zufallsIntervallMax;
    doc["fade"] = fadingDauer;
    doc["sequenz"] = aktiveSequenz;
    doc["sequenzintervall"] = sequenzIntervall;
    doc["joytickaktiv"] = joystickAktiv;
    doc["joytickinvertx"] = joystickXInvertieren;
    doc["joytickinverty"] = joystickYInvertieren;
    doc["joyticktotbereich"] = joystickTotbereich;
    doc["joytickskalierung"] = joystickSkalierung;
    doc["joytickbasisgeschwindigkeit"] = joystickBasisGeschwindigkeit;
    doc["joytickachsen"] = joystickGeschwindigkeitsAchse.c_str();
    doc["joystickCyclicSpeed"] = joystickSteuertZyklischeGeschwindigkeit;
    doc["joystickCyclicSpeedAxis"] = joystickZyklischeGeschwindigkeitsAchse.c_str();
    doc["joystickCyclicSpeedMin"] = joystickZyklischeGeschwindigkeitMin;
    doc["joystickCyclicSpeedMax"] = joystickZyklischeGeschwindigkeitMax;
    doc["joystickTotzoneOverride"] = joystickTotbereichOverride;
    doc["polarityThresholdPositive"] = polaritaetsSchwellePositiv;
    doc["polarityThresholdNegative"] = polaritaetsSchwelleNegativ;
    doc["halbeanintervall"] = halbeAnIntervall;
    doc["joystickPWM"] = joystickSteuertPWM;
    doc["joystickIntensityZ"] = joystickSteuertIntensitaetZ;
    doc["joystickSpeedW"] = joystickSteuertGeschwindigkeitW;
    doc["aktuellerModus"] = (aktuellerModus == GRUNDMUSTER_PWM) ? "grundmuster" : "joystick";
    return doc.as<String>();
}

void handleSaveAll(AsyncWebServerRequest *request) {
    Serial.println("Rufe savePatterns zum Speichern auf...");
    savePatterns(); // Rufe die Funktion auf, die alle Einstellungen und Muster speichert
    request->send(200, "text/plain", "Einstellungen und Muster gespeichert!");
    Serial.println("Einstellungen und Muster gespeichert.");
    request->redirect("/"); // Optional: Zurück zur Hauptseite leiten
}

void loadSettings() {
    Serial.println("Lade gespeicherte Einstellungen...");
    preferences.begin("magnet_settings");

    aktiverOverrideModus = static_cast<OverrideModus>(preferences.getInt("overrideModus", static_cast<int>(OverrideModus::KEIN_OVERRIDE)));
    overridePWM = preferences.getInt("overridePWM", 200);
    globalePWMIntensitaet = preferences.getInt("globalePWMIntensitaet", 255);
    grundMusterIntervall = preferences.getLong("grundMusterIntervall", 1000);
    pulsIntervall = preferences.getLong("pulsIntervall", 500);
    wellenGeschwindigkeit = preferences.getLong("wellenGeschwindigkeit", 200);
    zufallsIntervallMin = preferences.getLong("zufallsIntervallMin", 100);
    zufallsIntervallMax = preferences.getLong("zufallsIntervallMax", 1000);
    fadingDauer = preferences.getLong("fadingDauer", 1000);
    aktiveSequenz = preferences.getInt("aktiveSequenz", 0);
    sequenzIntervall = preferences.getLong("sequenzIntervall", 500);
    halbeAnModus = preferences.getInt("halbeAnModus", 0);
    alleAnAusZustand = preferences.getInt("alleAnAusZustand", 255);
    joystickSteuertZyklischeGeschwindigkeit = preferences.getBool("joystickSteuertZyklischeGeschwindigkeit", false);
    joystickZyklischeGeschwindigkeitsAchse = preferences.getString("joystickZyklischeGeschwindigkeitsAchse", "x");
    joystickZyklischeGeschwindigkeitMin = preferences.getLong("joystickZyklischeGeschwindigkeitMin", 100);
    joystickZyklischeGeschwindigkeitMax = preferences.getLong("joystickZyklischeGeschwindigkeitMax", 5000);
    joystickTotbereichOverride = preferences.getFloat("joystickTotbereichOverride", 0.1f);
    polaritaetsSchwellePositiv = preferences.getFloat("polaritaetsSchwellePositiv", 0.2f);
    polaritaetsSchwelleNegativ = preferences.getFloat("polaritaetsSchwelleNegativ", -0.2f);
    halbeAnIntervall = preferences.getLong("halbeAnIntervall", 500);
    joystickAktiv = preferences.getBool("joystickAktiv", false);
    joystickXInvertieren = preferences.getBool("joystickXInvertieren", false);
    joystickYInvertieren = preferences.getBool("joystickYInvertieren", false);
    joystickTotbereich = preferences.getInt("joystickTotbereich", 10);
    joystickSkalierung = preferences.getFloat("joystickSkalierung", 1.0f);
    joystickBasisGeschwindigkeit = preferences.getInt("joystickBasisGeschwindigkeit", 100);
    joystickGeschwindigkeitsAchse = preferences.getString("joystickGeschwindigkeitsAchse", "x");
    vorrueckIntervall = preferences.getLong("vorrueckIntervall", 500);
    aktuellerOverridePWM = preferences.getInt("aktuellerOverridePWM", 200);
    joystickSteuertPWM = preferences.getBool("joystickSteuertPWM", false);
    joystickSteuertIntensitaetZ = preferences.getBool("joystickSteuertIntensitaetZ", false);
    joystickSteuertGeschwindigkeitW = preferences.getBool("joystickSteuertGeschwindigkeitW", false);
    aktuellesJoystickMusterIndex = preferences.getInt("aktuellesJoystickMusterIndex", 0);
    joystickMusterIntervall = preferences.getLong("joystickMusterIntervall", 500);
    grundPWM = preferences.getInt("grundPWM", 150);
    Serial.print("Geladener grundPWM: ");
    Serial.println(grundPWM);
    aktuellerModus = static_cast<SteuerungModus>(preferences.getInt("aktuellerModus", static_cast<int>(GRUNDMUSTER_PWM)));
    joystickMaxIntensitaet = preferences.getInt("maxjoystickpwm", 255); // Beachte den Preference-Schlüssel
    
    preferences.end();
    Serial.println("Gespeicherte Einstellungen geladen.");
}

void saveSettings() {
    Serial.println("Speichere Einstellungen...");
    preferences.begin("magnet_settings");

    preferences.putInt("overrideModus", static_cast<int>(aktiverOverrideModus));
    preferences.putInt("overridePWM", overridePWM);
    preferences.putInt("globalePWMIntensitaet", globalePWMIntensitaet);
    preferences.putLong("grundMusterIntervall", grundMusterIntervall);
    preferences.putLong("pulsIntervall", pulsIntervall);
    preferences.putLong("wellenGeschwindigkeit", wellenGeschwindigkeit);
    preferences.putLong("zufallsIntervallMin", zufallsIntervallMin);
    preferences.putLong("zufallsIntervallMax", zufallsIntervallMax);
    preferences.putLong("fadingDauer", fadingDauer);
    preferences.putInt("aktiveSequenz", aktiveSequenz);
    preferences.putLong("sequenzIntervall", sequenzIntervall);
    preferences.putInt("halbeAnModus", halbeAnModus);
    preferences.putInt("alleAnAusZustand", alleAnAusZustand);
    preferences.putBool("joystickSteuertZyklischeGeschwindigkeit", joystickSteuertZyklischeGeschwindigkeit);
    preferences.putString("joystickZyklischeGeschwindigkeitsAchse", joystickZyklischeGeschwindigkeitsAchse.c_str());
    preferences.putLong("joystickZyklischeGeschwindigkeitMin", joystickZyklischeGeschwindigkeitMin);
    preferences.putLong("joystickZyklischeGeschwindigkeitMax", joystickZyklischeGeschwindigkeitMax);
    preferences.putFloat("joystickTotbereichOverride", joystickTotbereichOverride);
    preferences.putFloat("polaritaetsSchwellePositiv", polaritaetsSchwellePositiv);
    preferences.putFloat("polaritaetsSchwelleNegativ", polaritaetsSchwelleNegativ);
    preferences.putLong("halbeAnIntervall", halbeAnIntervall);
    preferences.putBool("joystickAktiv", joystickAktiv);
    preferences.putBool("joystickXInvertieren", joystickXInvertieren);
    preferences.putBool("joystickYInvertieren", joystickYInvertieren);
    preferences.putInt("joystickTotbereich", joystickTotbereich);
    preferences.putFloat("joystickSkalierung", joystickSkalierung);
    preferences.putInt("joystickBasisGeschwindigkeit", joystickBasisGeschwindigkeit);
    preferences.putString("joystickGeschwindigkeitsAchse", joystickGeschwindigkeitsAchse.c_str());
    preferences.putLong("vorrueckIntervall", vorrueckIntervall);
    preferences.putInt("aktuellerOverridePWM", aktuellerOverridePWM);
    preferences.putBool("joystickSteuertPWM", joystickSteuertPWM);
    preferences.putBool("joystickSteuertIntensitaetZ", joystickSteuertIntensitaetZ);
    preferences.putBool("joystickSteuertGeschwindigkeitW", joystickSteuertGeschwindigkeitW);
    preferences.putInt("aktuellesJoystickMusterIndex", aktuellesJoystickMusterIndex);
    preferences.putLong("joystickMusterIntervall", joystickMusterIntervall);
    preferences.putInt("grundPWM", grundPWM);
    preferences.putInt("aktuellerModus", static_cast<int>(aktuellerModus));
    preferences.putInt("joystickMaxIntensitaet", joystickMaxIntensitaet);
    preferences.putInt("maxjoystickpwm", joystickMaxIntensitaet); // Beachte den Preference-Schlüssel

    preferences.end();
    Serial.println("Einstellungen gespeichert.");
}

void savePatterns() {
    preferences.begin("magnet_config", false);
    preferences.putInt("overrideModus", static_cast<int>(aktiverOverrideModus)); // Explizit in int casten
    preferences.putInt("overridePWM", overridePWM);
    preferences.putInt("pwmFrequency", pwmFrequency);
    preferences.putLong("grundMusterIntervall", grundMusterIntervall);
    preferences.putLong("pulsIntervall", pulsIntervall);
    preferences.putLong("wellenGeschwindigkeit", wellenGeschwindigkeit);
    preferences.putLong("zufallsIntervallMin", zufallsIntervallMin);
    preferences.putLong("zufallsIntervallMax", zufallsIntervallMax);
    preferences.putLong("fadingDauer", fadingDauer);
    preferences.putInt("aktiveSequenz", aktiveSequenz);
    preferences.putLong("sequenzIntervall", sequenzIntervall);
    preferences.putInt("halbeAnModus", halbeAnModus);
    preferences.putLong("halbeAnIntervall", halbeAnIntervall);
    preferences.putBool("joystickAktiv", joystickAktiv);
    preferences.putBool("joystickXInvertieren", joystickXInvertieren);
    preferences.putBool("joystickYInvertieren", joystickYInvertieren);
    preferences.putInt("joystickTotbereich", joystickTotbereich);
    preferences.putFloat("joystickSkalierung", joystickSkalierung);
    preferences.putLong("joystickBasisGeschwindigkeit", joystickBasisGeschwindigkeit);
    preferences.putString("joystickSpeedAxis", joystickGeschwindigkeitsAchse);
    preferences.putBool("joystickCyclicSpeed", joystickSteuertZyklischeGeschwindigkeit);
    preferences.putString("joystickCyclicSpeedAxis", joystickZyklischeGeschwindigkeitsAchse);
    preferences.putLong("joystickCyclicSpeedMin", joystickZyklischeGeschwindigkeitMin);
    preferences.putLong("joystickCyclicSpeedMax", joystickZyklischeGeschwindigkeitMax);
    preferences.putFloat("joystickTotzoneOverride", joystickTotbereichOverride);
    preferences.putFloat("polarityThresholdPositive", polaritaetsSchwellePositiv);
    preferences.putFloat("polarityThresholdNegative", polaritaetsSchwelleNegativ);
    preferences.putBool("joystickControlsPWM", joystickSteuertPWM);
    preferences.putBool("joystickControlsIntensityZ", joystickSteuertIntensitaetZ);
    preferences.putBool("joystickControlsSpeedW", joystickSteuertGeschwindigkeitW);

    DynamicJsonDocument grundmusterDoc(4096);
    JsonArray grundmusterArray = grundmusterDoc.to<JsonArray>();
    for (const auto& pattern : grundPWMMuster) {
        JsonObject patternObj = grundmusterArray.createNestedObject();
        patternObj["name"] = pattern.name;
        JsonArray schritte = patternObj.createNestedArray("schritte");
        for (int intensitaet : pattern.intensitaeten) {
            schritte.add(intensitaet);
        }
    }
    String grundmusterJson;
    serializeJson(grundmusterDoc, grundmusterJson);
    preferences.putString("grundmusterJson", grundmusterJson);

    DynamicJsonDocument joystickmusterDoc(4096);
    JsonArray joystickmusterArray = joystickmusterDoc.to<JsonArray>();
    for (const auto& pattern : joystickPWMMuster) {
        JsonObject patternObj = joystickmusterArray.createNestedObject();
        patternObj["name"] = pattern.name;
        JsonArray schritte = patternObj.createNestedArray("schritte");
        for (int intensitaet : pattern.intensitaeten) {
            schritte.add(intensitaet);
        }
    }
    String joystickmusterJson;
    serializeJson(joystickmusterDoc, joystickmusterJson);
    preferences.putString("joystickmusterJson", joystickmusterJson);

    preferences.end();
    Serial.println("Einstellungen und Muster gespeichert.");
}

void loadPatterns() {
    preferences.begin("magnet_settings", false); // Korrigierter Namespace

    // Laden der grundlegenden Konfigurationen (bleiben gleich)
    aktiverOverrideModus = static_cast<OverrideModus>(preferences.getInt("overrideModus", static_cast<int>(OverrideModus::KEIN_OVERRIDE)));
    overridePWM = preferences.getInt("overridePWM", 128);
    pwmFrequency = preferences.getInt("pwmFrequency", 1000);
    grundMusterIntervall = preferences.getLong("grundMusterIntervall", 500);
    pulsIntervall = preferences.getLong("pulsIntervall", 200);
    wellenGeschwindigkeit = preferences.getLong("wellenGeschwindigkeit", 500);
    zufallsIntervallMin = preferences.getLong("zufallsIntervallMin", 100);
    zufallsIntervallMax = preferences.getLong("zufallsIntervallMax", 2000);
    fadingDauer = preferences.getLong("fadingDauer", 1000);
    aktiveSequenz = preferences.getInt("aktiveSequenz", 0);
    sequenzIntervall = preferences.getLong("sequenzIntervall", 1000);
    halbeAnModus = preferences.getInt("halbeAnModus", 0);
    halbeAnIntervall = preferences.getLong("halbeAnIntervall", 500);
    joystickAktiv = preferences.getBool("joystickAktiv", false);
    joystickXInvertieren = preferences.getBool("joystickXInvertieren", false);
    joystickYInvertieren = preferences.getBool("joystickYInvertieren", false);
    joystickTotbereich = preferences.getInt("joystickTotbereich", 50);
    joystickSkalierung = preferences.getFloat("joystickSkalierung", 1.0f);
    joystickBasisGeschwindigkeit = preferences.getLong("joystickBasisGeschwindigkeit", 50);
    joystickGeschwindigkeitsAchse = preferences.getString("joystickSpeedAxis", "x");
    joystickSteuertZyklischeGeschwindigkeit = preferences.getBool("joystickCyclicSpeed", false);
    joystickZyklischeGeschwindigkeitsAchse = preferences.getString("joystickCyclicSpeedAxis", "x");
    joystickZyklischeGeschwindigkeitMin = preferences.getLong("joystickCyclicSpeedMin", 100);
    joystickZyklischeGeschwindigkeitMax = preferences.getLong("joystickCyclicSpeedMax", 5000);
    joystickTotbereichOverride = preferences.getFloat("joystickTotzoneOverride", 0.05f);
    polaritaetsSchwellePositiv = preferences.getFloat("polarityThresholdPositive", 0.1f);
    polaritaetsSchwelleNegativ = preferences.getFloat("polarityThresholdNegative", -0.1f);
    joystickSteuertPWM = preferences.getBool("joystickControlsPWM", false);
    joystickSteuertIntensitaetZ = preferences.getBool("joystickControlsIntensityZ", false);
    joystickSteuertGeschwindigkeitW = preferences.getBool("joystickControlsSpeedW", false);
    joystickMaxIntensitaet  = preferences.getInt("maxjoystickpwm", 128);


    // Laden von grundPWMMuster
    String grundmusterJson = preferences.getString("grundmusterJson", "[{\"name\":\"Default\",\"schritte\":[0, 255, 0]}]");
    Serial.printf("Lade: grundmusterJson = %s\n", grundmusterJson.c_str());
    DynamicJsonDocument grundmusterDoc(4096);
    DeserializationError errorGrundmuster = deserializeJson(grundmusterDoc, grundmusterJson);
    if (errorGrundmuster) {
        Serial.print(F("Fehler bei der Deserialisierung des grundmusterJson: "));
        Serial.println(errorGrundmuster.c_str());
        grundPWMMuster.clear();
        grundPWMMuster.push_back({.name = "Default", .intensitaeten = {0, 255, 0}}); // Fallback-Muster
    } else {
        JsonArray grundmusterArray = grundmusterDoc.as<JsonArray>();
        grundPWMMuster.clear();
        for (JsonVariant v : grundmusterArray) {
            PWMMuster pattern;
            pattern.name = v["name"].as<String>();
            JsonArray schritte = v["schritte"].as<JsonArray>();
            for (JsonVariant schrittVar : schritte) {
                pattern.intensitaeten.push_back(schrittVar.as<int>());
            }
            grundPWMMuster.push_back(pattern);
        }
    }

    // Laden von joystickPWMMuster
    String joystickmusterJson = preferences.getString("joystickmusterJson", "[]");
    Serial.printf("Lade: joystickmusterJson = %s\n", joystickmusterJson.c_str());
    DynamicJsonDocument joystickmusterDoc(4096);
    DeserializationError errorJoystick = deserializeJson(joystickmusterDoc, joystickmusterJson);
    if (errorJoystick) {
        Serial.print(F("Fehler bei der Deserialisierung des joystickmusterJson: "));
        Serial.println(errorJoystick.c_str());
        joystickPWMMuster.clear();
    } else {
        JsonArray joystickmusterArray = joystickmusterDoc.as<JsonArray>();
        joystickPWMMuster.clear();
        for (JsonVariant v : joystickmusterArray) {
            PWMMuster pattern;
            pattern.name = v["name"].as<String>();
            JsonArray schritte = v["schritte"].as<JsonArray>();
            for (JsonVariant schrittVar : schritte) {
                pattern.intensitaeten.push_back(schrittVar.as<int>());
            }
            joystickPWMMuster.push_back(pattern);
        }
    }

    preferences.end();
}

void handleSet(AsyncWebServerRequest *request) {
    Serial.println("Verarbeite /set Anfrage...");

    // Einstellungen verarbeiten
    if (request->hasParam("modus")) {
        String modus = request->getParam("modus")->value();
        if (modus == "grundmuster") {
            aktuellerModus = SteuerungModus::GRUNDMUSTER_PWM;
        } else if (modus == "joystick") {
            aktuellerModus = SteuerungModus::JOYSTICK_MODUS;
        }
    }
    if (request->hasParam("grundpwm")) {
        grundPWM = request->getParam("grundpwm")->value().toInt();
        Serial.print("Empfangener grundpwm: ");
        Serial.println(grundPWM);
    }
    if (request->hasParam("joystickpwm")) {
        joystickMaxIntensitaet = request->getParam("joystickpwm")->value().toInt();
        Serial.print("Empfangener joystickpwm: ");
        Serial.println(joystickMaxIntensitaet);
        preferences.putInt("maxjoystickpwm", joystickMaxIntensitaet);
    }
    if (request->hasParam("override")) {
        String overrideStr = request->getParam("override")->value();
        Serial.print("Empfangener Override-Modus: ");
        Serial.println(overrideStr);
        if (overrideStr == "-1") aktiverOverrideModus = OverrideModus::KEIN_OVERRIDE;
        else if (overrideStr == "zyklisch") aktiverOverrideModus = OverrideModus::ZYKLISCHER_OVERRIDE;
        else if (overrideStr == "vorrueck") aktiverOverrideModus = OverrideModus::VORRUECK_OVERRIDE;
        else if (overrideStr == "ruhig") aktiverOverrideModus = OverrideModus::RUHIG_OVERRIDE;
        else if (overrideStr == "pulsierend") aktiverOverrideModus = OverrideModus::PULSIEREND_OVERRIDE;
        else if (overrideStr == "welle") aktiverOverrideModus = OverrideModus::WELLEN_OVERRIDE;
        else if (overrideStr == "zufall") aktiverOverrideModus = OverrideModus::ZUFALLS_OVERRIDE;
        else if (overrideStr == "fading") aktiverOverrideModus = OverrideModus::FADING_OVERRIDE;
        else if (overrideStr == "sequenz") aktiverOverrideModus = OverrideModus::SEQUENZ_OVERRIDE;
        else if (overrideStr == "allean") aktiverOverrideModus = OverrideModus::ALLE_AN_AUS_OVERRIDE;
        else if (overrideStr == "halbean") aktiverOverrideModus = OverrideModus::HALBE_AN_OVERRIDE;
        else if (overrideStr == "muster") aktiverOverrideModus = OverrideModus::MUSTER_OVERRIDE;
    }
    if (request->hasParam("overridepwm")) {
        overridePWM = request->getParam("overridepwm")->value().toInt();
        Serial.print("Empfangener Override-PWM: ");
        Serial.println(overridePWM);
    }
    if (request->hasParam("globaleintensitaet")) {
        globalePWMIntensitaet = request->getParam("globaleintensitaet")->value().toInt();
        Serial.print("Empfangene globale Intensität: ");
        Serial.println(globalePWMIntensitaet);
    }
    if (request->hasParam("grundmusterintervall")) {
        grundMusterIntervall = request->getParam("grundmusterintervall")->value().toInt();
        Serial.print("Empfangenes Grundmuster Intervall: ");
        Serial.println(grundMusterIntervall);
    }
    if (request->hasParam("pulsintervall")) {
        pulsIntervall = request->getParam("pulsintervall")->value().toInt();
        Serial.print("Empfangenes Puls Intervall: ");
        Serial.println(pulsIntervall);
    }
    if (request->hasParam("wellengeschwindigkeit")) {
        wellenGeschwindigkeit = request->getParam("wellengeschwindigkeit")->value().toInt();
        Serial.print("Empfangene Wellen Geschwindigkeit: ");
        Serial.println(wellenGeschwindigkeit);
    }
    if (request->hasParam("zufallsintervallmin")) {
        zufallsIntervallMin = request->getParam("zufallsintervallmin")->value().toInt();
        Serial.print("Empfangenes Zufalls Intervall Min: ");
        Serial.println(zufallsIntervallMin);
    }
    if (request->hasParam("zufallsintervallmax")) {
        zufallsIntervallMax = request->getParam("zufallsintervallmax")->value().toInt();
        Serial.print("Empfangenes Zufalls Intervall Max: ");
        Serial.println(zufallsIntervallMax);
    }
    if (request->hasParam("fadingdauer")) {
        fadingDauer = request->getParam("fadingdauer")->value().toInt();
        Serial.print("Empfangene Fading Dauer: ");
        Serial.println(fadingDauer);
    }
    if (request->hasParam("aktivesequenz")) {
        aktiveSequenz = request->getParam("aktivesequenz")->value().toInt();
        Serial.print("Empfangene aktive Sequenz: ");
        Serial.println(aktiveSequenz);
    }
    if (request->hasParam("sequenzintervall")) {
        sequenzIntervall = request->getParam("sequenzintervall")->value().toInt();
        Serial.print("Empfangenes Sequenz Intervall: ");
        Serial.println(sequenzIntervall);
    }
    if (request->hasParam("halbeanmodus")) {
        halbeAnModus = request->getParam("halbeanmodus")->value().toInt();
        Serial.print("Empfangener Halbe An Modus: ");
        Serial.println(halbeAnModus);
    }
    if (request->hasParam("alleanzustand")) {
        alleAnAusZustand = request->getParam("alleanzustand")->value().toInt();
        Serial.print("Empfangener Alle An/Aus Zustand: ");
        Serial.println(alleAnAusZustand);
    }
    if (request->hasParam("joystickCyclicSpeed")) {
        joystickSteuertZyklischeGeschwindigkeit = request->getParam("joystickCyclicSpeed")->value().toInt() == 1;
        Serial.print("Empfange Joystick steuert zyklische Geschwindigkeit: ");
        Serial.println(joystickSteuertZyklischeGeschwindigkeit);
    }
    if (request->hasParam("joystickCyclicSpeedAxis")) {
        joystickZyklischeGeschwindigkeitsAchse = request->getParam("joystickCyclicSpeedAxis")->value();
        Serial.print("Empfangene Joystick zyklische Geschwindigkeitsachse: ");
        Serial.println(joystickZyklischeGeschwindigkeitsAchse);
    }
    if (request->hasParam("joystickCyclicSpeedMin")) {
        joystickZyklischeGeschwindigkeitMin = request->getParam("joystickCyclicSpeedMin")->value().toInt();
        Serial.print("Empfangener Joystick zyklische Geschwindigkeit Min: ");
        Serial.println(joystickZyklischeGeschwindigkeitMin);
    }
    if (request->hasParam("joystickCyclicSpeedMax")) {
        joystickZyklischeGeschwindigkeitMax = request->getParam("joystickCyclicSpeedMax")->value().toInt();
        Serial.print("Empfangener Joystick zyklische Geschwindigkeit Max: ");
        Serial.println(joystickZyklischeGeschwindigkeitMax);
    }
    if (request->hasParam("joystickTotzoneOverride")) {
        joystickTotbereichOverride = request->getParam("joystickTotzoneOverride")->value().toFloat();
        Serial.print("Empfangener Joystick Totbereich Override: ");
        Serial.println(joystickTotbereichOverride);
    }
    if (request->hasParam("polarityThresholdPositive")) {
        polaritaetsSchwellePositiv = request->getParam("polarityThresholdPositive")->value().toFloat();
        Serial.print("Empfangene Polaritäts Schwelle Positiv: ");
        Serial.println(polaritaetsSchwellePositiv);
    }
    if (request->hasParam("polarityThresholdNegative")) {
        polaritaetsSchwelleNegativ = request->getParam("polarityThresholdNegative")->value().toFloat();
        Serial.print("Empfangene Polaritäts Schwelle Negativ: ");
        Serial.println(polaritaetsSchwelleNegativ);
    }
    if (request->hasParam("halbeanintervall")) {
        halbeAnIntervall = request->getParam("halbeanintervall")->value().toInt();
        Serial.print("Empfangenes Halbe An Intervall: ");
        Serial.println(halbeAnIntervall);
    }
    if (request->hasParam("joytickaktiv")) {
        joystickAktiv = request->getParam("joytickaktiv")->value().toInt() == 1;
        Serial.print("Empfangener Joystick Aktiv: ");
        Serial.println(joystickAktiv);
    }
    if (request->hasParam("joytickinvertx")) {
        joystickXInvertieren = request->getParam("joytickinvertx")->value().toInt() == 1;
        Serial.print("Empfangene Joystick X Invertieren: ");
        Serial.println(joystickXInvertieren);
    }
    if (request->hasParam("joytickinverty")) {
        joystickYInvertieren = request->getParam("joytickinverty")->value().toInt() == 1;
        Serial.print("Empfangene Joystick Y Invertieren: ");
        Serial.println(joystickYInvertieren);
    }
    if (request->hasParam("joyticktotbereich")) {
        joystickTotbereich = request->getParam("joyticktotbereich")->value().toInt();
        Serial.print("Empfangener Joystick Totbereich: ");
        Serial.println(joystickTotbereich);
    }
    if (request->hasParam("joytickskalierung")) {
        joystickSkalierung = request->getParam("joytickskalierung")->value().toFloat();
        Serial.print("Empfangene Joystick Skalierung: ");
        Serial.println(joystickSkalierung);
    }
    if (request->hasParam("joytickbasisgeschwindigkeit")) {
        joystickBasisGeschwindigkeit = request->getParam("joytickbasisgeschwindigkeit")->value().toInt();
        Serial.print("Empfangene Joystick Basis Geschwindigkeit: ");
        Serial.println(joystickBasisGeschwindigkeit);
    }
    if (request->hasParam("joytickachsen")) {
        joystickGeschwindigkeitsAchse = request->getParam("joytickachsen")->value();
        Serial.print("Empfangene Joystick Achse: ");
        Serial.println(joystickGeschwindigkeitsAchse);
    }
    if (request->hasParam("vorrueckintervall")) {
        vorrueckIntervall = request->getParam("vorrueckintervall")->value().toInt();
        Serial.print("Empfangenes Vorrück Intervall: ");
        Serial.println(vorrueckIntervall);
    }
    if (request->hasParam("aktuelleroverridepwm")) {
        aktuellerOverridePWM = request->getParam("aktuelleroverridepwm")->value().toInt();
        Serial.print("Empfangener aktueller Override PWM: ");
        Serial.println(aktuellerOverridePWM);
    }
    if (request->hasParam("joystickpwm")) {
        joystickMaxIntensitaet = request->getParam("joystickpwm")->value().toInt();
        Serial.print("Empfangener joystickpwm: ");
        Serial.println(joystickMaxIntensitaet);
        preferences.putInt("joystickMaxIntensitaet", joystickMaxIntensitaet);
}
    if (request->hasParam("joystickIntensityZ")) {
        joystickSteuertIntensitaetZ = request->getParam("joystickIntensityZ")->value().toInt() == 1;
        Serial.print("Empfange Joystick steuert Intensität Z: ");
        Serial.println(joystickSteuertIntensitaetZ);
    }
    if (request->hasParam("joystickSpeedW")) {
        joystickSteuertGeschwindigkeitW = request->getParam("joystickSpeedW")->value().toInt() == 1;
        Serial.print("Empfange Joystick steuert Geschwindigkeit W: ");
        Serial.println(joystickSteuertGeschwindigkeitW);
    }
    if (request->hasParam("aktuellesjoystickmusterindex")) {
        aktuellesJoystickMusterIndex = request->getParam("aktuellesjoystickmusterindex")->value().toInt();
        Serial.print("Empfangener aktueller Joystick Muster Index: ");
        Serial.println(aktuellesJoystickMusterIndex);
    }
    if (request->hasParam("joystickmusterintervall")) {
        joystickMusterIntervall = request->getParam("joystickmusterintervall")->value().toInt();
        Serial.print("Empfangenes Joystick Muster Intervall: ");
        Serial.println(joystickMusterIntervall);
    }

    // Musterdaten verarbeiten (falls vorhanden)
    if (request->hasParam("plain", true)) {
        String body = request->getParam("plain", true)->value();
        DynamicJsonDocument doc(4096); // Größe je nach erwarteter Musteranzahl
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            Serial.print("deserializeJson() beim Verarbeiten von Mustern fehlgeschlagen: ");
            Serial.println(error.c_str());
            request->send(400, "text/plain", "Ungültiges JSON-Format für Muster");
            return;
        }

        if (doc.containsKey("grundmuster")) {
            grundPWMMuster.clear();
            JsonArray grundArray = doc["grundmuster"].as<JsonArray>();
            for (JsonObject musterObj : grundArray) {
                PWMMuster muster;
                muster.name = musterObj["name"].as<String>();
                JsonArray schritteArray = musterObj["schritte"].as<JsonArray>();
                for (JsonVariant schritt : schritteArray) {
                    muster.intensitaeten.push_back(schritt.as<int>());
                }
                grundPWMMuster.push_back(muster);
            }
            Serial.println("Grundmuster aktualisiert.");
        }

        if (doc.containsKey("joystickmuster")) {
            joystickPWMMuster.clear();
            JsonArray joystickArray = doc["joystickmuster"].as<JsonArray>();
            for (JsonObject musterObj : joystickArray) {
                PWMMuster muster;
                muster.name = musterObj["name"].as<String>();
                JsonArray schritteArray = musterObj["schritte"].as<JsonArray>();
                for (JsonVariant schritt : schritteArray) {
                    muster.intensitaeten.push_back(schritt.as<int>());
                }
                joystickPWMMuster.push_back(muster);
            }
            Serial.println("Joystick Muster aktualisiert.");
        }
    }

    // Einstellungen und Muster speichern
    saveSettings();
    savePatterns();

    request->send(200, "text/plain", "Einstellungen und Muster gespeichert.");
    Serial.println("Einstellungen und Muster gespeichert.");
}

void handleSavePatterns(AsyncWebServerRequest *request) {
    Serial.println("Verarbeite /save_patterns Anfrage...");
    if (request->hasParam("plain", true)) {
        String body = request->getParam("plain", true)->value();
        DynamicJsonDocument doc(4096); // Größe je nach erwarteter Musteranzahl
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            Serial.print("deserializeJson() beim Verarbeiten von Mustern fehlgeschlagen: ");
            Serial.println(error.c_str());
            request->send(400, "text/plain", "Ungültiges JSON-Format für Muster");
            return;
        }

        if (doc.containsKey("grundmuster")) {
            grundPWMMuster.clear();
            JsonArray grundArray = doc["grundmuster"].as<JsonArray>();
            for (JsonObject musterObj : grundArray) {
                PWMMuster muster;
                muster.name = musterObj["name"].as<String>();
                JsonArray schritteArray = musterObj["schritte"].as<JsonArray>();
                for (JsonVariant schritt : schritteArray) {
                    muster.intensitaeten.push_back(schritt.as<int>());
                }
                grundPWMMuster.push_back(muster);
            }
            Serial.println("Grundmuster aktualisiert.");
        }

        if (doc.containsKey("maxintensivmuster")) {
            maxIntensivPWMMuster.clear();
            JsonArray maxIntensivArray = doc["maxintensivmuster"].as<JsonArray>();
            for (JsonObject musterObj : maxIntensivArray) {
                PWMMuster muster;
                muster.name = musterObj["name"].as<String>();
                JsonArray schritteArray = musterObj["schritte"].as<JsonArray>();
                for (JsonVariant schritt : schritteArray) {
                    muster.intensitaeten.push_back(schritt.as<int>());
                }
                maxIntensivPWMMuster.push_back(muster);
            }
            Serial.println("Max Intensiv Muster aktualisiert.");
        }

        if (doc.containsKey("joystickmuster")) {
            joystickPWMMuster.clear();
            JsonArray joystickArray = doc["joystickmuster"].as<JsonArray>();
            for (JsonObject musterObj : joystickArray) {
                PWMMuster muster;
                muster.name = musterObj["name"].as<String>();
                JsonArray schritteArray = musterObj["schritte"].as<JsonArray>();
                for (JsonVariant schritt : schritteArray) {
                    muster.intensitaeten.push_back(schritt.as<int>());
                }
                joystickPWMMuster.push_back(muster);
            }
            Serial.println("Joystick Muster aktualisiert.");
        }

        savePatterns();
        request->send(200, "text/plain", "Muster gespeichert.");
        Serial.println("Muster gespeichert.");

    } else {
        request->send(400, "text/plain", "Keine Daten im Anfragekörper für Muster");
    }
}

void modusUmschalten(SteuerungModus neuerModus) {
    if (aktuellerModus != neuerModus) {
        Serial.printf("Modus wird umgeschaltet von %d zu %d\n", aktuellerModus, neuerModus);
        aktuellerModus = neuerModus;
        // Hier könnten zusätzliche Aktionen beim Moduswechsel erfolgen
    }
}
void updateMagnets() {
    aktuelleZeit = millis();

if (aktuellerModus == JOYSTICK_MODUS && joystickAktiv) {
    int joystickXRoh = liesJoystick(joystickXPins);
    int joystickYRoh = liesJoystick(joystickYPins);
    int joystickZRoh = liesJoystick(joystickZPins);
    int joystickWRoh = liesJoystick(joystickWPins);

    float joystickX = mapFloat(joystickXRoh, 0, 4095, -1.0f, 1.0f);
    float joystickY = mapFloat(joystickYRoh, 0, 4095, -1.0f, 1.0f);
    float joystickZ = mapFloat(joystickZRoh, 0, 4095, 0.0f, 1.0f); // Für Intensität 0-1
    float joystickW = mapFloat(joystickWRoh, 0, 4095, 0.0f, 1.0f); // Für Geschwindigkeit 0-1

    if (joystickXInvertieren) joystickX *= -1;
    if (joystickYInvertieren) joystickY *= -1;

    float geschwindigkeitFaktor = 1.0f;
    if (joystickSteuertGeschwindigkeitW) {
        geschwindigkeitFaktor = joystickW;
    }

    if (aktiverOverrideModus == OverrideModus::ZYKLISCHER_OVERRIDE && joystickSteuertZyklischeGeschwindigkeit) {
        float joystickWert = 0;
        if (joystickZyklischeGeschwindigkeitsAchse == "x") joystickWert = joystickX;
        else if (joystickZyklischeGeschwindigkeitsAchse == "y") joystickWert = joystickY;

        if (abs(joystickWert) > joystickTotbereichOverride) {
            grundMusterIntervall = mapFloat(joystickWert, -1.0f, 1.0f, joystickZyklischeGeschwindigkeitMin, joystickZyklischeGeschwindigkeitMax);
            if (grundMusterIntervall < 10) grundMusterIntervall = 10; // Schutz vor zu schnellen Intervallen
        } else {
            grundMusterIntervall = joystickBasisGeschwindigkeit; // Standardgeschwindigkeit bei Totbereich
        }
    }

    int pwmWert = overridePWM;
    if (joystickSteuertPWM) {
        float joystickWert = 0;
        if (joystickGeschwindigkeitsAchse == "x") joystickWert = joystickX;
        else if (joystickGeschwindigkeitsAchse == "y") joystickWert = joystickY;
        pwmWert = mapFloat(joystickWert, -1.0f, 1.0f, 0, 255);
        pwmWert = constrain(pwmWert, 0, 255);
    }

    int globaleIntensitaet = globalePWMIntensitaet;
    if (joystickSteuertIntensitaetZ) {
        globaleIntensitaet = mapFloat(joystickZ, 0.0f, 1.0f, 0, 255);
    }

    if (aktiverOverrideModus == OverrideModus::MUSTER_OVERRIDE && !joystickPWMMuster.empty()) {
        if (aktuelleZeit - letztesJoystickMusterZeit >= (joystickMusterIntervall * geschwindigkeitFaktor)) {
            letztesJoystickMusterZeit = aktuelleZeit;
            const auto& aktuellesMuster = joystickPWMMuster[0]; // Verwende immer das erste Muster für Joystick-Steuerung (kann später erweitert werden)
            if (!aktuellesMuster.intensitaeten.empty()) {
                int intensitaet = aktuellesMuster.intensitaeten[aktuellesJoystickMusterIndex % aktuellesMuster.intensitaeten.size()];
                for (int i = 0; i < NUM_MAGNETS; ++i) {
                    setMagnetPWM(i, static_cast<int>(intensitaet * (globaleIntensitaet / 255.0f)));
                }
                setMagnetPWM(STATIC_MAGNET_INDEX, static_cast<int>(intensitaet * (globaleIntensitaet / 255.0f)));
                aktuellesJoystickMusterIndex++;
            }
        }
    } else if (aktiverOverrideModus != OverrideModus::KEIN_OVERRIDE && aktiverOverrideModus != OverrideModus::MUSTER_OVERRIDE) {
        for (int i = 0; i < NUM_MAGNETS; ++i) {
            setMagnetPWM(i, static_cast<int>(pwmWert * (globaleIntensitaet / 255.0f)));
        }
        setMagnetPWM(STATIC_MAGNET_INDEX, static_cast<int>(pwmWert * (globaleIntensitaet / 255.0f)));
    } else if (aktiverOverrideModus == OverrideModus::KEIN_OVERRIDE) {
        for (int i = 0; i < NUM_MAGNETS + 1; ++i) {
            setMagnetPWM(i, 0);
        }
    }

} else if (aktuellerModus == GRUNDMUSTER_PWM) {
    if (aktiverOverrideModus == OverrideModus::ZYKLISCHER_OVERRIDE && !grundPWMMuster.empty()) {
        if (aktuelleZeit - letzteMusterAenderungZeit >= grundMusterIntervall) {
            letzteMusterAenderungZeit = aktuelleZeit;
            const auto& aktuellesMuster = grundPWMMuster[aktiveSequenz % grundPWMMuster.size()];
            if (!aktuellesMuster.intensitaeten.empty()) {
                for (int i = 0; i < NUM_MAGNETS; ++i) {
                    setMagnetPWM(i, aktuellesMuster.intensitaeten[sequenzSchritt % aktuellesMuster.intensitaeten.size()]);
                }
                setMagnetPWM(STATIC_MAGNET_INDEX, aktuellesMuster.intensitaeten[sequenzSchritt % aktuellesMuster.intensitaeten.size()]);
                sequenzSchritt = (sequenzSchritt + 1) % aktuellesMuster.intensitaeten.size();
            }
        }
    } else if (aktiverOverrideModus == OverrideModus::PULSIEREND_OVERRIDE) {
        if (aktuelleZeit - letztePulsZeit >= pulsIntervall) {
            letztePulsZeit = aktuelleZeit;
            pulsZustand = !pulsZustand;
            int intensitaet = pulsZustand ? overridePWM : 0;
            for (int i = 0; i < NUM_MAGNETS; ++i) {
                setMagnetPWM(i, intensitaet);
            }
            setMagnetPWM(STATIC_MAGNET_INDEX, intensitaet);
        }
    } else if (aktiverOverrideModus == OverrideModus::WELLEN_OVERRIDE) {
        if (aktuelleZeit - letzteWellenZeit >= wellenGeschwindigkeit) {
            letzteWellenZeit = aktuelleZeit;
            for (int i = 0; i < NUM_MAGNETS; ++i) {
                int phase = (sequenzSchritt + i * 10) % 255;
                int intensitaet = (sin(phase * 2 * M_PI / 255.0f) * 0.5f + 0.5f) * overridePWM;
                setMagnetPWM(i, intensitaet);
            }
            int staticPhase = (sequenzSchritt + 7 * 10) % 255;
            int staticIntensitaet = (sin(staticPhase * 2 * M_PI / 255.0f) * 0.5f + 0.5f) * overridePWM;
            setMagnetPWM(STATIC_MAGNET_INDEX, staticIntensitaet);
            sequenzSchritt = (sequenzSchritt + 5) % 255;
        }
    } else if (aktiverOverrideModus == OverrideModus::ZUFALLS_OVERRIDE) {
        if (aktuelleZeit - letzteZufallsAenderungZeit >= random(zufallsIntervallMin, zufallsIntervallMax)) {
            letzteZufallsAenderungZeit = aktuelleZeit;
            for (int i = 0; i < NUM_MAGNETS; ++i) {
                setMagnetPWM(i, random(0, overridePWM + 1));
            }
            setMagnetPWM(STATIC_MAGNET_INDEX, random(0, overridePWM + 1));
        }
    } else if (aktiverOverrideModus == OverrideModus::FADING_OVERRIDE) {
        if (aktuelleZeit - fadingStartZeit >= fadingDauer / 255.0f) {
            fadingStartZeit = aktuelleZeit;
            for (int i = 0; i < NUM_MAGNETS + 1; ++i) {
                magnetIntensitaeten[i] += fadingRichtung[i];
                if (magnetIntensitaeten[i] >= overridePWM || magnetIntensitaeten[i] <= 0) {
                    fadingRichtung[i] *= -1;
                }
                setMagnetPWM(i, magnetIntensitaeten[i]);
            }
        }
    } else if (aktiverOverrideModus == OverrideModus::SEQUENZ_OVERRIDE && !maxIntensivPWMMuster.empty()) {
        if (aktuelleZeit - letzteSequenzZeit >= sequenzIntervall) {
            letzteSequenzZeit = aktuelleZeit;
            const auto& aktuellesMuster = maxIntensivPWMMuster[aktiveSequenz % maxIntensivPWMMuster.size()];
            if (!aktuellesMuster.intensitaeten.empty()) {
                for (int i = 0; i < NUM_MAGNETS; ++i) {
                    setMagnetPWM(i, aktuellesMuster.intensitaeten[sequenzSchritt % aktuellesMuster.intensitaeten.size()]);
                }
                setMagnetPWM(STATIC_MAGNET_INDEX, aktuellesMuster.intensitaeten[sequenzSchritt % aktuellesMuster.intensitaeten.size()]);
                sequenzSchritt = (sequenzSchritt + 1) % aktuellesMuster.intensitaeten.size();
            }
        }
    } else if (aktiverOverrideModus == OverrideModus::ALLE_AN_AUS_OVERRIDE) {
        for (int i = 0; i < NUM_MAGNETS; ++i) {
            setMagnetPWM(i, alleAnAusZustand);
        }
        setMagnetPWM(STATIC_MAGNET_INDEX, alleAnAusZustand);
        if (aktuelleZeit - letzteMusterAenderungZeit >= grundMusterIntervall) {
            letzteMusterAenderungZeit = aktuelleZeit;
            alleAnAusZustand = (alleAnAusZustand == 0) ? overridePWM : 0;
        }
    } else if (aktiverOverrideModus == OverrideModus::HALBE_AN_OVERRIDE) {
        if (!halbeAnMuster.empty()) {
            int magnetIndex1 = halbeAnMuster[halbeAnModus % halbeAnMuster.size()];
            int magnetIndex2 = halbeAnMuster[(halbeAnModus + 1) % halbeAnMuster.size()];
            for (int i = 0; i < NUM_MAGNETS; ++i) {
                setMagnetPWM(i, 0);
            }
            setMagnetPWM(STATIC_MAGNET_INDEX, 0);
            setMagnetPWM(magnetIndex1, overridePWM);
            setMagnetPWM(magnetIndex2, overridePWM);
            if (aktuelleZeit - letzteMusterAenderungZeit >= halbeAnIntervall) {
                letzteMusterAenderungZeit = aktuelleZeit;
                halbeAnModus = (halbeAnModus + 1) % halbeAnMuster.size();
            }
        }
    } else if (aktiverOverrideModus == OverrideModus::MUSTER_OVERRIDE && !grundPWMMuster.empty()) {
        if (aktuelleZeit - letzteMusterAenderungZeit >= grundMusterIntervall) {
            letzteMusterAenderungZeit = aktuelleZeit;
            const auto& aktuellesMuster = grundPWMMuster[aktiveSequenz % grundPWMMuster.size()];
            if (!aktuellesMuster.intensitaeten.empty()) {
                for (int i = 0; i < NUM_MAGNETS; ++i) {
                    setMagnetPWM(i, aktuellesMuster.intensitaeten[sequenzSchritt % aktuellesMuster.intensitaeten.size()]);
                }
                setMagnetPWM(STATIC_MAGNET_INDEX, aktuellesMuster.intensitaeten[sequenzSchritt % aktuellesMuster.intensitaeten.size()]);
                sequenzSchritt = (sequenzSchritt + 1) % aktuellesMuster.intensitaeten.size();
            }
        }
    } else if (aktiverOverrideModus == OverrideModus::RUHIG_OVERRIDE) {
        for (int i = 0; i < NUM_MAGNETS; ++i) {
            setMagnetPWM(i, overridePWM);
        }
        setMagnetPWM(STATIC_MAGNET_INDEX, overridePWM);
    } else if (aktiverOverrideModus == OverrideModus::VORRUECK_OVERRIDE) {
        if (aktuelleZeit - letzteVorrueckZeit >= vorrueckIntervall) {
            letzteVorrueckZeit = aktuelleZeit;
            for (int i = 0; i < NUM_MAGNETS; ++i) {
                setMagnetPWM(i, (i == vorrueckIndex) ? overridePWM : 0);
            }
            setMagnetPWM(STATIC_MAGNET_INDEX, (STATIC_MAGNET_INDEX == vorrueckIndex) ? overridePWM : 0);
            vorrueckIndex = (vorrueckIndex + 1) % (NUM_MAGNETS + 1);
        }
    } else if (aktiverOverrideModus == OverrideModus::KEIN_OVERRIDE) {
        for (int i = 0; i < NUM_MAGNETS + 1; ++i) {
            setMagnetPWM(i, 0);
        }
    }
}
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starte Magnetsteuerung...");

    preferences.begin("magnet_settings");
    loadSettings(); // Lade gespeicherte Einstellungen aus Preferences

    WiFi.begin(ssid, password);
    Serial.println("Verbinde mit WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi verbunden! IP-Adresse: " + WiFi.localIP().toString());

    // PWM für steuerbare Magnete (0-6) initialisieren
    for (int i = 0; i < NUM_MAGNETS; i++) {
        ledcSetup(MAGNET_CHANNELS[i][0], pwmFrequency, PWM_RESOLUTION);
        ledcSetup(MAGNET_CHANNELS[i][1], pwmFrequency, PWM_RESOLUTION);
        pinMode(MAGNET_PINS[i][0], OUTPUT);
        pinMode(MAGNET_PINS[i][1], OUTPUT);
        setMagnetPWM(i, 0); // Initial alle Magnete aus
        pinMode(magnetDirectionAPins[i], OUTPUT);
        pinMode(magnetDirectionBPins[i], OUTPUT);
        setMagnetRichtung(i, 0); // Standardrichtung einstellen
    }

    // PWM für statischen Magnet 8 initialisieren
    ledcSetup(staticMagnetChannels[0], pwmFrequency, PWM_RESOLUTION);
    ledcSetup(staticMagnetChannels[1], pwmFrequency, PWM_RESOLUTION);
    pinMode(STATIC_MAGNET_PINS[0], OUTPUT);
    pinMode(STATIC_MAGNET_PINS[1], OUTPUT);
    setMagnetPWM(STATIC_MAGNET_INDEX, 0); // Initial statischen Magnet aus
    pinMode(staticMagnetDirectionAPin, OUTPUT);
    pinMode(staticMagnetDirectionBPin, OUTPUT);
    setMagnetRichtung(STATIC_MAGNET_INDEX, 0); // Standardrichtung einstellen

    loadPatterns(); // Lade gespeicherte Muster aus Preferences

    // Webserver Routen definieren
    server.on("/", HTTP_GET, [grundPWM, joystickMaxIntensitaet, aktiverOverrideModus, overridePWM, joystickAktiv, joystickXInvertieren, joystickYInvertieren, joystickTotbereich, joystickSkalierung, joystickBasisGeschwindigkeit, joystickGeschwindigkeitsAchse, joystickSteuertZyklischeGeschwindigkeit, joystickZyklischeGeschwindigkeitsAchse, joystickZyklischeGeschwindigkeitMin, joystickZyklischeGeschwindigkeitMax, joystickTotbereichOverride, polaritaetsSchwellePositiv, polaritaetsSchwelleNegativ, joystickSteuertPWM, joystickSteuertIntensitaetZ, joystickSteuertGeschwindigkeitW, grundPWMMuster, maxIntensivPWMMuster, joystickPWMMuster, aktuellerModus](AsyncWebServerRequest *request) {
        String html = "<!DOCTYPE html>\n<html><head><title>Magnetsteuerung</title><meta charset=\"UTF-8\"></head><body><h1>Magnetsteuerung</h1>\n";
        html += "<h2>Steuerungsmodus</h2>\n";
        html += "<form action=\"/set\" method=\"get\">\n";
        html += "<input type=\"radio\" id=\"grundmuster\" name=\"modus\" value=\"grundmuster\" " + String((aktuellerModus == SteuerungModus::GRUNDMUSTER_PWM) ? "checked" : "") + " onchange=\"fetch('/set?modus=grundmuster')\">";
        html += "<label for=\"grundmuster\">Grundmuster</label><br>\n";
        html += "<input type=\"radio\" id=\"joystick\" name=\"modus\" value=\"joystick\" " + String((aktuellerModus == SteuerungModus::JOYSTICK_MODUS) ? "checked" : "") + " onchange=\"fetch('/set?modus=joystick')\">";
        html += "<label for=\"joystick\">Joystick</label><br><br>\n";
        html += "</form>\n";

        html += "<h2>PWM-Einstellungen</h2>\n";
        html += "<label for=\"grundpwm\">Grund-PWM (0-255):</label>\n";
        html += "<input type=\"range\" id=\"grundpwm\" min=\"0\" max=\"255\" value=\"" + String(grundPWM) + "\" oninput=\"document.getElementById('grundpwm_value').textContent = this.value; fetch('/set?grundpwm=' + this.value)\"><span id=\"grundpwm_value\">" + String(grundPWM) + "</span>\n";
        html += "<input type=\"number\" id=\"grundpwm_input\" min=\"0\" max=\"255\" value=\"" + String(grundPWM) + "\" oninput=\"document.getElementById('grundpwm').value = this.value; document.getElementById('grundpwm_value').textContent = this.value; fetch('/set?grundpwm=' + this.value);\"> (0-255)<br><br>\n";

        html += "<label for=\"joystickMaxIntensitaet\">Max. Intensität (Joystick) (0-255):</label>\n";
        html += "<input type=\"range\" id=\"joystickMaxIntensitaet\" min=\"0\" max=\"255\" value=\"" + String(joystickMaxIntensitaet) + "\" oninput=\"document.getElementById('joystickMaxIntensitaet_value').textContent = this.value; fetch('/set?joystickpwm=' + this.value)\"><span id=\"joystickMaxIntensitaet_value\">" + String(joystickMaxIntensitaet) + "</span>\n";
        html += "<input type=\"number\" id=\"joystickMaxIntensitaet_input\" min=\"0\" max=\"255\" value=\"" + String(joystickMaxIntensitaet) + "\" oninput=\"document.getElementById('joystickMaxIntensitaet').value = this.value; document.getElementById('joystickMaxIntensitaet_value').textContent = this.value; fetch('/set?joystickpwm=' + this.value);\"> (0-255)<br><br>\n";
        html += "<form action=\"/set\" method=\"get\">\n";
        html += "<label for=\"override\">Modus:</label>\n";
        html += "<select name=\"override\" id=\"override\" onchange=\"fetch('/set?override=' + this.value)\">\n";
        html += "<option value=\"-1\" " + String((aktiverOverrideModus == OverrideModus::KEIN_OVERRIDE) ? "selected" : "") + ">Kein Override</option>\n";
        html += "<option value=\"zyklisch\" " + String((aktiverOverrideModus == OverrideModus::ZYKLISCHER_OVERRIDE) ? "selected" : "") + ">Zyklisch</option>\n";
        html += "<option value=\"vorrueck\" " + String((aktiverOverrideModus == OverrideModus::VORRUECK_OVERRIDE) ? "selected" : "") + ">Vorrück</option>\n";
        html += "<option value=\"ruhig\" " + String((aktiverOverrideModus == OverrideModus::RUHIG_OVERRIDE) ? "selected" : "") + ">Ruhig</option>\n";
        html += "<option value=\"pulsierend\" " + String((aktiverOverrideModus == OverrideModus::PULSIEREND_OVERRIDE) ? "selected" : "") + ">Pulsierend</option>\n";
        html += "<option value=\"welle\" " + String((aktiverOverrideModus == OverrideModus::WELLEN_OVERRIDE) ? "selected" : "") + ">Welle</option>\n";
        html += "<option value=\"zufall\" " + String((aktiverOverrideModus == OverrideModus::ZUFALLS_OVERRIDE) ? "selected" : "") + ">Zufall</option>\n";
        html += "<option value=\"fading\" " + String((aktiverOverrideModus == OverrideModus::FADING_OVERRIDE) ? "selected" : "") + ">Fading</option>\n";
        html += "<option value=\"sequenz\" " + String((aktiverOverrideModus == OverrideModus::SEQUENZ_OVERRIDE) ? "selected" : "") + ">Sequenz</option>\n";
        html += "<option value=\"allean\" " + String((aktiverOverrideModus == OverrideModus::ALLE_AN_AUS_OVERRIDE) ? "selected" : "") + ">Alle An/Aus</option>\n";
        html += "<option value=\"halbean\" " + String((aktiverOverrideModus == OverrideModus::HALBE_AN_OVERRIDE) ? "selected" : "") + ">Halbe An</option>\n";
        html += "<option value=\"muster\" " + String((aktiverOverrideModus == OverrideModus::MUSTER_OVERRIDE) ? "selected" : "") + ">Muster</option>\n";
        html += "</select><br><br>\n";
        html += "<label for=\"overridepwm\">Override PWM (0-255):</label>\n";
        html += "<input type=\"range\" id=\"overridepwm\" min=\"0\" max=\"255\" value=\"" + String(overridePWM) + "\" oninput=\"document.getElementById('overridepwm_value').textContent = this.value; fetch('/set?overridepwm=' + this.value)\"><span id=\"overridepwm_value\">" + String(overridePWM) + "</span>\n";
        html += "</form>\n";
        html += "<input type=\"number\" id=\"overridepwm_input\" min=\"0\" max=\"255\" value=\"" + String(overridePWM) + "\" oninput=\"document.getElementById('overridepwm').value = this.value; document.getElementById('overridepwm_value').textContent = this.value; fetch('/set?overridepwm=' + this.value);\"> (0-255)<br><br>\n";
        // Joystick Einstellungen
        html += "<h2>Joystick Einstellungen</h2>\n";
        html += "<form action=\"/set\" method=\"get\">\n";
        html += "<label for=\"joytickaktiv\">Joystick Aktiv:</label>\n";
        html += "<input type=\"checkbox\" id=\"joytickaktiv\" name=\"joytickaktiv\" value=\"1\" " + String(joystickAktiv ? "checked" : "") + " onchange=\"fetch('/set?joytickaktiv=' + (this.checked ? 1 : 0))\"> <br><br>\n";
        html += "<label for=\"joytickinvertx\">Joystick X invertieren:</label>\n";
        html += "<input type=\"checkbox\" id=\"joytickinvertx\" name=\"joytickinvertx\" value=\"1\" " + String(joystickXInvertieren ? "checked" : "") + " onchange=\"fetch('/set?joytickinvertx=' + (this.checked ? 1 : 0))\"> <br>\n";
        html += "<label for=\"joytickinverty\">Joystick Y invertieren:</label>\n";
        html += "<input type=\"checkbox\" id=\"joytickinverty\" name=\"joytickinverty\" value=\"1\" " + String(joystickYInvertieren ? "checked" : "") + " onchange=\"fetch('/set?joytickinverty=' + (this.checked ? 1 : 0))\"> <br><br>\n";
        html += "<label for=\"joyticktotbereich\">Joystick Totbereich:</label>\n";
        html += "<input type=\"number\" id=\"joyticktotbereich\" name=\"joyticktotbereich\" value=\"" + String(joystickTotbereich) + "\" onchange=\"fetch('/set?joyticktotbereich=' + this.value)\"><br><br>\n";
        html += "<label for=\"joytickskalierung\">Joystick Skalierung:</label>\n";
        html += "<input type=\"number\" step=\"0.1\" id=\"joytickskalierung\" name=\"joytickskalierung\" value=\"" + String(joystickSkalierung, 1) + "\" onchange=\"fetch('/set?joytickskalierung=' + this.value)\"><br><br>\n";
        html += "<label for=\"joytickbasisgeschwindigkeit\">Joystick Basisgeschwindigkeit:</label>\n";
        html += "<input type=\"number\" id=\"joytickbasisgeschwindigkeit\" name=\"joytickbasisgeschwindigkeit\" value=\"" + String(joystickBasisGeschwindigkeit) + "\" onchange=\"fetch('/set?joytickbasisgeschwindigkeit=' + this.value)\"><br><br>\n";
        html += "<label for=\"joytickachsen\">Joystick Geschwindigkeitsachse:</label>\n";
        html += "<select name=\"joytickachsen\" id=\"joytickachsen\" onchange=\"fetch('/set?joytickachsen=' + this.value)\">\n";
        html += "<option value=\"x\" " + String((joystickGeschwindigkeitsAchse == "x") ? "selected" : "") + ">X</option>\n";
        html += "<option value=\"y\" " + String((joystickGeschwindigkeitsAchse == "y") ? "selected" : "") + ">Y</option>\n";
        html += "<option value=\"z\" " + String((joystickGeschwindigkeitsAchse == "z") ? "selected" : "") + ">Z</option>\n";
        html += "<option value=\"w\" " + String((joystickGeschwindigkeitsAchse == "w") ? "selected" : "") + ">W</option>\n";
        html += "</select><br><br>\n";
        html += "<label for=\"joystickCyclicSpeed\">Joystick steuert zykl. Geschw.:</label>\n";
        html += "<input type=\"checkbox\" id=\"joystickCyclicSpeed\" name=\"joystickCyclicSpeed\" value=\"1\" " + String(joystickSteuertZyklischeGeschwindigkeit ? "checked" : "") + " onchange=\"fetch('/set?joystickCyclicSpeed=' + (this.checked ? 1 : 0))\"> <br>\n";
        html += "<label for=\"joystickCyclicSpeedAxis\">Joystick Achse f. zykl. Geschw.:</label>\n";
        html += "<select name=\"joystickCyclicSpeedAxis\" id=\"joystickCyclicSpeedAxis\" onchange=\"fetch('/set?joystickCyclicSpeedAxis=' + this.value)\">\n";
        html += "<option value=\"x\" " + String((joystickZyklischeGeschwindigkeitsAchse == "x") ? "selected" : "") + ">X</option>\n";
        html += "<option value=\"y\" " + String((joystickZyklischeGeschwindigkeitsAchse == "y") ? "selected" : "") + ">Y</option>\n";
        html += "<option value=\"z\" " + String((joystickZyklischeGeschwindigkeitsAchse == "z") ? "selected" : "") + ">Z</option>\n";
        html += "<option value=\"w\" " + String((joystickZyklischeGeschwindigkeitsAchse == "w") ? "selected" : "") + ">W</option>\n";
        html += "</select><br>\n";
        html += "<label for=\"joystickCyclicSpeedMin\">Joystick min. zykl. Geschw.:</label>\n";
        html += "<input type=\"number\" id=\"joystickCyclicSpeedMin\" name=\"joystickCyclicSpeedMin\" value=\"" + String(joystickZyklischeGeschwindigkeitMin) + "\" onchange=\"fetch('/set?joystickCyclicSpeedMin=' + this.value)\"><br>\n";
        html += "<label for=\"joystickCyclicSpeedMax\">Joystick max. zykl. Geschw.:</label>\n";
        html += "<input type=\"number\" id=\"joystickCyclicSpeedMax\" name=\"joystickCyclicSpeedMax\" value=\"" + String(joystickZyklischeGeschwindigkeitMax) + "\" onchange=\"fetch('/set?joystickCyclicSpeedMax=' + this.value)\"><br><br>\n";
        html += "<label for=\"joystickTotzoneOverride\">Joystick Totzone Override:</label>\n";
        html += "<input type=\"number\" step=\"0.01\" id=\"joystickTotzoneOverride\" name=\"joystickTotzoneOverride\" value=\"" + String(joystickTotbereichOverride, 2) + "\" onchange=\"fetch('/set?joystickTotzoneOverride=' + this.value)\"><br>\n";
        html += "<label for=\"polarityThresholdPositive\">Polarität Schwelle Positiv:</label>\n";
        html += "<input type=\"number\" step=\"0.01\" id=\"polarityThresholdPositive\" name=\"polarityThresholdPositive\" value=\"" + String(polaritaetsSchwellePositiv, 2) + "\" onchange=\"fetch('/set?polarityThresholdPositive=' + this.value)\"><br>\n";
        html += "<label for=\"polarityThresholdNegative\">Polarität Schwelle Negativ:</label>\n";
        html += "<input type=\"number\" step=\"0.01\" id=\"polarityThresholdNegative\" name=\"polarityThresholdNegative\" value=\"" + String(polaritaetsSchwelleNegativ, 2) + "\" onchange=\"fetch('/set?polarityThresholdNegative=' + this.value)\"><br><br>\n";
        html += "<label for=\"joystickPWM\">Joystick steuert PWM direkt:</label>\n";
        html += "<input type=\"checkbox\" id=\"joystickPWM\" name=\"joystickPWM\" value=\"1\" " + String(joystickSteuertPWM ? "checked" : "") + " onchange=\"fetch('/set?joystickPWM=' + (this.checked ? 1 : 0))\"> <br>\n";
        html += "<label for=\"joystickIntensityZ\">Joystick steuert Intensität Z:</label>\n";
        html += "<input type=\"checkbox\" id=\"joystickIntensityZ\" name=\"joystickIntensityZ\" value=\"1\" " + String(joystickSteuertIntensitaetZ ? "checked" : "") + " onchange=\"fetch('/set?joystickIntensityZ=' + (this.checked ? 1 : 0))\"> <br>\n";
        html += "<label for=\"joystickSpeedW\">Joystick steuert Geschwindigkeit W:</label>\n";
        html += "<input type=\"checkbox\" id=\"joystickSpeedW\" name=\"joystickSpeedW\" value=\"1\" " + String(joystickSteuertGeschwindigkeitW ? "checked" : "") + " onchange=\"fetch('/set?joystickSpeedW=' + (this.checked ? 1 : 0))\"> <br><br>\n";
        html += "</form>\n";
        html += "<script>\n";
        html += "function saveAllPatterns() {\n";
        html += "    const grundmusterContainer = document.getElementById('grundmuster_container');\n";
        html += "    const joystickmusterContainer = document.getElementById('joystickmuster_container');\n";
        html += "\n";
        html += "    function extractPatterns(container) {\n";
        html += "        const musterArray = [];\n";
        html += "        container.querySelectorAll('div').forEach(musterDiv => {\n";
        html += "            const nameInput = musterDiv.querySelector('input[type=\"text\"]');\n";
        html += "            const intensitaetsInputs = musterDiv.querySelectorAll('input[type=\"number\"]');\n";
        html += "            if (nameInput && intensitaetsInputs.length > 0) {\n";
        html += "                const intensitaeten = Array.from(intensitaetsInputs).map(input => parseInt(input.value));\n";
        html += "                musterArray.push({ name: nameInput.value, schritte: intensitaeten });\n";
        html += "            }\n";
        html += "        });\n";
        html += "        return musterArray;\n";
        html += "    }\n";
        html += "\n";
        html += "    const grundmusterData = extractPatterns(grundmusterContainer);\n";
        html += "    const joystickmusterData = extractPatterns(joystickmusterContainer);\n";
        html += "\n";
        html += "    fetch('/save_patterns', {\n";
        html += "        method: 'POST',\n";
        html += "        headers: {\n";
        html += "            'Content-Type': 'application/json',\n";
        html += "        },\n";
        html += "        body: JSON.stringify({\n";
        html += "            grundmuster: grundmusterData,\n";
        html += "            joystickmuster: joystickmusterData\n";
        html += "        }),\n";
        html += "    });\n";
        html += "}\n";
        html += "\n";
        html += "function addGrundmuster() {\n";
        html += "    const container = document.getElementById('grundmuster_container');\n";
        html += "    const newMusterDiv = document.createElement('div');\n";
        html += "    newMusterDiv.innerHTML = '<input type=\"text\" placeholder=\"Name\"> <input type=\"number\" min=\"0\" max=\"255\"> <button onclick=\"this.parentNode.remove()\">Löschen</button>';\n";
        html += "    container.appendChild(newMusterDiv);\n";
        html += "}\n";
        html += "\n";
        html += "function addJoystickMuster() {\n";
        html += "    const container = document.getElementById('joystickmuster_container');\n";
        html += "    const newMusterDiv = document.createElement('div');\n";
        html += "    newMusterDiv.innerHTML = '<input type=\"text\" placeholder=\"Name\"> <input type=\"number\" min=\"0\" max=\"255\"> <button onclick=\"this.parentNode.remove()\">Löschen</button>';\n";
        html += "    container.appendChild(newMusterDiv);\n";
        html += "}\n";
        html += "</script>\n";

        // Grundmuster Verwaltung
        html += "<h2>Grundmuster Verwaltung</h2>\n";
        html += "<div id=\"grundmuster_container\">\n";
        for (const auto& muster : grundPWMMuster) {
            html += "<div><input type=\"text\" placeholder=\"Name\" value=\"" + muster.name + "\">";
            for (int intensitaet : muster.intensitaeten) {
                html += "<input type=\"number\" min=\"0\" max=\"255\" value=\"" + String(intensitaet) + "\">";
            }
            html += "<button onclick=\"this.parentNode.remove();\">Löschen</button></div>\n";
        }
        html += "</div>\n";
        html += "<button onclick=\"addGrundmuster()\">Neues Grundmuster hinzufügen</button><br><br>\n";
        html += "<button onclick=\"saveAllPatterns()\">Alle Muster speichern</button><br><br>\n";

        // Joystick Muster Verwaltung
        html += "<h2>Joystick Muster Verwaltung</h2>\n";
        html += "<div id=\"joystickmuster_container\">\n";
        for (const auto& muster : joystickPWMMuster) {
            html += "<div><input type=\"text\" placeholder=\"Name\" value=\"" + muster.name + "\">";
            for (int intensitaet : muster.intensitaeten) {
                html += "<input type=\"number\" min=\"0\" max=\"255\" value=\"" + String(intensitaet) + "\">";
            }
            html += "<button onclick=\"this.parentNode.remove();\">Löschen</button></div>\n";
        }
        html += "</div>\n";
        html += "<button onclick=\"addJoystickMuster()\">Neues Joystick Muster hinzufügen</button><br><br>\n";
        html += "<button onclick=\"saveAllPatterns()\">Alle Muster speichern</button><br><br>\n";

        html += "</body></html>\n";
        request->send(200, "text/html", html);
    });

    server.on("/save_patterns", HTTP_POST, handleSavePatterns);
    server.on("/set", HTTP_GET, handleSet);
    server.on("/set", HTTP_POST, handleSet);
    server.on("/save_all", HTTP_GET, handleSaveAll); // Route zum Speichern aller Muster
    server.on("/get_patterns", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument root(16384); // Größeres Dokument für alle Muster
        JsonArray grundArray = root.createNestedArray("grundmuster");
        for (const auto& pattern : grundPWMMuster) {
            JsonObject patternObj = grundArray.createNestedObject();
            patternObj["name"] = pattern.name;
            JsonArray intensitaeten = patternObj.createNestedArray("schritte");
            for (int intensitaet : pattern.intensitaeten) {
                intensitaeten.add(intensitaet);
            }
        }
        JsonArray maxIntensivArray = root.createNestedArray("maxintensivmuster");
        for (const auto& pattern : maxIntensivPWMMuster) {
            JsonObject patternObj = maxIntensivArray.createNestedObject();
            patternObj["name"] = pattern.name;
            JsonArray intensitaeten = patternObj.createNestedArray("schritte");
            for (int intensitaet : pattern.intensitaeten) {
                intensitaeten.add(intensitaet);
            }
        }
        JsonArray joystickArray = root.createNestedArray("joystickmuster");
        for (const auto& pattern : joystickPWMMuster) {
            JsonObject patternObj = joystickArray.createNestedObject();
            patternObj["name"] = pattern.name;
            JsonArray intensitaeten = patternObj.createNestedArray("schritte");
            for (int intensitaet : pattern.intensitaeten) {
                intensitaeten.add(intensitaet);
            }
        }
        String jsonString;
        serializeJson(root, jsonString);
        request->send(200, "application/json", jsonString);
    });

    server.begin();
    Serial.println("Webserver gestartet.");
}
void loop() {
    updateMagnets();
    delay(1); // Kleine Verzögerung, um die CPU nicht zu überlasten
}