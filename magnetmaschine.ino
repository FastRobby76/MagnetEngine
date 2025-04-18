#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <cmath>
#include <random> // Für Zufallszahlen

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);

const int joystickXPins[] = {36, 39};
const int joystickYPins[] = {34, 35};
const int joystickZPins[] = {32, 33};
const int joystickWPins[] = {25, 26};

const int magnetPins[7][2] = {{16, 17}, {18, 19}, {21, 22}, {23, 4}, {2, 15}, {13, 12}, {14, 27}};
const int magnet8Pins[2] = {5, 3};
const int magnetChannels[7][2] = {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9}, {10, 11}, {12, 13}};
int switchPattern[3][7] = {{255, 0, 0, 0, 0, 0, 0}, {0, 255, 0, 0, 0, 0, 0}, {0, 0, 255, 0, 0, 0, 0}};

enum OverrideMode {
  NO_OVERRIDE = -1,
  CYCLIC_OVERRIDE = 0,
  FORWARD_OVERRIDE = 1,
  QUIET_OVERRIDE = 2,
  PULSE_OVERRIDE = 3,
  WAVE_OVERRIDE = 4,
  RANDOM_OVERRIDE = 5,
  FADE_OVERRIDE = 6,
  SEQUENCE_OVERRIDE = 7,
  ALL_ON_OFF_OVERRIDE = 8,
  HALF_ON_OVERRIDE = 9
};

OverrideMode activeOverrideMode = NO_OVERRIDE;
int patternIndex = 0;
int grundPWM = 200;
int maxIntensivPWM = 255;
int overridePWM = 200;
float joystickTotzone = 0.05;
long minimalAenderungsIntervall = 100;
unsigned long lastPatternChangeTime = 0;
long grundPatternInterval = 1000;
unsigned long lastGrundPatternChangeTime = 0;
int grundPatternIndex = 0;
int pwmFrequency = 120;
const int pwmResolution = 8;
Preferences preferences;
long pulseInterval = 500;
long waveSpeed = 200;
long randomIntervalMin = 100;
long randomIntervalMax = 1000;
long fadeDuration = 1000;
unsigned long fadeStartTime = 0;
int fadeDirection[8] = {1, 1, 1, 1, 1, 1, 1, 1}; // 1 für hoch, -1 für runter
int activeSequence = 0;
int sequences[2][7] = {{255, 0, 255, 0, 255, 0, 255}, {0, 255, 0, 255, 0, 255, 0}};
long sequenceInterval = 500;
unsigned long lastSequenceTime = 0;
int sequenceStep = 0;
int halfOnMode = 0;
int allOnOffState = 0;

// Neue globale Variablen für die Joystick-Steuerung der zyklischen Geschwindigkeit
bool joystickControlsCyclicSpeed = false;
String joystickCyclicSpeedAxis = "x";
long joystickCyclicSpeedMin = 100;
long joystickCyclicSpeedMax = 5000;

// Neue globale Variable für die Intensität von Magnet 8
int magnet8Intensity = 255;
float joystickTotzoneOverride = 0.1; // Totzone für Override-Steuerung

int getOverrideModeFromString(String name) {
  name.toLowerCase();
  if (name == "zyklisch") return CYCLIC_OVERRIDE;
  if (name == "vorrueck") return FORWARD_OVERRIDE;
  if (name == "ruhig") return QUIET_OVERRIDE;
  if (name == "pulsierend") return PULSE_OVERRIDE;
  if (name == "welle") return WAVE_OVERRIDE;
  if (name == "zufall") return RANDOM_OVERRIDE;
  if (name == "fading") return FADE_OVERRIDE;
  if (name == "sequenz") return SEQUENCE_OVERRIDE;
  if (name == "allean") return ALL_ON_OFF_OVERRIDE;
  if (name == "halbean") return HALF_ON_OVERRIDE;
  if (name == "kein") return NO_OVERRIDE;
  return -2;
}

void handleSet() {
  bool updated = false;
  if (server.hasArg("grundpwm")) {
    grundPWM = server.arg("grundpwm").toInt();
    grundPWM = constrain(grundPWM, 0, 255);
    preferences.putInt("grundPWM", grundPWM);
    updated = true;
  }
  if (server.hasArg("maxintensivpwm")) {
    maxIntensivPWM = server.arg("maxintensivpwm").toInt();
    maxIntensivPWM = constrain(maxIntensivPWM, 0, 255);
    preferences.putInt("maxIntensivPWM", maxIntensivPWM);
    updated = true;
  }
  if (server.hasArg("override")) {
    String arg = server.arg("override");
    int newOverrideMode = getOverrideModeFromString(arg);
    if (newOverrideMode != -2) {
      activeOverrideMode = static_cast<OverrideMode>(newOverrideMode);
      preferences.putInt("overrideMode", activeOverrideMode);
      updated = true;
    } else {
      Serial.println("Ungültiger Override-Modus.");
      server.send(400, "text/plain", "Ungültiger Override-Modus.");
      return;
    }
  }
  if (server.hasArg("overridepwm")) {
    overridePWM = server.arg("overridepwm").toInt();
    overridePWM = constrain(overridePWM, 0, 255);
    preferences.putInt("overridePWM", overridePWM);
    updated = true;
  }
  if (server.hasArg("pwmfrequency")) {
    int newFrequency = server.arg("pwmfrequency").toInt();
    if (newFrequency > 0 && newFrequency <= 1000) {
      pwmFrequency = newFrequency;
      preferences.putInt("pwmFrequency", pwmFrequency);
      updated = true;
    } else {
      Serial.println("Ungültige PWM-Frequenz. Bereich: 1-1000 Hz.");
      server.send(400, "text/plain", "Ungültige PWM-Frequenz.");
      return;
    }
  }
  if (server.hasArg("grundpatterninterval")) {
    long newInterval = server.arg("grundpatterninterval").toInt();
    if (newInterval > 0) {
      grundPatternInterval = newInterval;
      preferences.putLong("grundPatternInterval", grundPatternInterval);
      updated = true;
    } else {
      Serial.println("Ungültiges Grundmuster-Intervall.");
      server.send(400, "text/plain", "Ungültiges Grundmuster-Intervall.");
      return;
    }
  }
  if (server.hasArg("pulsintervall")) {
    pulseInterval = server.arg("pulsintervall").toInt();
    pulseInterval = constrain(pulseInterval, 50, 5000);
    preferences.putLong("pulseInterval", pulseInterval);
    updated = true;
  }
  if (server.hasArg("zufallmin")) {
    randomIntervalMin = server.arg("zufallmin").toInt();
    randomIntervalMin = constrain(randomIntervalMin, 50, 5000);
    preferences.putLong("randomIntervalMin", randomIntervalMin);
    updated = true;
  }
  if (server.hasArg("zufallmax")) {
    randomIntervalMax = server.arg("zufallmax").toInt();
    randomIntervalMax = constrain(randomIntervalMax, 100, 10000);
    preferences.putLong("randomIntervalMax", randomIntervalMax);
    updated = true;
  }
  if (server.hasArg("welle")) { // Hier wird der Wert für 'welle' entgegengenommen
    waveSpeed = server.arg("welle").toInt();
    waveSpeed = constrain(waveSpeed, 50, 2000);
    preferences.putLong("waveSpeed", waveSpeed);
    updated = true;
  }
  if (server.hasArg("halbeanmodus")) {
    halfOnMode = server.arg("halbeanmodus").toInt();
    halfOnMode = constrain(halfOnMode, 0, 1);
    preferences.putInt("halfOnMode", halfOnMode);
    updated = true;
  }
  if (server.hasArg("alleanstatus")) {
    allOnOffState = server.arg("alleanstatus").toInt();
    allOnOffState = constrain(allOnOffState, 0, 255);
    preferences.putInt("allOnOffState", allOnOffState);
    updated = true;
  }
  if (server.hasArg("fade")) {
    fadeDuration = server.arg("fade").toInt();
    fadeDuration = constrain(fadeDuration, 100, 10000);
    preferences.putLong("fadeDuration", fadeDuration);
    updated = true;
  }
  if (server.hasArg("sequenz")) {
    activeSequence = server.arg("sequenz").toInt();
    activeSequence = constrain(activeSequence, 0, 1); // Angenommen 2 Sequenzen
    preferences.putInt("activeSequence", activeSequence);
    updated = true;
  }
  if (server.hasArg("sequenzintervall")) {
    sequenceInterval = server.arg("sequenzintervall").toInt();
    sequenceInterval = constrain(sequenceInterval, 50, 2000);
    preferences.putLong("sequenceInterval", sequenceInterval);
    updated = true;
  }
  if (server.hasArg("joystickCyclicSpeed")) {
    joystickControlsCyclicSpeed = (server.arg("joystickCyclicSpeed") == "on");
    preferences.putBool("joystickCyclicSpeed", joystickControlsCyclicSpeed);
    updated = true;
  }
  if (server.hasArg("joystickCyclicSpeedAxis")) {
    joystickCyclicSpeedAxis = server.arg("joystickCyclicSpeedAxis");
    preferences.putString("joystickCyclicSpeedAxis", joystickCyclicSpeedAxis);
    updated = true;
  }
  if (server.hasArg("joystickCyclicSpeedMin")) {
    joystickCyclicSpeedMin = server.arg("joystickCyclicSpeedMin").toInt();
    preferences.putLong("joystickCyclicSpeedMin", joystickCyclicSpeedMin);
    updated = true;
  }
  if (server.hasArg("joystickCyclicSpeedMax")) {
    joystickCyclicSpeedMax = server.arg("joystickCyclicSpeedMax").toInt();
    preferences.putLong("joystickCyclicSpeedMax", joystickCyclicSpeedMax);
    updated = true;
  }
  // Neuer Parameter für die Intensität von Magnet 8
  if (server.hasArg("magnet8intensity")) {
    magnet8Intensity = server.arg("magnet8intensity").toInt();
    magnet8Intensity = constrain(magnet8Intensity, 0, 255);
    preferences.putInt("magnet8Intensity", magnet8Intensity);
    updated = true;
  }
  if (server.hasArg("joystickTotzoneOverride")) {
    joystickTotzoneOverride = server.arg("joystickTotzoneOverride").toFloat();
    joystickTotzoneOverride = constrain(joystickTotzoneOverride, 0.0f, 1.0f);
    preferences.putFloat("joystickTotzoneOverride", joystickTotzoneOverride);
    updated = true;
  }

  if (updated) {
    for (int i = 0; i < 7; i++) {
      ledcSetup(magnetChannels[i][0], pwmFrequency, pwmResolution);
      ledcSetup(magnetChannels[i][1], pwmFrequency, pwmResolution);
    }
  }
  server.send(200, "text/plain", "OK");
}

void setMagnetPWM(int magnetIndex, int pwmValue) {
  if (magnetIndex >= 0 && magnetIndex < 7) {
    ledcWrite(magnetChannels[magnetIndex][0], pwmValue);
    ledcWrite(magnetChannels[magnetIndex][1], pwmValue);
  }
}

void setMagnet8(int value) {
  digitalWrite(magnet8Pins[0], value > 127 ? HIGH : LOW);
  digitalWrite(magnet8Pins[1], value > 127 ? HIGH : LOW);
}

void updateMagnets() {
  unsigned long currentTime = millis();
  int joystickX = analogRead(joystickXPins[0]);
  int joystickY = analogRead(joystickYPins[0]);
  float normalizedX = (float)(joystickX - 2048) / 2048.0;
  float normalizedY = (float)(joystickY - 2048) / 2048.0;

  if (activeOverrideMode == NO_OVERRIDE) {
    for (int i = 0; i < 7; i++) {
      float influenceX = cos(2 * PI * i / 7.0);
      float influenceY = sin(2 * PI * i / 7.0);
      float control = grundPWM + (maxIntensivPWM - grundPWM) * constrain(influenceX * normalizedX + influenceY * normalizedY, -1.0, 1.0);
      setMagnetPWM(i, static_cast<int>(control));
    }
    setMagnet8(magnet8Intensity);
  } else {
    switch (activeOverrideMode) {
      case CYCLIC_OVERRIDE: {
        long currentInterval = grundPatternInterval;
        float currentIntensityFactor = 1.0;

        // Langsame, kontinuierliche Rotation innerhalb der Totzone könnte die Grundgeschwindigkeit leicht beeinflussen
        if (abs(normalizedX) < joystickTotzoneOverride && abs(normalizedY) < joystickTotzoneOverride) {
          // Sanfte Anpassung des Intervalls basierend auf der leichten Auslenkung (Beispiel)
          float speedFactorX = map(normalizedX, -joystickTotzoneOverride, joystickTotzoneOverride, 1.1f, 0.9f);
          float speedFactorY = map(normalizedY, -joystickTotzoneOverride, joystickTotzoneOverride, 1.1f, 0.9f);
          currentInterval = static_cast<long>(grundPatternInterval * (speedFactorX * 0.5 + speedFactorY * 0.5));
          currentInterval = constrain(currentInterval, static_cast<long>(grundPatternInterval * 0.5), static_cast<long>(grundPatternInterval * 2));
        } else {
          // Überlagerungs-Aktionen bei signifikanter Auslenkung
          if (normalizedY > joystickTotzoneOverride) {
            // Joystick nach oben: Intervall schneller (stärker)
            currentInterval = static_cast<long>(grundPatternInterval * 0.3);
          } else if (normalizedY < -joystickTotzoneOverride) {
            // Joystick nach unten: Intervall langsamer (stärker)
            currentInterval = static_cast<long>(grundPatternInterval * 3.0);
          }
          if (normalizedX > joystickTotzoneOverride) {
            // Joystick nach rechts: Nächstes Muster
            patternIndex = (patternIndex + 1) % 3;
            lastPatternChangeTime = currentTime; // Sofortige Änderung
          } else if (normalizedX < -joystickTotzoneOverride) {
            // Joystick nach links: Vorheriges Muster
            patternIndex = (patternIndex - 1 + 3) % 3;
            lastPatternChangeTime = currentTime; // Sofortige Änderung
          }
          // Zusätzliche Logik für Intensitätsüberlagerung bei starker Auslenkung
          currentIntensityFactor = 1.0 + max(abs(normalizedX), abs(normalizedY)) * 0.8; // Stärkere Intensitätsänderung
          currentIntensityFactor = constrain(currentIntensityFactor, 0.2f, 2.0f); // Begrenzung des Faktors
        }

        if (currentTime - lastPatternChangeTime >= currentInterval) {
          lastPatternChangeTime = currentTime;
          for (int i = 0; i < 7; i++) {
            setMagnetPWM(i, static_cast<int>(switchPattern[patternIndex][i] * overridePWM * currentIntensityFactor / 255.0f));
          }
          setMagnet8(static_cast<int>(magnet8Intensity * overridePWM * currentIntensityFactor / 255.0f));
        } else {
          // Beibehalte die Intensitätsüberlagerung auch zwischen den Musterwechseln
          for (int i = 0; i < 7; i++) {
            setMagnetPWM(i, static_cast<int>(switchPattern[patternIndex][i] * overridePWM * currentIntensityFactor / 255.0f));
          }
          setMagnet8(static_cast<int>(magnet8Intensity * overridePWM * currentIntensityFactor / 255.0f));
        }
        break;
      }
      case FORWARD_OVERRIDE: {
        float intensityFactor = 1.0;
        if (abs(normalizedX) > joystickTotzoneOverride || abs(normalizedY) > joystickTotzoneOverride) {
          intensityFactor = 1.0 + max(abs(normalizedX), abs(normalizedY)) * 0.8;
          intensityFactor = constrain(intensityFactor, 0.2f, 2.0f);
        }
        if (currentTime - lastPatternChangeTime >= grundPatternInterval) {
          lastPatternChangeTime = currentTime;
          for (int i = 0; i < 7; i++) {
            setMagnetPWM(i, static_cast<int>((i == patternIndex) ? (overridePWM * intensityFactor) : 0));
          }
          patternIndex = (patternIndex + 1) % 7;
        }
        setMagnet8(static_cast<int>(magnet8Intensity * overridePWM * intensityFactor / 255.0f));
        break;
      }
      case QUIET_OVERRIDE: {
        float intensityFactor = 1.0;
        if (abs(normalizedX) > joystickTotzoneOverride || abs(normalizedY) > joystickTotzoneOverride) {
          intensityFactor = 1.0 + max(abs(normalizedX), abs(normalizedY)) * 0.8;
          intensityFactor = constrain(intensityFactor, 0.2f, 2.0f);
          overridePWM = constrain(static_cast<int>(preferences.getInt("overridePWM", 200) * intensityFactor), 0, 255);
        } else {
          overridePWM = preferences.getInt("overridePWM", 200);
        }
        for (int i = 0; i < 7; i++) {
          setMagnetPWM(i, overridePWM);
        }
        setMagnet8(static_cast<int>(magnet8Intensity * overridePWM / 255.0f));
        break;
      }
      case PULSE_OVERRIDE: {
        float intensityFactor = 1.0;
        float pulseSpeedFactor = 1.0;
        if (abs(normalizedX) > joystickTotzoneOverride) {
          pulseSpeedFactor = 1.0 + normalizedX * 0.8;
          pulseSpeedFactor = constrain(pulseSpeedFactor, 0.2f, 3.0f);
        }
        if (abs(normalizedY) > joystickTotzoneOverride) {
          intensityFactor = 1.0 + abs(normalizedY) * 0.8;
          intensityFactor = constrain(intensityFactor, 0.2f, 2.0f);
        }
        long currentPulseInterval = static_cast<long>(pulseInterval / pulseSpeedFactor);
        float factor = 0.5 * (1 + sin(2 * PI * currentTime / currentPulseInterval));
        int pwm = static_cast<int>(grundPWM + factor * (overridePWM * intensityFactor - grundPWM));
        for (int i = 0; i < 7; i++) {
          setMagnetPWM(i, pwm);
        }
        setMagnet8(static_cast<int>(magnet8Intensity * overridePWM * intensityFactor / 255.0f));
        break;
      }
      case WAVE_OVERRIDE: {
        float waveSpeedFactor = 1.0;
        float intensityFactor = 1.0;
        if (abs(normalizedX) > joystickTotzoneOverride) {
          waveSpeedFactor = 1.0 + normalizedX * 0.8;
          waveSpeedFactor = constrain(waveSpeedFactor, 0.2f, 3.0f);
        }
        if (abs(normalizedY) > joystickTotzoneOverride) {
          intensityFactor = 1.0 + abs(normalizedY) * 0.8;
          intensityFactor = constrain(intensityFactor, 0.2f, 2.0f);
        }
        long currentWaveSpeed = static_cast<long>(waveSpeed / waveSpeedFactor);
        for (int i = 0; i < 7; i++) {
          float phase = 2 * PI * i / 7.0;
          float factor = 0.5 * (1 + sin(2 * PI * currentTime / currentWaveSpeed + phase));
          setMagnetPWM(i, static_cast<int>(grundPWM + factor * (overridePWM * intensityFactor - grundPWM)));
        }
        setMagnet8(static_cast<int>(magnet8Intensity * overridePWM * intensityFactor / 255.0f));
        break;
      }
      case RANDOM_OVERRIDE: {
        float speedFactor = 1.0;
        float intensityFactor = 1.0;
        if (abs(normalizedX) > joystickTotzoneOverride) {
          speedFactor = 1.0 + normalizedX * 0.8;
          speedFactor = constrain(speedFactor, 0.2f, 3.0f);
        }
        if (abs(normalizedY) > joystickTotzoneOverride) {
          intensityFactor = 1.0 + abs(normalizedY) * 0.8;
          intensityFactor = constrain(intensityFactor, 0.2f, 2.0f);
        }
        long currentRandomInterval = random(static_cast<long>(randomIntervalMin / speedFactor), static_cast<long>(randomIntervalMax / speedFactor) + 1);
        if (currentTime - lastPatternChangeTime >= currentRandomInterval) {
          lastPatternChangeTime = currentTime;
          for (int i = 0; i < 7; i++) {
            setMagnetPWM(i, random(0, static_cast<int>(overridePWM * intensityFactor) + 1));
          }
        }
        setMagnet8(static_cast<int>(magnet8Intensity * overridePWM * intensityFactor / 255.0f));
        break;
      }
      case FADE_OVERRIDE: {
        float speedFactor = 1.0;
        float intensityFactor = 1.0;
        if (abs(normalizedX) > joystickTotzoneOverride) {
          speedFactor = 1.0 + normalizedX * 0.8;
          speedFactor = constrain(speedFactor, 0.2f, 3.0f);
        }
        if (abs(normalizedY) > joystickTotzoneOverride) {
          intensityFactor = 1.0 + abs(normalizedY) * 0.8;
          intensityFactor = constrain(intensityFactor, 0.2f, 2.0f);
        }
        long currentFadeDuration = static_cast<long>(fadeDuration / speedFactor);
        if (currentTime - fadeStartTime >= currentFadeDuration) {
          fadeStartTime = currentTime;
          for (int i = 0; i < 7; i++) {
            fadeDirection[i] *= -1;
          }
        }
        for (int i = 0; i < 7; i++) {
          float progress = (float)(currentTime - fadeStartTime) / currentFadeDuration;
          int delta = static_cast<int>(255 * progress);
          int pwmValue = constrain(grundPWM + fadeDirection[i] * delta, 0, static_cast<int>(255 * intensityFactor));
          setMagnetPWM(i, pwmValue);
        }
        setMagnet8(static_cast<int>(magnet8Intensity * overridePWM * intensityFactor / 255.0f));
        break;
      }
      case SEQUENCE_OVERRIDE: {
        float speedFactor = 1.0;
        float intensityFactor = 1.0;
        if (abs(normalizedX) > joystickTotzoneOverride) {
          speedFactor = 1.0 + normalizedX * 0.8;
          speedFactor = constrain(speedFactor, 0.2f, 3.0f);
        }
        if (abs(normalizedY) > joystickTotzoneOverride) {
          intensityFactor = 1.0 + abs(normalizedY) * 0.8;
          intensityFactor = constrain(intensityFactor, 0.2f, 2.0f);
        }
        long currentSequenceInterval = static_cast<long>(sequenceInterval / speedFactor);
        if (currentTime - lastSequenceTime >= currentSequenceInterval) {
          lastSequenceTime = currentTime;
          for (int i = 0; i < 7; i++) {
            setMagnetPWM(i, static_cast<int>(sequences[activeSequence][i] * intensityFactor));
          }
          sequenceStep = (sequenceStep + 1) % 7; // Für 7-stufige Sequenz
        }
        setMagnet8(static_cast<int>(magnet8Intensity * overridePWM * intensityFactor / 255.0f));
        break;
      }
      case ALL_ON_OFF_OVERRIDE: {
        float intensityFactor = 1.0;
        if (abs(normalizedX) > joystickTotzoneOverride || abs(normalizedY) > joystickTotzoneOverride) {
          intensityFactor = 1.0 + max(abs(normalizedX), abs(normalizedY)) * 0.8;
          intensityFactor = constrain(intensityFactor, 0.2f, 2.0f);
          allOnOffState = constrain(static_cast<int>(preferences.getInt("allOnOffState", 255) * intensityFactor), 0, 255);
        } else {
          allOnOffState = preferences.getInt("allOnOffState", 255);
        }
        for (int i = 0; i < 7; i++) {
          setMagnetPWM(i, allOnOffState);
        }
        setMagnet8(static_cast<int>(magnet8Intensity * allOnOffState / 255.0f));
        break;
      }
      case HALF_ON_OVERRIDE: {
        float intensityFactor = 1.0;
        if (abs(normalizedX) > joystickTotzoneOverride || abs(normalizedY) > joystickTotzoneOverride) {
          intensityFactor = 1.0 + max(abs(normalizedX), abs(normalizedY)) * 0.8;
          intensityFactor = constrain(intensityFactor, 0.2f, 2.0f);
          overridePWM = constrain(static_cast<int>(preferences.getInt("overridePWM", 200) * intensityFactor), 0, 255);
        } else {
          overridePWM = preferences.getInt("overridePWM", 200);
        }
        for (int i = 0; i < 7; i++) {
          setMagnetPWM(i, (i % 2 == halfOnMode) ? overridePWM : 0);
        }
        setMagnet8(static_cast<int>(magnet8Intensity * overridePWM / 255.0f));
        break;
      }
      default:
        break;
    }
  }
}

void setup() {
  Serial.begin(115200);
  preferences.begin("magnet_control");

  grundPWM = preferences.getInt("grundPWM", 200);
  maxIntensivPWM = preferences.getInt("maxIntensivPWM", 255);
  activeOverrideMode = static_cast<OverrideMode>(preferences.getInt("overrideMode", -1));
  overridePWM = preferences.getInt("overridePWM", 200);
  pwmFrequency = preferences.getInt("pwmFrequency", 120);
  grundPatternInterval = preferences.getLong("grundPatternInterval", 1000);
  pulseInterval = preferences.getLong("pulseInterval", 500);
  waveSpeed = preferences.getLong("waveSpeed", 200);
  randomIntervalMin = preferences.getLong("randomIntervalMin", 100);
  randomIntervalMax = preferences.getLong("randomIntervalMax", 1000);
  fadeDuration = preferences.getLong("fadeDuration", 1000);
  activeSequence = preferences.getInt("activeSequence", 0);
  sequenceInterval = preferences.getLong("sequenceInterval", 500);
  halfOnMode = preferences.getInt("halfOnMode", 0);
  allOnOffState = preferences.getInt("allOnOffState", 0);
  joystickControlsCyclicSpeed = preferences.getBool("joystickCyclicSpeed", false);
  joystickCyclicSpeedAxis = preferences.getString("joystickCyclicSpeedAxis", "x");
  joystickCyclicSpeedMin = preferences.getLong("joystickCyclicSpeedMin", 100);
  joystickCyclicSpeedMax = preferences.getLong("joystickCyclicSpeedMax", 5000);
  // Laden der gespeicherten Intensität für Magnet 8 oder Standardwert setzen
  magnet8Intensity = preferences.getInt("magnet8Intensity", 255);
  joystickTotzoneOverride = preferences.getFloat("joystickTotzoneOverride", 0.1f);

  WiFi.begin(ssid, password);
  Serial.println("Verbinde mit WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi verbunden!");

  for (int i = 0; i < 7; i++) {
    ledcSetup(magnetChannels[i][0], pwmFrequency, pwmResolution);
    ledcSetup(magnetChannels[i][1], pwmFrequency, pwmResolution);
    pinMode(magnetPins[i][0], OUTPUT);
    pinMode(magnetPins[i][1], OUTPUT);
  }
  pinMode(magnet8Pins[0], OUTPUT);
  pinMode(magnet8Pins[1], OUTPUT);

  server.on("/", HTTP_GET, []() {
    String html = "<!DOCTYPE html>\n"
                  "<html>\n"
                  "<head>\n"
                  "  <title>Magnetsteuerung</title>\n"
                  "</head>\n"
                  "<body>\n"
                  "  <h1>Magnetsteuerung</h1>\n"
                  "\n"
                  "  <h2>PWM-Einstellungen</h2>\n"
                  "  <label for=\"grundpwm\">Grund-PWM (0-255):</label>\n"
                  "  <input type=\"range\" id=\"grundpwm\" min=\"0\" max=\"255\" value=\"" + String(grundPWM) + "\" oninput=\"document.getElementById('grundpwm_value').textContent = this.value; document.getElementById('grundpwm_input').value = this.value;\"><span id=\"grundpwm_value\">" + String(grundPWM) + "</span>\n"
                  "  <input type=\"number\" id=\"grundpwm_input\" min=\"0\" max=\"255\" value=\"" + String(grundPWM) + "\" oninput=\"document.getElementById('grundpwm').value = this.value; document.getElementById('grundpwm_value').textContent = this.value;\"> (0-255)<br><br>\n"
                  "\n"
                  "  <label for=\"maxintensivpwm\">Max. Intensität (Joystick) (0-255):</label>\n"
                  "  <input type=\"number\" id=\"maxintensivpwm_input\" min=\"0\" max=\"255\" value=\"" + String(maxIntensivPWM) + "\" oninput=\"document.getElementById('maxintensivpwm').value = this.value; document.getElementById('maxintensivpwm_value').textContent = this.value;\"> (0-255)<br><br>\n"                  "\n"
                  "  <label for=\"overridepwm\">Override PWM (0-255):</label>\n"
                  "  <input type=\"range\" id=\"overridepwm\" min=\"0\" max=\"255\" value=\"" + String(overridePWM) + "\" oninput=\"document.getElementById('overridepwm_value').textContent = this.value; document.getElementById('overridepwm_input').value = this.value;\"><span id=\"overridepwm_value\">" + String(overridePWM) + "</span>\n"
                  "  <input type=\"number\" id=\"overridepwm_input\" min=\"0\" max=\"255\" value=\"" + String(overridePWM) + "\" oninput=\"document.getElementById('overridepwm').value = this.value; document.getElementById('overridepwm_value').textContent = this.value;\"> (0-255)<br><br>\n"
                  "\n"
                  "  <label for=\"pwmfrequenz\">PWM Frequenz (Hz) (1-1000):</label>\n"
                  "  <input type=\"number\" id=\"pwmfrequenz\" min=\"1\" max=\"1000\" value=\"" + String(pwmFrequency) + "\"> (1-1000)<br><br>\n"
                  "\n"
                  "  <h2>Override-Modus</h2>\n"
                  "  <select id=\"override_modus\">\n"
                  "    <option value=\"kein\"" + (activeOverrideMode == NO_OVERRIDE ? " selected" : "") + ">Kein Override (Joystick)</option>\n"
                  "    <option value=\"zyklisch\"" + (activeOverrideMode == CYCLIC_OVERRIDE ? " selected" : "") + ">Zyklisch</option>\n"
                  "    <option value=\"vorrueck\"" + (activeOverrideMode == FORWARD_OVERRIDE ? " selected" : "") + ">Vorrück</option>\n"
                  "    <option value=\"ruhig\"" + (activeOverrideMode == QUIET_OVERRIDE ? " selected" : "") + ">Ruhig</option>\n"
                  "    <option value=\"pulsierend\"" + (activeOverrideMode == PULSE_OVERRIDE ? " selected" : "") + ">Pulsierend</option>\n"
                  "    <option value=\"welle\"" + (activeOverrideMode == WAVE_OVERRIDE ? " selected" : "") + ">Welle</option>\n"
                  "    <option value=\"zufall\"" + (activeOverrideMode == RANDOM_OVERRIDE ? " selected" : "") + ">Zufall</option>\n"
                  "    <option value=\"fading\"" + (activeOverrideMode == FADE_OVERRIDE ? " selected" : "") + ">Fading</option>\n"
                  "    <option value=\"sequenz\"" + (activeOverrideMode == SEQUENCE_OVERRIDE ? " selected" : "") + ">Sequenz</option>\n"
                  "    <option value=\"allean\"" + (activeOverrideMode == ALL_ON_OFF_OVERRIDE ? " selected" : "") + ">Alle An/Aus</option>\n"
                  "    <option value=\"halbean\"" + (activeOverrideMode == HALF_ON_OVERRIDE ? " selected" : "") + ">Halbe An</option>\n"
                  "  </select><br><br>\n"
                  "\n"
                  "  <h2>Einstellungen für Magnet 8</h2>\n"
                  "  <label for=\"magnet8intensity\">Intensität Magnet 8 (0-255):</label>\n"
                  "  <input type=\"range\" id=\"magnet8intensity\" min=\"0\" max=\"255\" value=\"" + String(magnet8Intensity) + "\" oninput=\"document.getElementById('magnet8intensity_value').textContent = this.value; document.getElementById('magnet8intensity_input').value = this.value;\"><span id=\"magnet8intensity_value\">" + String(magnet8Intensity) + "</span>\n"
                  "  <input type=\"number\" id=\"magnet8intensity_input\" min=\"0\" max=\"255\" value=\"" + String(magnet8Intensity) + "\" oninput=\"document.getElementById('magnet8intensity').value = this.value; document.getElementById('magnet8intensity_value').textContent = this.value;\"> (0-255)<br><br>\n"
                  "\n"
                  "  <h2>Joystick Override Einstellungen</h2>\n"
                  "  <label for=\"joystickTotzoneOverride\">Totzone Joystick Override (0.0-1.0):</label>\n"
                  "  <input type=\"range\" id=\"joystickTotzoneOverride\" min=\"0.0\" max=\"1.0\" step=\"0.01\" value=\"" + String(joystickTotzoneOverride) + "\" oninput=\"document.getElementById('joystickTotzoneOverride_value').textContent = this.value; document.getElementById('joystickTotzoneOverride_input').value = this.value;\"><span id=\"joystickTotzoneOverride_value\">" + String(joystickTotzoneOverride) + "</span>\n"
                  "  <input type=\"number\" id=\"joystickTotzoneOverride_input\" min=\"0.0\" max=\"1.0\" step=\"0.01\" value=\"" + String(joystickTotzoneOverride) + "\" oninput=\"document.getElementById('joystickTotzoneOverride').value = this.value; document.getElementById('joystickTotzoneOverride_value').textContent = this.value;\"> (0.0-1.0)<br><br>\n"
                  "\n"
                  "  <h2>Zyklischer Override Einstellungen</h2>\n"
                  "  <label for=\"joystickCyclicSpeedAxis\">Joystick Achse (Geschwindigkeit):</label>\n"
                  "  <select id=\"joystickCyclicSpeedAxis\">\n"
                  "    <option value=\"x\"" + (preferences.getString("joystickCyclicSpeedAxis", "x") == "x" ? " selected" : "") + ">X-Achse</option>\n"
                  "    <option value=\"y\"" + (preferences.getString("joystickCyclicSpeedAxis", "x") == "y" ? " selected" : "") + ">Y-Achse</option>\n"
                  "  </select><br><br>\n"
                  "  <label for=\"joystickCyclicSpeedMin\">Min. Intervall (ms) (50-5000):</label>\n"
                  "  <input type=\"number\" id=\"joystickCyclicSpeedMin\" value=\"" + String(preferences.getLong("joystickCyclicSpeedMin", 100)) + "\"> (50-5000)<br><br>\n"
                  "  <label for=\"joystickCyclicSpeedMax\">Max. Intervall (ms) (100-10000):</label>\n"
                  "  <input type=\"number\" id=\"joystickCyclicSpeedMax\" value=\"" + String(preferences.getLong("joystickCyclicSpeedMax", 5000)) + "\"> (100-10000)<br><br>\n"
                  "\n"
                  "  <label for=\"pulsintervall\">Puls Intervall (ms) (50-5000):</label>\n"
                  "  <input type=\"number\" id=\"pulsintervall\" min=\"50\" max=\"5000\" value=\"" + String(pulseInterval) + "\"> (50-5000)<br><br>\n"
                  "\n"
                  "  <label for=\"zufallmin\">Zufall Min. Intervall (ms) (50-5000):</label>\n"
                  "  <input type=\"number\" id=\"zufallmin\" min=\"50\" max=\"5000\" value=\"" + String(randomIntervalMin) + "\"> (50-5000)<br><br>\n"
                  "\n"
                  "  <label for=\"zufallmax\">Zufall Max. Intervall (ms) (100-10000):</label>\n"
                  "  <input type=\"number\" id=\"zufallmax\" min=\"100\" max=\"10000\" value=\"" + String(randomIntervalMax) + "\"> (100-10000)<br><br>\n"
                  "\n"
                  "  <label for=\"welle\">Wellengeschwindigkeit (ms) (50-2000):</label>\n"
                  "  <input type=\"number\" id=\"welle\" min=\"50\" max=\"2000\" value=\"" + String(waveSpeed) + "\"> (50-2000)<br><br>\n"
                  "\n"
                  "  <label for=\"halbeanmodus\">Halbe An Modus (0/1):</label>\n"
                  "  <input type=\"number\" id=\"halbeanmodus\" min=\"0\" max=\"1\" value=\"" + String(halfOnMode) + "\"> (0-1)<br><br>\n"
                  "\n"
                  "  <label for=\"alleanstatus\">Alle An/Aus Status (0-255):</label>\n"
                  "  <input type=\"number\" id=\"alleanstatus\" min=\"0\" max=\"255\" value=\"" + String(allOnOffState) + "\"> (0-255)<br><br>\n"
                  "\n"
                  "  <label for=\"fade\">Fading Dauer (ms) (100-10000):</label>\n"
                  "  <input type=\"number\" id=\"fade\" min=\"100\" max=\"10000\" value=\"" + String(fadeDuration) + "\"> (100-10000)<br><br>\n"
                  "\n"
                  "  <label for=\"sequenz\">Aktive Sequenz (0/1):</label>\n"
                  "  <input type=\"number\" id=\"sequenz\" min=\"0\" max=\"1\" value=\"" + String(activeSequence) + "\"> (0-1)<br><br>\n"
                  "\n"
                  "  <label for=\"sequenzintervall\">Sequenz Intervall (ms) (50-2000):</label>\n"
                  "  <input type=\"number\" id=\"sequenzintervall\" min=\"50\" max=\"2000\" value=\"" + String(sequenceInterval) + "\"> (50-2000)<br><br>\n"
                  "\n"
                  "  <button onclick=\"sendData()\">Speichern</button>\n"
                  "\n"
                  "  <script>\n"
                  "    function sendData() {\n"
                  "      var grundpwm = document.getElementById('grundpwm').value;\n"
                  "      var maxintensivpwm = document.getElementById('maxintensivpwm').value;\n"
                  "      var override = document.getElementById('override_modus').value;\n"
                  "      var overridepwm = document.getElementById('overridepwm').value;\n"
                  "      var pwmfrequenz = document.getElementById('pwmfrequenz').value;\n"
                  "      var grundpatterninterval = document.getElementById('grundpatterninterval').value;\n"
                  "      var pulsintervall = document.getElementById('pulsintervall').value;\n"
                  "      var zufallmin = document.getElementById('zufallmin').value;\n"
                  "      var zufallmax = document.getElementById('zufallmax').value;\n"
                  "      var welle = document.getElementById('welle').value;\n"
                  "      var halbeanmodus = document.getElementById('halbeanmodus').value;\n"
                  "      var alleanstatus = document.getElementById('alleanstatus').value;\n"
                  "      var fade = document.getElementById('fade').value;\n"
                  "      var sequenz = document.getElementById('sequenz').value;\n"
                  "      var sequenzintervall = document.getElementById('sequenzintervall').value;\n"
                  "      var joystickCyclicSpeed = document.getElementById('joystickCyclicSpeed').checked;\n"
                  "      var joystickCyclicSpeedAxis = document.getElementById('joystickCyclicSpeedAxis').value;\n"
                  "      var joystickCyclicSpeedMin = document.getElementById('joystickCyclicSpeedMin').value;\n"
                  "      var joystickCyclicSpeedMax = document.getElementById('joystickCyclicSpeedMax').value;\n"
                  "      var magnet8intensity = document.getElementById('magnet8intensity').value;\n"
                  "      var joystickTotzoneOverride = document.getElementById('joystickTotzoneOverride').value;\n"
                  "\n"
                  "      var xhr = new XMLHttpRequest();\n"
                  "      xhr.open(\"GET\", \"/set?grundpwm=\" + grundpwm + \"&maxintensivpwm=\" + maxintensivpwm + \"&override=\" + override + \"&overridepwm=\" + overridepwm + \"&pwmfrequency=\" + pwmfrequenz + \"&grundpatterninterval=\" + grundpatterninterval + \"&pulsintervall=\" + pulsintervall + \"&zufallmin=\" + zufallmin + \"&zufallmax=\" + zufallmax + \"&welle=\" + welle + \"&halbeanmodus=\" + halbeanmodus + \"&alleanstatus=\" + alleanstatus + \"&fade=\" + fade + \"&sequenz=\" + sequenz + \"&sequenzintervall=\" + sequenzintervall + \"&joystickCyclicSpeed=\" + (joystickCyclicSpeed ? 'on' : 'off') + \"&joystickCyclicSpeedAxis=\" + joystickCyclicSpeedAxis + \"&joystickCyclicSpeedMin=\" + joystickCyclicSpeedMin + \"&joystickCyclicSpeedMax=\" + joystickCyclicSpeedMax + \"&magnet8intensity=\" + magnet8intensity + \"&joystickTotzoneOverride=\" + joystickTotzoneOverride, true);\n"
                  "      xhr.send();\n"
                  "    }\n"
                  "  </script>\n"
                  "</body>\n"
                  "</html>";
    server.send(200, "text/html", html);
  });
  server.on("/set", HTTP_GET, handleSet);
  server.begin();
  Serial.println("HTTP Server gestartet");
}

void loop() {
  server.handleClient();
  updateMagnets();
  delay(10);
}