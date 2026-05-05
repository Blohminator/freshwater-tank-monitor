# Frischwasser-Tank Füllstandsanzeige

ESP32-basiertes Tank-Überwachungssystem mit TFmini-S LiDAR-Sensor, OLED-Display und SignalK-Integration.

## Funktionen

- **LiDAR-Distanzmessung**: TFmini-S Sensor für präzise berührungslose Füllstandsmessung
- **OLED-Display**: Echtzeit-Anzeige von Füllstand (%) und Höhe (cm) auf 128x64 SH1106 Display
- **SignalK-Integration**: Vollständige Integration mit SignalK Marine-Datensystem über SensESP 3.2.0
- **Web-konfigurierbare Tank-Abmessungen**: Tankgröße, Sensor-Offset und Alarmschwelle sind über das Web-Interface konfigurierbar — kein Neukompilieren nötig
- **Überfüllungs-Alarm**: Konfigurierbarer Alarmausgang bei Überschreitung des Schwellwerts (Standard 95%)
- **Glättungsfilter**: Exponentieller gleitender Mittelwert (EMA, α=0,1) für stabile Messwerte
- **WiFi-Konnektivität**: Web-Interface für Konfiguration und Überwachung

## Hardware-Anforderungen

### Komponenten
- ESP32 Development Board (ESP32-WROOM-32)
- TFmini-S LiDAR Distanzsensor
- 1,3" OLED Display (128x64, SH1106 Chip, I2C)
- Alarmausgabe-Gerät (optional, an GPIO 23 angeschlossen)

### Verkabelung

#### TFmini-S LiDAR Sensor
| TFmini-S | ESP32 |
|----------|-------|
| TX (Grün) | GPIO 16 (RXD2) |
| RX (Weiß) | GPIO 17 (TXD2) |
| VCC (Rot) | 5V |
| GND (Schwarz) | GND |

#### OLED Display (I2C)
| OLED | ESP32 |
|------|-------|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| VCC | 3.3V |
| GND | GND |

#### Alarmausgang
| Komponente | ESP32 |
|-----------|-------|
| Alarmausgang | GPIO 23 |

## Software-Einrichtung

### Voraussetzungen
- PlatformIO IDE oder PlatformIO Core
- USB-Kabel für ESP32-Programmierung

### Installation

1. Projekt klonen oder herunterladen
2. Projekt in PlatformIO öffnen
3. Kompilieren und hochladen:
```bash
pio run --target upload
```

4. Tank-Abmessungen nach dem ersten Start über das Web-Interface konfigurieren (siehe unten)

### Erstkonfiguration

1. Nach dem ersten Start erstellt der ESP32 einen WiFi-Access-Point namens "SensESP-freshwater-tank"
2. Mit diesem Netzwerk über Smartphone oder Computer verbinden
   - **WiFi-Passwort:** `thisisfine`
3. Ein Captive Portal öffnet sich automatisch (oder zu 192.168.4.1 navigieren)
4. WiFi-Zugangsdaten und SignalK-Server-Einstellungen konfigurieren
5. Das Gerät startet neu und verbindet sich mit Ihrem Netzwerk

## Tank-Konfiguration (Web-Interface)

Alle Tank-Parameter sind über das Web-Interface unter `http://freshwater-tank.local` konfigurierbar — keine Code-Änderungen erforderlich.

> 🔒 **Anmeldung erforderlich** — Benutzername: `admin` / Passwort: `thisisfine`

| Parameter | Pfad | Standard | Beschreibung |
|-----------|------|----------|--------------|
| Tanklänge | `/Tank/Length_cm` | 100 cm | Tanklänge in cm |
| Tankbreite | `/Tank/Width_cm` | 50 cm | Tankbreite in cm |
| Tankhöhe | `/Tank/Height_cm` | 110 cm | Tankhöhe in cm |
| Sensor-Offset | `/Tank/Offset_cm` | 5 cm | Abstand Sensor zur Tankoberkante |
| Alarmschwelle | `/Tank/Alarm_pct` | 95 % | Füllstand, bei dem der Alarm auslöst |

Änderungen werden sofort übernommen, ohne das Gerät neu zu starten.

## SignalK-Integration

Das System veröffentlicht drei Werte an SignalK:

| Pfad | Beschreibung | Einheit |
|------|--------------|---------|
| `tanks.freshWater.0.currentLevel` | Aktueller Füllstand | 0.0–1.0 (Verhältnis) |
| `tanks.freshWater.0.capacity` | Tankkapazität | m³ |
| `tanks.freshWater.0.currentVolume` | Aktuelles Volumen | m³ |

`currentLevel` und `currentVolume` werden gesendet, wenn sich der Wert um mehr als 1% ändert, maximal alle 2 Sekunden.  
`capacity` wird einmalig nach dem Start (5 s Verzögerung) und danach alle 60 Sekunden gesendet.

## Display-Informationen

Das OLED-Display zeigt:
- **Zeile 1**: Titel — "Freshwater Tank"
- **Zeile 2**: Füllstand in Prozent — z.B. "Level: 75 %"
- **Zeile 3**: Geglättete Füllhöhe in Zentimetern — z.B. "Height: 82 cm"

## Alarmfunktion

- Alarmausgang (GPIO 23) wird HIGH, wenn der Füllstand den konfigurierten Schwellwert erreicht oder überschreitet
- Standard-Schwellwert: 95% (Überfüllungswarnung beim Befüllen des Tanks)
- Konfigurierbar über Web-Interface unter `/Tank/Alarm_pct`

## Tank-Volumenberechnung

```
Kapazität (Liter) = (Länge_cm × Breite_cm × Höhe_cm) / 1000
Kapazität (m³)    = Kapazität (Liter) / 1000
Aktuelles Volumen = Kapazität (m³) × Füllstand-Verhältnis
```

## Glättungsfilter

Rohe Distanzmesswerte werden mit einem exponentiellen gleitenden Mittelwert (EMA) geglättet:

```
gefiltert = gefiltert + α × (roh − gefiltert)   (α = 0,1)
```

Dies reduziert Rauschen durch Wellengang oder Sensor-Jitter. Die angezeigte Höhe und der Prozentwert werden beide aus dem geglätteten Wert berechnet.

## Fehlerbehebung

### Display zeigt "Init..." und aktualisiert nicht
- I2C-Verbindungen prüfen (SDA/SCL)
- Display-Adresse 0x3C verifizieren
- Prüfen ob Display SH1106-kompatibel ist

### Keine LiDAR-Messwerte
- UART-Verbindungen prüfen (TX/RX sind gekreuzt)
- 5V-Stromversorgung des Sensors prüfen
- Sicherstellen, dass Sensor freie Sicht zur Wasseroberfläche hat

### SignalK empfängt keine Daten
- WiFi-Verbindung prüfen
- SignalK-Server-Adresse im Web-Interface prüfen
- Sicherstellen, dass SignalK-Server läuft und erreichbar ist

### Falsche Volumenberechnungen
- Tank-Abmessungen im Web-Interface prüfen
- Sensor-Offset-Wert prüfen
- Sicherstellen, dass Sensor korrekt über Tank montiert ist

### Alarm löst bei falschem Füllstand aus
- Alarmschwelle im Web-Interface anpassen (`/Tank/Alarm_pct`, Standard 95%)
- Alarm wird ausgelöst wenn Füllstand >= Schwellwert

## Technische Spezifikationen

- **Messbereich**: 0,3 m – 12 m (TFmini-S)
- **Messgenauigkeit**: ±6 cm @ 6 m
- **Aktualisierungsrate**: 2 Hz (500 ms)
- **SignalK Sendeintervall**: max. alle 2 s, nur bei >1% Änderung
- **Betriebsspannung**: 5V (über USB oder externe Stromversorgung)
- **WiFi**: 802.11 b/g/n (2,4 GHz)
- **Display**: 128×64 Pixel, monochrom

## Montage-Hinweise

1. **Sensor-Position**: Montieren Sie den LiDAR-Sensor direkt über der Mitte des Tanks
2. **Sensor-Ausrichtung**: Sensor muss senkrecht nach unten zeigen
3. **Abstand**: Messen Sie den Abstand vom Sensor zur Tankoberkante und tragen Sie diesen als Sensor-Offset ein
4. **Hindernisse**: Stellen Sie sicher, dass keine Hindernisse zwischen Sensor und Wasseroberfläche sind
5. **Vibrationen**: Sensor sollte vibrationsfrei montiert werden

## Lizenz

Dieses Projekt ist Open Source. Fühlen Sie sich frei, es für Ihre Bedürfnisse anzupassen.

## Danksagungen

- Erstellt mit [SensESP](https://github.com/SignalK/SensESP) 3.2.0
- Verwendet [U8g2](https://github.com/olikraus/u8g2) Bibliothek für Display
- Kompatibel mit [SignalK](https://signalk.org/) Marine-Datenstandard

## Versions-Historie

- **v1.1** - Web-konfigurierbare Parameter, EMA-Glättungsfilter
  - Tank-Abmessungen und Alarmschwelle über Web-Interface konfigurierbar
  - Exponentieller gleitender Mittelwert (α=0,1) für stabile Messwerte
  - Display zeigt geglättete Höhe
  - Aktualisierungsrate auf 2 Hz (500 ms) geändert
- **v1.0** - Erste Veröffentlichung mit SensESP 3.2.0 Unterstützung
  - TFmini-S LiDAR Integration
  - OLED Display Unterstützung
  - SignalK Integration
  - Überfüllungs-Alarmausgang (95% Schwellwert)
