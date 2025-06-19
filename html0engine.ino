#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <freertos/semphr.h>
#include <stdlib.h> // Für die Zufallszahlengenerierung
#include <vector>
#include <String> // Für String-Manipulation

// WLAN Konfiguration
const char* ssid = "EngineTestWeb";
const char* password = "robbydaheim";

WebServer server(80);

// Pin-Definitionen (unverändert)
const int magnetPins[7][2] = {
    {18, 19}, {21, 22}, {23, 4}, {2, 15}, {13, 12}, {14, 27}, {16, 17}
};
const int magnet8Pins[2] = {5, 3};

// PWM-Konfiguration für Magnete 0-6 (unverändert)
int grundPWM = 255;
int pwmFrequency = 80;
const int pwmResolution = 8;
volatile int delayTime = 233; // Anpassbare Verzögerungszeit für die Gesamtgeschwindigkeit

// Globale Variable zur Steuerung von Magnet 8 (0 = aus, 5 = Pin 5 an, 3 = Pin 3 an)
volatile int magnet8Steuerung = 5;

// Kanal-Definitionen (unverändert)
const int magnetChannels[7][2] = {
    {0, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9}, {10, 11}, {12, 13}
};

// Array mit den Zeigern auf die 7 Basis-Schaltbilder (unverändert - aber im neuen System nicht direkt verwendet)
const int schaltbild1[7] = {1, -1, 0, 1, -1, 0, 1};
const int schaltbild2[7] = {-1, 0, 1, -1, 0, 1, 1};
const int schaltbild3[7] = {0, 1, -1, 0, 1, 1, -1};
const int schaltbild4[7] = {1, -1, 0, 1, 1, -1, 0};
const int schaltbild5[7] = {-1, 0, 1, 1, -1, 0, 1};
const int schaltbild6[7] = {0, 1, -1, -1, 0, 1, -1};
const int schaltbild7[7] = {1, -1, -1, 0, 1, -1, 0};

const int* basisSchaltbilder[7] = {
    schaltbild1, schaltbild2, schaltbild3, schaltbild4, schaltbild5, schaltbild6, schaltbild7
};
const int anzahlBasisSchaltbilder = sizeof(basisSchaltbilder) / sizeof(basisSchaltbilder[0]);

// Globale Variablen zur Speicherung der Konfiguration
std::vector<bool> sequenzAktivierung;
std::vector<std::vector<std::vector<int>>> sequenzMatrixSammlung; // [matrix][schritt][magnet] (+1, -1, 0)
std::vector<std::vector<int>> schrittReihenfolgeProSequenz;     // [matrix][index_in_reihenfolge] -> schritt_index (0-basiert)
std::vector<std::vector<bool>> schrittAktivierung;           // [matrix][schritt]
std::vector<std::vector<int>> magnet8SequenzSteuerung;       // [sequenz][schritt] -> 0, 5, oder 3
volatile int aktuelleMatrixIndex = 0;
volatile int aktuellerSchrittIndexInReihenfolge = 0; // Index innerhalb der Schrittreihenfolge
volatile int anzahlSchaltbilderInMatrix = 7;
volatile int anzahlSchritteInSequenz = 7;

// Task-Handles
TaskHandle_t magnetControlTaskHandle;
TaskHandle_t magnet8ControlTaskHandle;

// Deklaration des Mutex
SemaphoreHandle_t serialMutex;

// Forward-Deklarationen der Handler-Funktionen
void handleRoot();
void handleSetDelay();
void handleSetPWM();
void handleEditMatrices();
void handleConfigureMatrices();
void handleSaveConfig();
void magnetControlTask(void * parameter);
void magnet8ControlTask(void * parameter);
String vectorToString(const std::vector<int>& vec);
std::vector<int> stringToVector(const String& str);

// Handler für die Hauptseite (ERWEITERT UM SCHRITTREIHENFOLGE)
void handleRoot() {
    Serial.println("handleRoot aufgerufen.");
    String html = "<!DOCTYPE html>\n";
    html += "<html><head><title>ESP32 Magnetsteuerung</title><style>table { width: auto !important; overflow-x: auto; display: block; }</style></head><body>";
    html += "<h1>ESP32 Magnetsteuerung</h1>";

    // Formular für die direkte Verzögerungszeit-Eingabe
    html += "<h2>Direkte Verzögerungszeit-Einstellung</h2>";
    html += "<form action='/set_delay'>";
    html += "<label for='delay'>Verzögerungszeit (ms):</label>";
    html += "<input type='number' id='delay' name='delay' value='" + String(delayTime) + "'>";
    html += "<input type='submit' value='Verzögerung setzen'>";
    html += "</form><br><br>";

    // Formular für die PWM-Einstellungen
    html += "<h2>PWM Einstellungen</h2>";
    html += "<form action='/set_pwm'>";
    html += "<label for='pwm_frequency'>PWM Frequenz (Hz):</label>";
    html += "<input type='number' id='pwm_frequency' name='pwm_frequency' value='" + String(pwmFrequency) + "'>";
    html += "<br>";
    html += "<label for='grund_pwm'>Grund PWM (0-255):</label>";
    html += "<input type='number' id='grund_pwm' name='grund_pwm' min='0' max='255' value='" + String(grundPWM) + "'>";
    html += "<br>";
    html += "<input type='submit' value='PWM speichern'>";
    html += "</form><br>";
    html += "Aktuelle PWM Frequenz: " + String(pwmFrequency) + " Hz<br>";
    html += "Aktueller Grund PWM: " + String(grundPWM) + "<br><br>";

    // Formular zur Auswahl der Anzahl der Matrizen und Schritte
    html += "<h2>Sequenz Konfiguration</h2>";
    html += "<form action='/configure_matrices' method='post'>";
    html += "<label for='anzahl_matrizen'>Anzahl der Matrizen:</label>";
    html += "<input type='number' id='anzahl_matrizen' name='anzahl_matrizen' min='0' value='" + String(sequenzMatrixSammlung.size()) + "'><br><br>";
    html += "<label for='anzahl_schritte'>Anzahl der Schritte pro Sequenz:</label>";
    html += "<input type='number' id='anzahl_schritte' name='anzahl_schritte' min='1' value='" + String(anzahlSchritteInSequenz) + "'><br><br>";
    html += "<input type='submit' value='Matrizen konfigurieren'>";
    html += "</form><br>";

    html += "</body></html>";
    server.send(200, "text/html", html);
}

// Handler zum Setzen der Verzögerungszeit (unverändert)
void handleSetDelay() {
    if (server.hasArg("delay")) {
        delayTime = server.arg("delay").toInt();
        server.send(200, "text/html", "<h1>Verzögerungszeit gesetzt!</h1><p>Neue Verzögerungszeit: " + String(delayTime) + " ms</p><a href='/'>Zurück</a>");
    } else {
        server.send(400, "text/plain", "Fehler: Verzögerungszeit nicht übergeben.");
    }
}

// Neue Handler-Funktion zum Setzen der PWM-Einstellungen (unverändert)
void handleSetPWM() {
    if (server.hasArg("pwm_frequency") && server.hasArg("grund_pwm")) {
        pwmFrequency = server.arg("pwm_frequency").toInt();
        grundPWM = server.arg("grund_pwm").toInt();

        // PWM-Konfiguration neu initialisieren
        for (int i = 0; i < 7; i++) {
            for (int j = 0; j < 2; j++) {
                ledcSetup(magnetChannels[i][j], pwmFrequency, pwmResolution);
            }
        }

        server.send(200, "text/html", "<h1>PWM Einstellungen gespeichert!</h1>"
                            "<p>Neue PWM Frequenz: " + String(pwmFrequency) + " Hz</p>"
                            "<p>Neuer Grund PWM: " + String(grundPWM) + "</p>"
                            "<a href='/'>Zurück</a>");
    } else {
        server.send(400, "text/plain", "Fehler: PWM Frequenz oder Grund PWM nicht übergeben.");
    }
}

// NEU: Handler für die Seite zur Auswahl der Anzahl der Matrizen
void handleEditMatrices() {
    Serial.println("handleEditMatrices aufgerufen.");
    String html = "<!DOCTYPE html>\n";
    html += "<html><head><title>Sequenz Matrizen Konfiguration</title></head><body>";
    html += "<h1>Sequenz Matrizen Konfiguration</h1>";
    html += "<form action='/configure_matrices' method='get'>";
    html += "<label for='anzahl_matrizen'>Anzahl 7x7 Matrizen:</label>";
    html += "<input type='number' id='anzahl_matrizen' name='anzahl_matrizen' value='" + String(sequenzMatrixSammlung.size()) + "' min='0'>";
    html += "<input type='submit' value='Konfiguration starten'>";
    html += "</form><br>";
    html += "<p><a href='/'>Zurück zur Hauptseite</a></p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleConfigureMatrices() {
    Serial.println("handleConfigureMatrices aufgerufen.");
    if (server.hasArg("anzahl_matrizen") && server.hasArg("anzahl_schritte")) {
        int anzahlMatrizen = server.arg("anzahl_matrizen").toInt();
        anzahlSchritteInSequenz = server.arg("anzahl_schritte").toInt();

        if (anzahlMatrizen >= 0 && anzahlSchritteInSequenz > 0) {
            String html = "<!DOCTYPE html>\n";
            html += "<html><head><title>Sequenz Matrizen Konfiguration</title>";
            html += "<style>table { width: auto !important; overflow-x: auto; display: block; }</style>";
            html += "</head><body>";
            html += "<h1>Sequenz Matrizen Konfiguration</h1>";
            html += "<form action='/save_config' method='post'>";
            html += "<input type='hidden' name='anzahl_matrizen' value='" + String(anzahlMatrizen) + "'>";
            html += "<input type='hidden' name='anzahl_schritte' value='" + String(anzahlSchritteInSequenz) + "'>";

            for (int m = 0; m < anzahlMatrizen; ++m) {
                html += "<h3>Sequenz " + String(m + 1) + "</h3>";
                // Checkbox zur Aktivierung/Deaktivierung der gesamten Sequenz
                html += "<label for='sequenzAktiv_" + String(m) + "'>";
                html += "<input type='checkbox' id='sequenzAktiv_" + String(m) + "' name='sequenzAktiv[" + String(m) + "]' value='true' " + (m < sequenzAktivierung.size() && sequenzAktivierung[m] ? "checked" : "") + ">Sequenz " + String(m + 1) + " aktivieren";
                html += "</label><br>";

                // Eingabefeld für die Schrittreihenfolge
                html += "<label for='schritt_reihenfolge_" + String(m) + "'>Schrittreihenfolge (z.B. 1,3,4,2):</label>";
                html += "<input type='text' id='schritt_reihenfolge_" + String(m) + "' name='schritt_reihenfolge[" + String(m) + "]' value='" + (schrittReihenfolgeProSequenz.size() > m ? vectorToString(schrittReihenfolgeProSequenz[m]) : "") + "'><br><br>";

                html += "<table id='sequenzMatrixTabelle_" + String(m) + "'>";
                html += "<thead><tr><th>Schritt</th>";
                for (int mag_header = 0; mag_header < 7; ++mag_header) {
                    html += "<th>M" + String(mag_header) + "</th>";
                }
                html += "<th>Aktiv</th></tr></thead><tbody>";
                for (int schritt = 0; schritt < anzahlSchritteInSequenz; ++schritt) {
                    html += "<tr><td>Schritt " + String(schritt + 1) + "</td>";
                    for (int mag = 0; mag < 7; ++mag) {
                        String wert = "";
                        if (m < sequenzMatrixSammlung.size() && schritt < sequenzMatrixSammlung[m].size() && mag < sequenzMatrixSammlung[m][schritt].size()) {
                            wert = (sequenzMatrixSammlung[m][schritt][mag] == 1) ? "+" : (sequenzMatrixSammlung[m][schritt][mag] == -1) ? "-" : "0";
                        }
                        html += "<td><select name='matrices[" + String(m) + "][" + String(schritt) + "][" + String(mag) + "]'>";
                        html += "<option value='0'" + String(wert == "0" ? " selected" : "") + ">0</option>";
                        html += "<option value='+'" + String(wert == "+" ? " selected" : "") + ">+</option>";
                        html += "<option value='-'" + String(wert == "-" ? " selected" : "") + ">-</option>";
                        html += "</select></td>";
                    }
                    // Checkbox zur Aktivierung/Deaktivierung jedes einzelnen Schritts
                    html += "<td><label for='schrittAktiv_" + String(m) + "_" + String(schritt) + "'>";
                    html += "<input type='checkbox' id='schrittAktiv_" + String(m) + "_" + String(schritt) + "' name='schrittAktiv[" + String(m) + "][" + String(schritt) + "]' value='true' " + (m < schrittAktivierung.size() && schritt < schrittAktivierung[m].size() && schrittAktivierung[m][schritt] ? "checked" : "") + ">Aktiv";
                    html += "</label></td>";
                    html += "</tr>";
                }
                html += "</tbody></table><br>";

                // Steuerung für Magnet 8
                html += "<h4>Magnet 8 Steuerung für Sequenz " + String(m + 1) + "</h4>";
                html += "<table><tr><th>Schritt</th>";
                for (int s = 1; s <= anzahlSchritteInSequenz; ++s) {
                    html += "<th>" + String(s) + "</th>";
                }
                html += "</tr><tr><td>Richtung:</td>";
                if (m < magnet8SequenzSteuerung.size() && magnet8SequenzSteuerung[m].size() != anzahlSchritteInSequenz) {
                    magnet8SequenzSteuerung[m].resize(anzahlSchritteInSequenz, 0);
                }
                for (int s = 0; s < anzahlSchritteInSequenz; ++s) {
                    int currentValue = (m < magnet8SequenzSteuerung.size() && s < magnet8SequenzSteuerung[m].size()) ? magnet8SequenzSteuerung[m][s] : 0;
                    html += "<td><select name='magnet8_richtung[" + String(m) + "][" + String(s) + "]'>";
                    html += "<option value='0'" + String(currentValue == 0 ? " selected" : "") + ">Aus</option>";
                    html += "<option value='5'" + String(currentValue == 5 ? " selected" : "") + ">Pin 5 HIGH</option>";
                    html += "<option value='3'" + String(currentValue == 3 ? " selected" : "") + ">Pin 3 HIGH</option>";
                    html += "</select></td>";
                }
                html += "</tr></table><br>";
            }

            html += "<input type='submit' value='Konfiguration speichern'>";
            html += "</form><br>";
            html += "<p><a href='/'>Zurück zur Hauptseite</a></p>";
            html += "seite</a></p>";
            html += "</body></html>";
            server.send(200, "text/html", html);
        } else {
            server.send(400, "text/plain", "Fehler: Ungültige Anzahl von Matrizen oder Schritten.");
        }
    } else {
        server.send(400, "text/plain", "Fehler: Anzahl der Matrizen oder Schritte nicht übergeben.");
    }
}

void handleSaveConfig() {
    if (server.hasArg("anzahl_matrizen") && server.hasArg("anzahl_schritte")) {
        int anzahlMatrizen = server.arg("anzahl_matrizen").toInt();
        int empfangeneAnzahlSchritte = server.arg("anzahl_schritte").toInt();

        sequenzMatrixSammlung.resize(anzahlMatrizen);
        magnet8SequenzSteuerung.resize(anzahlMatrizen);
        sequenzAktivierung.resize(anzahlMatrizen);
        schrittAktivierung.resize(anzahlMatrizen);
        schrittReihenfolgeProSequenz.resize(anzahlMatrizen);

        for (int m = 0; m < anzahlMatrizen; ++m) {
            sequenzMatrixSammlung[m].resize(empfangeneAnzahlSchritte);
            magnet8SequenzSteuerung[m].resize(empfangeneAnzahlSchritte, 0);
            schrittAktivierung[m].resize(empfangeneAnzahlSchritte, false);

            // Auslesen des Aktivierungszustands für die gesamte Sequenz
            String argNameAktivSequenz = "sequenzAktiv[" + String(m) + "]";
            sequenzAktivierung[m] = server.hasArg(argNameAktivSequenz) && (server.arg(argNameAktivSequenz) == "true");

            // Auslesen der Schrittreihenfolge
            String schrittReihenfolgeStr = server.arg("schritt_reihenfolge[" + String(m) + "]");
            schrittReihenfolgeProSequenz[m] = stringToVector(schrittReihenfolgeStr);

            for (int schritt = 0; schritt < empfangeneAnzahlSchritte; ++schritt) {
                sequenzMatrixSammlung[m][schritt].resize(7, 0);

                for (int mag = 0; mag < 7; ++mag) {
                    String argName = "matrices[" + String(m) + "][" + String(schritt) + "][" + String(mag) + "]";
                    if (server.hasArg(argName)) {
                        String wertStr = server.arg(argName);
                        if (wertStr == "+") sequenzMatrixSammlung[m][schritt][mag] = 1;
                        else if (wertStr == "-") sequenzMatrixSammlung[m][schritt][mag] = -1;
                        else sequenzMatrixSammlung[m][schritt][mag] = 0;
                    }
                }
                // Auslesen des Aktivierungszustands für den Schritt
                String argNameAktivSchritt = "schrittAktiv[" + String(m) + "][" + String(schritt) + "]";
                schrittAktivierung[m][schritt] = server.hasArg(argNameAktivSchritt) && (server.arg(argNameAktivSchritt) == "true");
            }
            // Verarbeitung der Magnet 8 Richtung
            for (int j = 0; j < empfangeneAnzahlSchritte; ++j) {
                String argNameMagnet8 = "magnet8_richtung[" + String(m) + "][" + String(j) + "]";
                if (server.hasArg(argNameMagnet8)) {
                    magnet8SequenzSteuerung[m][j] = server.arg(argNameMagnet8).toInt();
                }
            }
        }

        server.send(200, "text/html", "<h1>Konfiguration gespeichert!</h1><a href='/'>Zurück zur Hauptseite</a>");
        Serial.println("Konfiguration gespeichert!");

    } else {
        server.send(400, "text/plain", "Fehler: Anzahl der Matrizen oder Schritte nicht übergeben.");
    }
}

void magnetControlTask(void *parameter) {
    Serial.println("Magnet Control Task gestartet.");
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        for (int m = 0; m < sequenzMatrixSammlung.size(); ++m) {
            if (sequenzAktivierung[m]) {
                Serial.printf("Sequenz: %d, Anzahl Schritte: %d, Reihenfolge: %s\n", m, sequenzMatrixSammlung[m].size(), vectorToString(schrittReihenfolgeProSequenz[m]).c_str());

                for (int schrittIndexInReihenfolge = 0; schrittIndexInReihenfolge < schrittReihenfolgeProSequenz[m].size(); ++schrittIndexInReihenfolge) {
                    int aktuellerSchritt = schrittReihenfolgeProSequenz[m][schrittIndexInReihenfolge];
                    int anzahlSchritteInAktuellerSequenz = sequenzMatrixSammlung[m].size();

                    if (aktuellerSchritt >= 0 && aktuellerSchritt < anzahlSchritteInAktuellerSequenz && aktuellerSchritt < schrittAktivierung[m].size() && schrittAktivierung[m][aktuellerSchritt]) {
                        Serial.printf("  Ausführe Schritt %d (in Reihenfolge: %d)\n", aktuellerSchritt + 1, schrittIndexInReihenfolge + 1);
                        for (int mag = 0; mag < 7; ++mag) {
                            int pwmValue = grundPWM;
                            if (sequenzMatrixSammlung[m][aktuellerSchritt][mag] == 1) {
                                ledcWrite(magnetChannels[mag][0], pwmValue);
                                ledcWrite(magnetChannels[mag][1], 0);
                                Serial.printf("    Magnet %d: +\n", mag);
                            } else if (sequenzMatrixSammlung[m][aktuellerSchritt][mag] == -1) {
                                ledcWrite(magnetChannels[mag][0], 0);
                                ledcWrite(magnetChannels[mag][1], pwmValue);
                                Serial.printf("    Magnet %d: -\n", mag);
                            } else {
                                ledcWrite(magnetChannels[mag][0], 0);
                                ledcWrite(magnetChannels[mag][1], 0);
                                Serial.printf("    Magnet %d: 0\n", mag);
                            }
                        }

                        // Steuerung von Magnet 8 innerhalb der Schritt-Schleife
                        if (m < magnet8SequenzSteuerung.size() && aktuellerSchritt < magnet8SequenzSteuerung[m].size()) {
                            magnet8Steuerung = magnet8SequenzSteuerung[m][aktuellerSchritt];
                            if (magnet8Steuerung == 5) {
                                digitalWrite(magnet8Pins[0], HIGH);
                                digitalWrite(magnet8Pins[1], LOW);
                                Serial.printf("    Magnet 8: Pin 5 HIGH\n");
                            } else if (magnet8Steuerung == 3) {
                                digitalWrite(magnet8Pins[0], LOW);
                                digitalWrite(magnet8Pins[1], HIGH);
                                Serial.printf("    Magnet 8: Pin 3 HIGH\n");
                            } else {
                                digitalWrite(magnet8Pins[0], LOW);
                                digitalWrite(magnet8Pins[1], LOW);
                                Serial.printf("    Magnet 8: AUS\n");
                            }
                        }

 