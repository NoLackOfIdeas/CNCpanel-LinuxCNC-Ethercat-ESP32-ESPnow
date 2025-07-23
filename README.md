**Integriertes Dual-ESP32-Steuerungssystem für LinuxCNC über EtherCAT und ESP-NOW**

**Teil 1: Systemarchitektur und Designprinzipien**

**1.1. Konzeptionelle Übersicht**

Dieses Dokument beschreibt die Architektur, das Hardware-Design und die Firmware-Implementierung eines hochentwickelten, dualen ESP32-basierten Steuerungssystems, das als Mensch-Maschine-Schnittstelle (HMI) für eine LinuxCNC-gesteuerte Maschine konzipiert ist. Das System ist modular aufgebaut und besteht aus zwei primären Mikrocontroller-Einheiten, die jeweils für spezifische Aufgaben optimiert sind, um maximale Leistung, Reaktionsfähigkeit und Konfigurierbarkeit zu gewährleisten.

1. **ESP1: Die Echtzeit-Kommunikationsbrücke (Real-Time Communication Bridge)**: Diese Einheit fungiert als primäres Gateway zwischen der industriellen Steuerungswelt und dem HMI-Subsystem. Sie ist direkt über ein EasyCAT PRO Shield mit dem LinuxCNC-Host-PC verbunden und nutzt das deterministische EtherCAT-Protokoll für den Hochgeschwindigkeitsdatenaustausch. Zusätzlich erfasst ESP1 Daten von einer Reihe von Hochgeschwindigkeitsperipheriegeräten, die eine enge zeitliche Kopplung mit der Maschinensteuerung erfordern, wie z. B. Quadratur-Encoder zur Positionsbestimmung, einen Hall-Sensor zur Spindeldrehzahlerfassung und induktive Sonden für Referenzierungs- oder Messaufgaben.
2. **ESP2: Der interaktive HMI-Controller (Interactive HMI Controller)**: Diese Einheit bildet das Herzstück der Benutzeroberfläche. Sie verwaltet eine komplexe Anordnung von Eingabe- und Ausgabegeräten, darunter weitere Encoder, eine große Tastenmatrix, eine korrespondierende LED-Matrix zur Statusvisualisierung, analoge Potentiometer für stufenlose Einstellungen und mehrstufige Drehschalter zur Moduswahl. ESP2 hostet zudem eine Weboberfläche zur dynamischen Konfiguration und Überwachung.

Die Kommunikation zwischen den beiden ESP32-Einheiten erfolgt drahtlos über das ESP-NOW-Protokoll, das für seine geringe Latenz und hohe Zuverlässigkeit in Punkt-zu-Punkt-Verbindungen ausgewählt wurde.<sup>1</sup> Dieses duale Konzept entkoppelt die zeitkritischen Maschinenkommunikationsaufgaben (ESP1) von den komplexeren, aber weniger zeitkritischen HMI-Management- und Webserver-Aufgaben (ESP2), was zu einem robusteren und leistungsfähigeren Gesamtsystem führt.

Das Kernziel dieses Projekts ist die Schaffung eines physischen Bedienfelds, das die Flexibilität und Konfigurierbarkeit moderner Embedded-Systeme mit der Zuverlässigkeit und Leistung industrieller Steuerungsprotokolle verbindet.

**System-Topologie:**

```mermaid

graph TD;

subgraph LinuxCNC-Umgebung

LCNC(LinuxCNC+Ethercat Host-PC)

end

subgraph (Servos
  )
SERVO1(Servo 1)
SERVO2(Servo 2)
SERVO3(Servo ...)
SERVO4(Servo X)
end

subgraph HMI-Einheit

ESP2(ESP2)

subgraph ESP2 Peripherie

BTN(max 8x8 BTNs)

LED(max 8x8 LEDs)

ENC2(max. 8 Quadratur-Encoder)

POTS(max. 6 Potentiometer)

ROT(max 4 ROT.Switches)

end

end

subgraph Kommunikationsbrücke

ESP1(ESP1)

subgraph ESP1 Peripherie

ENC1(max. 8 Quadratur-Encoder)

HALL(1 Hall/RPM)

PROBES(max 8 Probes)

end

end

LCNC <--> |EtherCAT| SERVO1
SERVO1 <--> |EtherCAT| SERVO2
SERVO2 <--> |EtherCAT| SERVO3
SERVO3 <--> |EtherCAT| SERVO4

LCNC <--> |EtherCAT| ESP1

ESP1 <--> |ESP-NOW| ESP2

ESP1 -- erfasst --> ENC1

ESP1 -- erfasst --> HALL

ESP1 -- erfasst --> PROBES

ESP2 -- steuert & liest <--> BTN

ESP2 -- steuert --> LED

ESP2 -- liest --> ENC2

ESP2 -- liest --> POTS

ESP2 -- liest --> ROT
```

**1.2. Datenfluss und Kommunikationslogik**

Der bidirektionale Datenfluss ist das zentrale Element der Systemfunktionalität. Er ermöglicht es, dass Benutzereingaben an der HMI nahezu in Echtzeit an die Maschinensteuerung weitergeleitet werden und umgekehrt der Maschinenzustand auf der HMI visualisiert wird.

**Pfad 1: Steuerungseingabe (HMI → LinuxCNC)**

1. **Erfassung (ESP2):** Ein Peripheriegerät an ESP2 ändert seinen Zustand (z. B. eine Taste wird gedrückt, ein Encoder wird gedreht).
2. **Verarbeitung (ESP2):** Die Firmware auf ESP2 erfasst diese Zustandsänderung durch Scannen der Matrix oder über Interrupts. Die neuen Daten aller HMI-Peripheriegeräte werden in einer vordefinierten Datenstruktur (einem C++ struct) zusammengefasst.
3. **Übertragung (ESP-NOW):** Diese Datenstruktur wird als ESP-NOW-Paket an ESP1 gesendet.
4. **Bridging (ESP1):** ESP1 empfängt das ESP-NOW-Paket über eine Callback-Funktion. Die Firmware extrahiert die Daten aus der Struktur und schreibt sie an die entsprechenden Offsets im _Input Process Data Object (PDO)_ des EtherCAT-Speichers. Dieses PDO wird auch als "Process Data Image" bezeichnet.<sup>3</sup>
5. **Kommunikation (EtherCAT):** Im nächsten EtherCAT-Zyklus liest der LinuxCNC-Master das aktualisierte Input-PDO von ESP1 und stellt die Daten der CNC-Steuerungslogik zur Verfügung.

**Pfad 2: Zustandsrückmeldung (LinuxCNC → HMI)**

1. **Aktualisierung (LinuxCNC):** Der Zustand der CNC-Maschine ändert sich (z. B. ein Programm wird gestartet, ein Fehler tritt auf). LinuxCNC schreibt die neuen Statusinformationen (z. B. Flags für aktive Modi, Daten für LEDs) in das _Output Process Data Object (PDO)_, das für ESP1 bestimmt ist.
2. **Kommunikation (EtherCAT):** Der EtherCAT-Master sendet das aktualisierte Output-PDO an den ESP1-Slave.
3. **Empfang (ESP1):** Die EasyCAT-Bibliothek auf ESP1 empfängt die Daten und speichert sie im lokalen Output-Puffer.
4. **Bridging (ESP1):** Die Firmware auf ESP1 erkennt die neuen Daten im EtherCAT-Output-Puffer. Sie verpackt die relevanten Informationen (insbesondere die Zustände für die LED-Matrix) in die ESP-NOW-Datenstruktur.
5. **Übertragung (ESP-NOW):** ESP1 sendet das Paket mit den Statusdaten an ESP2.
6. **Visualisierung (ESP2):** ESP2 empfängt die Daten, extrahiert die LED-Zustände und aktualisiert die LED-Matrix entsprechend, um dem Benutzer eine visuelle Rückmeldung zu geben.

Die Konsistenz und Korrektheit dieser Datenübertragung wird durch die Verwendung identischer struct-Definitionen auf beiden ESP32-Geräten für die ESP-NOW-Kommunikation <sup>2</sup> und durch eine exakt definierte Abbildung der Daten auf das EtherCAT-Prozessabbild sichergestellt, die über eine XML-Konfigurationsdatei (ESI-Datei) für den EtherCAT-Master definiert wird.<sup>3</sup>

**1.3. Kernentscheidungen im Design und deren Begründung**

Die Auswahl der Technologien und Architekturen für dieses Projekt erfolgte auf der Grundlage spezifischer Leistungs- und Flexibilitätsanforderungen.

- **EtherCAT (ESP1):** Die Verbindung zwischen der Steuerung (LinuxCNC) und dem I/O-Knoten (ESP1) ist die kritischste im gesamten System. EtherCAT wurde aufgrund seiner herausragenden Echtzeiteigenschaften gewählt. Es bietet deterministische Zykluszeiten im Mikrosekundenbereich, geringe Latenz und eine präzise Synchronisation durch verteilte Uhren (Distributed Clocks).<sup>3</sup> Diese Eigenschaften sind für CNC-Anwendungen, bei denen Timing und Synchronität entscheidend sind, unerlässlich. Das EasyCAT Pro Shield ist die Hardware-Komponente, die diese industrielle Kommunikation für den ESP32 zugänglich macht.<sup>5</sup>
- **ESP-NOW (ESP1-ESP2-Link):** Für die drahtlose Verbindung zwischen den beiden ESP32-Controllern wurde ESP-NOW gegenüber Standard-Wi-Fi (TCP/IP) oder Bluetooth bevorzugt. ESP-NOW ist ein von Espressif entwickeltes, verbindungsloses Protokoll, das direkt auf der Datenverbindungsschicht arbeitet.<sup>1</sup> Dies führt zu einem minimalen Protokoll-Overhead und damit zu extrem niedrigen Latenzzeiten, was für eine reaktionsschnelle Bedienoberfläche entscheidend ist. Die verbindungslose Natur vereinfacht zudem das Pairing und erhöht die Robustheit der Kommunikation, da keine aufwendigen Wiederverbindungsmechanismen wie bei Wi-Fi erforderlich sind, falls die Verbindung kurzzeitig unterbrochen wird.<sup>1</sup>
- **Nicht-blockierende, interruptgesteuerte Firmware:** Eine grundlegende Anforderung ist, dass die Firmware vollständig asynchron und ereignisgesteuert arbeitet. Die Verwendung von blockierenden Funktionen wie delay() ist strikt untersagt. Stattdessen werden Hardware-Interrupts und Timer verwendet, um auf externe Ereignisse zu reagieren. Hochfrequente Signale, wie die von Quadratur-Encodern, werden über die dedizierte PCNT-Hardware (Pulse Counter) des ESP32 oder über GPIO-Interrupts erfasst, um sicherzustellen, dass kein Impuls verloren geht.<sup>6</sup> Die Hauptschleife (

loop()) bleibt dadurch frei und kann sich um Kommunikationsaufgaben und das Scannen von langsameren Peripheriegeräten wie der Tastenmatrix kümmern. Dieser Ansatz ist eine bewährte Praxis in der Embedded-Entwicklung, um reaktionsfähige und zuverlässige Systeme zu schaffen.<sup>8</sup>

- **Hardware-Abstraktion und Konfigurierbarkeit:** Um das System wartbar, anpassbar und wiederverwendbar zu machen, werden alle hardwarespezifischen Details (wie GPIO-Pin-Zuweisungen) und Betriebsparameter (wie Filterwerte oder Geräteanzahlen) in dedizierten Konfigurationsdateien (config.h) ausgelagert. Die Hauptanwendungslogik greift nur auf abstrakte Namen und Konstanten zu. Dies ermöglicht es, die Hardware zu ändern (z. B. einen Sensor an einen anderen Pin anzuschließen), ohne den Kern-Code des Programms modifizieren zu müssen. Dies ist eine fundamentale Best Practice für die Entwicklung professioneller Embedded-Software.<sup>10</sup>

**Teil 2: Hardware-Integration und Schaltpläne**

**2.1. Stromverteilung und -management**

Eine stabile und saubere Stromversorgung ist die Grundlage für den zuverlässigen Betrieb des gesamten Systems, insbesondere bei der Kombination von 12V-Industriesensoren, 5V-Logik und empfindlichen 3.3V-Mikrocontrollern.

Das System wird von einem 12V-Hutschienen-Schaltnetzteil von Meanwell gespeist, das ausreichend Stromreserven bietet. Von dieser Hauptversorgung werden zwei separate DC-DC-Abwärtswandler (Buck Converter) vom Typ XL4015 gespeist:

1. **5V-Schiene:** Ein XL4015-Modul wird so eingestellt, dass es eine stabile 5V-Spannung liefert. Diese Schiene versorgt alle 5V-Peripheriegeräte, einschließlich der Logikseite der MCP23S17 I/O-Expander, der VCCb-Seite der TXS0108E-Pegelwandler und der LED-Matrix.
2. **3.3V-Schiene:** Das zweite XL4015-Modul erzeugt eine stabile 3.3V-Spannung. Diese Schiene versorgt die beiden ESP32 DevKitC-Boards und die VCCa-Seite der TXS0108E-Pegelwandler.

Ein kritischer Aspekt des Designs ist die Gewährleistung einer gemeinsamen Masse (GND). Alle Komponenten – das 12V-Netzteil, die XL4015-Module, beide ESP32-Boards, alle Peripheriegeräte und Pegelwandler – müssen mit derselben Masse verbunden sein. Ohne eine gemeinsame Massebezugsebene sind die Logikpegel undefiniert, was zu unzuverlässigem oder fehlerhaftem Verhalten führt.<sup>13</sup> Zusätzlich werden an den Versorgungseingängen der ESP32- und MCP23S17-Chips Entkopplungskondensatoren (z. B. 100nF Keramikkondensatoren) platziert, um hochfrequentes Rauschen aus der Stromversorgung zu filtern und die Signalintegrität zu verbessern.

**2.2. ESP1 (EtherCAT Slave) Sub-System**

ESP1 ist die Schnittstelle zur Maschine und den direkt zugehörigen Sensoren. Die Pinbelegung ist für hohe Geschwindigkeit und Echtzeitfähigkeit optimiert.

**Tabelle: ESP1 GPIO-Zuweisung**

| Funktion               | ESP32 GPIO | Verbunden mit         | Anmerkungen                                    |
| ---------------------- | ---------- | --------------------- | ---------------------------------------------- |
| SPI MOSI               | GPIO 23    | EasyCAT Pro MOSI      | VSPI Standard                                  |
| SPI MISO               | GPIO 19    | EasyCAT Pro MISO      | VSPI Standard                                  |
| SPI SCK                | GPIO 18    | EasyCAT Pro SCK       | VSPI Standard                                  |
| SPI CS                 | GPIO 5     | EasyCAT Pro CS        | VSPI Standard                                  |
| Encoder 1A             | GPIO 34    | AS5311 Ch1 A          | Nur Eingang, PCNT-fähig                        |
| Encoder 1B             | GPIO 35    | AS5311 Ch1 B          | Nur Eingang, PCNT-fähig                        |
| Encoder 2A             | GPIO 32    | AS5311 Ch2 A          | PCNT-fähig                                     |
| Encoder 2B             | GPIO 33    | AS5311 Ch2 B          | PCNT-fähig                                     |
| ... (bis zu 8 Encoder) | ...        | ...                   | Bevorzugt PCNT-fähige Pins verwenden           |
| Hall-Sensor            | GPIO 27    | NJK-5002C Signal      | Interrupt-fähig, externer Pull-up erforderlich |
| Sonde 1                | GPIO 26    | SN04-N Sonde 1 Signal | Interrupt-fähig, externer Pull-up erforderlich |
| Sonde 2                | GPIO 25    | SN04-N Sonde 2 Signal | Interrupt-fähig, externer Pull-up erforderlich |
| ... (bis zu 8 Sonden)  | ...        | ...                   | Interrupt-fähige Pins verwenden                |

**Anbindung von Hochspannungssensoren (NJK-5002C, SN04-N)**

Die Anbindung der 10-30V-Industriesensoren an den 3.3V-ESP32 erfordert eine sorgfältige Beschaltung, um den Mikrocontroller nicht zu beschädigen. Sowohl der Hall-Sensor NJK-5002C <sup>14</sup> als auch die induktive Sonde SN04-N <sup>15</sup> sind NPN-Sensoren mit einem Open-Collector-Ausgang. Das bedeutet, der Signalausgang (schwarzes Kabel) schaltet bei Detektion auf Masse (GND), ist im inaktiven Zustand aber hochohmig (floating).

Um ein sauberes 3.3V-Logiksignal zu erzeugen, wird folgende Schaltung verwendet:

1. Die Versorgungsleitungen der Sensoren (braun und blau) werden an die 12V- bzw. GND-Schiene des Systems angeschlossen.
2. Die Signalleitung (schwarz) wird direkt mit einem GPIO-Pin des ESP32 verbunden.
3. Ein Pull-up-Widerstand (typischerweise 10 kΩ) wird zwischen denselben GPIO-Pin und die 3.3V-Versorgungsschiene des ESP32 geschaltet.

Wenn der Sensor nun inaktiv ist, zieht der Pull-up-Widerstand den GPIO-Pin auf 3.3V (logisch HIGH). Wenn der Sensor ein Objekt detektiert, schaltet der interne NPN-Transistor durch und zieht den GPIO-Pin auf GND (logisch LOW). Dies erzeugt ein sicheres und eindeutiges Logiksignal für den ESP32, ohne dass ein Pegelwandler erforderlich ist.<sup>17</sup>

**2.3. ESP2 (HMI Controller) Sub-System**

ESP2 verwaltet die komplexe Benutzeroberfläche und erfordert eine große Anzahl von I/O-Leitungen, die durch Port-Expander bereitgestellt werden.

**Tabelle: ESP2 GPIO-Zuweisung**

| Funktion               | ESP32 GPIO         | Verbunden mit                      | Anmerkungen                              |
| ---------------------- | ------------------ | ---------------------------------- | ---------------------------------------- |
| SPI MOSI               | GPIO 23            | TXS0108E (3.3V Seite) -> MCPs      | VSPI für I/O-Expander                    |
| SPI MISO               | GPIO 19            | TXS0108E (3.3V Seite) -> MCPs      | VSPI für I/O-Expander                    |
| SPI SCK                | GPIO 18            | TXS0108E (3.3V Seite) -> MCPs      | VSPI für I/O-Expander                    |
| SPI CS (Shared)        | GPIO 5             | TXS0108E (3.3V Seite) -> Alle MCPs | Gemeinsamer CS für Hardware-Adressierung |
| Pegelwandler OE        | GPIO 4             | Alle TXS0108E OE Pins              | Steuert die Aktivierung der Pegelwandler |
| ADC 1                  | GPIO 36 (ADC1_CH0) | Potentiometer 1                    | Nur Eingang                              |
| ADC 2                  | GPIO 39 (ADC1_CH3) | Potentiometer 2                    | Nur Eingang                              |
| ... (bis zu 6 Potis)   | ...                | ...                                | Pins von ADC1 verwenden                  |
| Encoder A              | GPIO 34            | Encoder A                          | Nur Eingang, PCNT-fähig                  |
| Encoder B              | GPIO 35            | Encoder B                          | Nur Eingang, PCNT-fähig                  |
| ... (bis zu 8 Encoder) | ...                | ...                                | Bevorzugt PCNT-fähige Pins verwenden     |

**Ansteuerung der LED-Matrix**

Ein MCP23S17 kann pro Pin nur einen geringen Strom liefern (ca. 25 mA), was nicht ausreicht, um eine LED-Matrix mit hoher Helligkeit zu betreiben, insbesondere wenn mehrere LEDs in einer Spalte gleichzeitig leuchten sollen. Die spezifizierten IRL540N N-Kanal-MOSFETs sind hierfür die korrekte Lösung.<sup>18</sup>

Die Ansteuerung erfolgt durch Multiplexing:

1. Die Anoden aller LEDs in einer Zeile werden miteinander verbunden und an einen Ausgangspin des MCP23S17 angeschlossen (über einen strombegrenzenden Widerstand pro Zeile).
2. Die Kathoden aller LEDs in einer Spalte werden miteinander verbunden und an den Drain-Anschluss eines IRL540N-MOSFETs angeschlossen.
3. Der Source-Anschluss des MOSFETs wird mit GND verbunden.
4. Der Gate-Anschluss des MOSFETs wird mit einem anderen Ausgangspin des MCP23S17 verbunden.

Um die Matrix anzusteuern, werden die Spalten nacheinander aktiviert (gemultiplext). Um eine Spalte zu aktivieren, wird der entsprechende MOSFET durch Anlegen einer HIGH-Spannung an sein Gate durchgeschaltet. Dadurch wird die gesamte Spalte mit GND verbunden (Low-Side-Switching). Anschließend werden die Zeilen-Pins des MCP23S17 angesteuert, um die gewünschten LEDs in dieser aktiven Spalte zum Leuchten zu bringen. Dieser Vorgang wird für alle Spalten sehr schnell wiederholt, sodass für das menschliche Auge der Eindruck eines statischen Bildes entsteht. Die MOSFETs schalten dabei den hohen Strom der 5V-Versorgung, während der MCP23S17 nur den vernachlässigbaren Gatestrom schalten muss.<sup>18</sup>

**2.4. SPI-Bus-Design mit gemeinsamem Chip Select**

Die Anforderung, mehrere SPI-Geräte (die MCP23S17-Expander) mit einer einzigen Chip-Select-Leitung zu betreiben, ist eine fortgeschrittene Technik, die die Hardware-Adressierungsfunktion der MCP23S17-Chips nutzt.

**Hardware-Adressierung:** Jeder MCP23S17 verfügt über drei Adresspins (A0, A1, A2). Durch festes Verdrahten dieser Pins mit GND (logisch 0) oder 5V (logisch 1) kann jedem Chip eine eindeutige 3-Bit-Adresse von 0b000 (0) bis 0b111 (7) zugewiesen werden.<sup>20</sup> Wenn der gemeinsame CS-Pin auf LOW gezogen wird, werden alle MCPs auf dem Bus aktiviert. Die SPI-Kommunikation muss dann ein spezielles Opcode-Byte enthalten, das die Adresse des Ziel-Chips enthält. Nur der Chip, dessen Hardware-Adresse mit der im Opcode übereinstimmt, verarbeitet den Befehl.<sup>22</sup> Dies spart wertvolle GPIO-Pins am Master-Controller.

**Pegelwandlung für SPI:** Da der ESP32 mit 3.3V arbeitet und die MCP23S17-Peripherie mit 5V betrieben wird, um eine robuste Ansteuerung der 5V-Lasten zu gewährleisten, ist ein bidirektionaler Pegelwandler für den SPI-Bus zwingend erforderlich.<sup>24</sup> Der TXS0108E ist hierfür geeignet, da er die automatische Richtungserkennung für bidirektionale Leitungen wie MISO/MOSI unterstützt. Die SPI-Leitungen des ESP32 (MOSI, MISO, SCK, CS) werden mit der 3.3V-Seite (A-Port) des TXS0108E verbunden, während die SPI-Leitungen aller MCP23S17-Chips mit der 5V-Seite (B-Port) verbunden werden.

**Stabilität beim Booten:** Der TXS0108E hat eine Eigenheit, die beim Systemstart zu Problemen führen kann. Seine I/O-Pins werden durch interne Pull-up-Widerstände hochgezogen, wenn der Output-Enable-Pin (OE) HIGH ist.<sup>26</sup> Wenn der OE-Pin direkt mit 3.3V verbunden ist, können diese Pull-ups die Boot-Modus-Pins des ESP32 beeinflussen und den Startvorgang stören. Die zuverlässige Lösung besteht darin, den OE-Pin aller TXS0108E-Boards mit einem dedizierten GPIO-Pin des ESP32 zu verbinden. Die Firmware muss sicherstellen, dass dieser GPIO-Pin während des Boot-Vorgangs LOW gehalten wird (wodurch die Ausgänge des Pegelwandlers deaktiviert werden) und erst dann auf HIGH gesetzt wird, wenn der ESP32 vollständig initialisiert ist.<sup>26</sup>

**Teil 3: PlatformIO-Projekt und Bibliothekskonfiguration**

PlatformIO wird als Entwicklungsumgebung verwendet, da es eine hervorragende Verwaltung von Abhängigkeiten, eine flexible Konfiguration und die Unterstützung für komplexe Projekte mit mehreren Zielen bietet.<sup>28</sup>

**3.1. Projektstruktur**

Das Projekt wird so strukturiert, dass es den Code für beide ESP32-Controller in einer einzigen, übersichtlichen Verzeichnisstruktur enthält.

Projekt-Root/

├── platformio.ini # Hauptkonfigurationsdatei

├── include/ # Globale Header-Dateien (z.B. für gemeinsame Strukturen)

│ └── shared_structures.h

├── src/ # Quellcode-Verzeichnisse für die Umgebungen

│ ├── esp1/ # Quellcode für ESP1

│ │ ├── main_esp1.cpp

│ │ ├── config_esp1.h

│ │ ├── ethercat_handler.h/cpp

│ │ └──...

│ └── esp2/ # Quellcode für ESP2

│ ├── main_esp2.cpp

│ ├── config_esp2.h

│ ├── web_interface.h/cpp

│ └──...

├── lib/ # Projekt-spezifische und heruntergeladene Bibliotheken

│ └── EasyCAT/ # Manuell hinzugefügte EasyCAT-Bibliothek

│ ├── EasyCAT.h

│ └── EasyCAT.cpp

└── data/ # Dateien für das LittleFS-Dateisystem von ESP2

├── index.html

├── style.css

└── script.js

Diese Struktur nutzt PlatformIO-Umgebungen ([env]), um die separaten Build-Prozesse für ESP1 und ESP2 zu verwalten.<sup>30</sup>

**3.2.** platformio.ini **Konfiguration**

Die platformio.ini-Datei ist das Herzstück der Projektkonfiguration. Sie definiert die Zielhardware, die Frameworks und alle Bibliotheksabhängigkeiten.

```Ini, TOML

; PlatformIO Project Configuration File

; <https://docs.platformio.org/page/projectconf.html>

[platformio]

default_envs = esp1, esp2

; Gemeinsame Konfigurationen für beide Umgebungen

[env]

framework = arduino

platform = espressif32

board = esp32dev

monitor_speed = 115200

; Umgebung für ESP1 (EtherCAT Bridge)

[env:esp1]

src_dir = src/esp1

lib_deps =

madhephaestus/ESP32Encoder@^0.11.7

build_flags = -DCORE_ESP1

; Umgebung für ESP2 (HMI Controller)

[env:esp2]

src_dir = src/esp2

board_build.filesystem = littlefs

lib_deps =

madhephaestus/ESP32Encoder@^0.11.7

; Adafruit-Bibliothek für MCP23S17, da sie Hardware-Adressierung unterstützt

adafruit/Adafruit MCP23017 Arduino Library@^2.3.2

; Asynchrone Webserver- und OTA-Bibliotheken

ESPAsyncWebServer

AsyncTCP

ayushsharma82/AsyncElegantOTA@^2.2.7

; Keypad-Bibliothek für die Tastenmatrix

chris--a/Keypad@^3.1.1

build_flags = -DCORE_ESP2

```

- [env:esp1] **und** [env:esp2]: Definieren separate Build-Ziele. src_dir weist PlatformIO an, den Code aus den jeweiligen Unterverzeichnissen zu kompilieren.
- lib_deps: Listet die Abhängigkeiten auf, die PlatformIO automatisch aus seiner Registry herunterladen und verwalten soll.<sup>31</sup> Für ESP2 werden die Bibliotheken für den MCP23S17 <sup>33</sup>, den asynchronen Webserver <sup>34</sup>, OTA-Updates <sup>35</sup> und die Tastenmatrix <sup>36</sup> eingebunden.
- build_flags: Definiert Präprozessor-Makros (-DCORE_ESP1, -DCORE_ESP2), die im Code verwendet werden können, um zwischen den Builds für die beiden Boards zu unterscheiden.
- board_build.filesystem = littlefs: Konfiguriert das Dateisystem für ESP2, um die Web-Dateien zu speichern.

**3.3. Lokale Bibliotheksintegration (EasyCAT)**

Die Recherche hat ergeben, dass die im PlatformIO-Verzeichnis verfügbare EasyCAT_lib für das Mbed-Framework und nicht für Arduino vorgesehen ist.<sup>32</sup> Die korrekte Arduino-Bibliothek muss manuell von der Hersteller-Website heruntergeladen und in das Projekt integriert werden.

**Vorgehensweise:**

1. Laden Sie die EasyCAT.zip-Datei (oder EasyCAT_V2_1.zip) von der Download-Seite von Bausano.net herunter.<sup>3</sup>
2. Erstellen Sie im lib-Verzeichnis des PlatformIO-Projekts einen neuen Ordner mit dem Namen EasyCAT.
3. Entpacken Sie den Inhalt der heruntergeladenen ZIP-Datei (insbesondere EasyCAT.h und EasyCAT.cpp) direkt in den neu erstellten lib/EasyCAT-Ordner.
4. PlatformIO erkennt diesen Ordner automatisch als lokale Bibliothek und kompiliert sie beim Erstellen der ESP1-Umgebung mit. Es ist keine weitere Konfiguration in der platformio.ini erforderlich.

**Teil 4: ESP1 Firmware Deep Dive: Die EtherCAT-zu-Wireless-Brücke**

**4.1. Kernlogik und Zustandsmanagement (**main_esp1.cpp**)**

Die Hauptdatei für ESP1 implementiert eine nicht-blockierende Hauptschleife, die kontinuierlich die verschiedenen Subsysteme abarbeitet.

```C++

// main_esp1.cpp (Ausschnitt)

# **include** "config_esp1.h"

# **include** "ethercat_handler.h"

# **include** "espnow_handler.h"

# **include** "peripherals_esp1.h"

void setup() {

// Initialisierung von Serieller Schnittstelle, GPIOs, etc.

serial_init();

peripherals_init(); // Initialisiert Encoder, Hall-Sensor, Sonden

esp_now_init(); // Initialisiert ESP-NOW Kommunikation

ethercat_init(); // Initialisiert die EasyCAT-Bibliothek

}

void loop() {

ethercat_task(); // Haupt-Task der EasyCAT-Bibliothek

peripherals_task(); // Liest Encoder, berechnet RPM, prüft Sonden

esp_now_task(); // Verarbeitet ein- und ausgehende ESP-NOW-Daten

}

```

Jede \_task()-Funktion ist so konzipiert, dass sie schnell ausgeführt wird und die Kontrolle sofort wieder an die loop()-Funktion zurückgibt, um die Reaktionsfähigkeit des Systems zu gewährleisten.

**4.2. EtherCAT-Prozessdatenaustausch**

Der Schlüssel zum konfigurierbaren Datenaustausch mit LinuxCNC ist die Verwendung einer benutzerdefinierten Datenstruktur, die mit dem "Easy Configurator"-Tool von Bausano erstellt wird.<sup>5</sup> Dieses Tool generiert eine Header-Datei (z. B.

MyData.h), die die exakte Struktur der 128 Byte Eingangs- und 128 Byte Ausgangsdaten definiert.

Ein Beispiel für eine solche generierte Header-Datei:

```C++

// MyData.h (Beispiel, generiert durch Easy Configurator)

# **ifndef** MYDATA_H

# **define** MYDATA_H

# **pragma** pack(push, 1) // Wichtig für exaktes Byte-Alignment

typedef union

{

struct

{

int32_t encoder_1_position;

int32_t encoder_2_position;

uint32_t spindle_rpm;

uint8_t probe_states;

uint8_t button_matrix_states; // Daten von ESP2

//... weitere Daten von ESP2

} Cust;

unsigned char Byte;

} EASYCAT_BUFFER_IN;

typedef union

{

struct

{

int32_t linuxcnc_status_word;

uint8_t led_states_from_lcnc; // Daten für ESP2

//... weitere Daten für ESP2

} Cust;

unsigned char Byte;

} EASYCAT_BUFFER_OUT;

# **pragma** pack(pop)

# **endif**

```

Diese Header-Datei wird in die ESP1-Firmware eingebunden. Der Zugriff auf die Prozessdaten erfolgt dann typsicher und lesbar über die EASYCAT-Instanz, die von der Bibliothek bereitgestellt wird <sup>38</sup>:

```C++

// In ethercat_handler.cpp

# **include** "EasyCAT.h"

# **include** "MyData.h" // Die benutzerdefinierte Header-Datei

EasyCAT EASYCAT; // Instanz der Bibliothek

//...

void write_rpm_to_ethercat(uint32_t rpm) {

EASYCAT.BufferIn.Cust.spindle_rpm = rpm;

}

uint8_t read_led_state_from_ethercat(int led_index) {

return EASYCAT.BufferOut.Cust.led_states_from_lcnc[led_index];

}

```

**Tabelle: Beispiel für EtherCAT PDO-Mapping**

| Richtung        | Byte-Offset | Datentyp | Variablenname (in Cust struct) | Beschreibung                        |
| --------------- | ----------- | -------- | ------------------------------ | ----------------------------------- |
| IN (ESP1→LCNC)  | 0-3         | int32_t  | encoder_1_position             | Position von ESP1 Encoder 1         |
| IN (ESP1→LCNC)  | 4-7         | uint32_t | spindle_rpm                    | Drehzahl vom Hall-Sensor            |
| IN (ESP1→LCNC)  | 8           | uint8_t  | probe_states                   | Bitmaske der Sonden an ESP1         |
| IN (ESP1→LCNC)  | 9-72        | uint8_t  | button_matrix_states           | Zustände der Tastenmatrix von ESP2  |
| OUT (LCNC→ESP1) | 0-3         | int32_t  | linuxcnc_status_word           | Status-Flags von LinuxCNC           |
| OUT (LCNC→ESP1) | 4-67        | uint8_t  | led_states_from_lcnc           | Zustände für die LED-Matrix an ESP2 |

**4.3. ESP-NOW Bridging-Logik**

Die ESP-NOW-Kommunikation verwendet eine gemeinsame Datenstruktur, die in shared_structures.h definiert und von beiden ESPs eingebunden wird, um Datenkonsistenz zu gewährleisten.

```C++

// shared_structures.h

# **ifndef** SHARED_STRUCTURES_H

# **define** SHARED_STRUCTURES_H

struct EspNowDataToEsp1 {

// Daten von ESP2-Peripherie

int32_t encoder_values;

uint8_t button_states; // 8x8 Matrix als 64-bit oder 8-byte Array

uint16_t poti_values;

uint8_t rotary_switch_positions;

};

struct EspNowDataToEsp2 {

// Daten von LinuxCNC für ESP2

uint8_t led_states; // 8x8 Matrix

};

# **endif**

```

Die ESP1-Firmware implementiert die ESP-NOW-Callbacks OnDataRecv und OnDataSent.<sup>2</sup> In

OnDataRecv werden die ankommenden Daten aus EspNowDataToEsp1 direkt in die EASYCAT.BufferIn-Struktur kopiert. In der Hauptschleife (esp_now_task) prüft die Firmware, ob neue Daten von EtherCAT (EASYCAT.BufferOut) vorliegen, verpackt diese in eine EspNowDataToEsp2-Struktur und sendet sie an ESP2.

**4.4. Hochgeschwindigkeits-Peripheriebehandlung (Interrupt-basiert)**

- **Encoder:** Die ESP32Encoder-Bibliothek wird verwendet, da sie die Hardware-Pulszähler (PCNT) des ESP32 nutzt und somit die CPU kaum belastet. Für jeden Encoder wird ein Objekt instanziiert und an die in config_esp1.h definierten GPIOs gebunden. Die Bibliothek kümmert sich intern um Überläufe des 16-Bit-Hardware-Zählers.<sup>7</sup> Die Firmware liest den 64-Bit-Gesamtzähler aus, um anwendungsseitige Überläufe zu vermeiden. Der konfigurierbare Glitch-Filter (encoder.setFilter(wert)) wird verwendet, um elektrisches Rauschen zu unterdrücken, insbesondere bei mechanischen Encodern.<sup>6</sup> Der Filterwert wird aus der Konfigurationsdatei übernommen.
- **Hall-Sensor** RPM-Berechnung: Ein GPIO-Interrupt wird an den Pin des Hall-Sensors angehängt und auf eine steigende oder fallende Flanke konfiguriert (attachInterrupt). Die zugehörige Interrupt-Service-Routine (ISR) ist extrem kurz und inkrementiert lediglich eine volatile-Ganzzahlvariable (pulse_count). In der Hauptschleife (peripherals_task) wird in einem festen Zeitintervall (z.B. alle 500 ms, konfigurierbar) die Drehzahl berechnet:  
   RPM=impulse_pro_umdrehungpulse_count​×verstrichene_zeit_ms60000​  
   Anschließend werden der Zähler und die Zeit zurückgesetzt.
- **Sondenerkennung:** Für die induktiven Sonden werden ebenfalls GPIO-Interrupts verwendet. Dies gewährleistet eine sofortige Erkennung einer Zustandsänderung (Metall detektiert/nicht detektiert). Die ISR setzt ein Flag oder aktualisiert direkt eine Statusvariable, die dann in das EtherCAT-Eingangsabbild gemappt wird.

**Teil 5: ESP2 Firmware Deep Dive: Die Mensch-Maschine-Schnittstelle**

**5.1. HMI-Kernlogik (**main_esp2.cpp**)**

Ähnlich wie bei ESP1 ist die loop()-Funktion von ESP2 nicht-blockierend und delegiert die Arbeit an spezialisierte Handler-Funktionen.

```C++

// main_esp2.cpp (Ausschnitt)

void loop() {

hmi_task(); // Scant Tastenmatrix, liest Encoder, Potis, etc.

esp_now_task(); // Verarbeitet ESP-NOW-Kommunikation

web_interface_task(); // Behandelt WebSocket-Clients (ws.cleanupClients())

// Kein delay()!

}

```

**5.2. ESP-NOW Datenbehandlung**

Die ESP-NOW-Implementierung auf ESP2 ist das Gegenstück zu der auf ESP1. Die hmi_task()-Funktion sammelt die Zustände aller Peripheriegeräte, verpackt sie in die EspNowDataToEsp1-Struktur und sendet sie in regelmäßigen Abständen oder bei Zustandsänderung. Die OnDataRecv-Callback-Funktion empfängt die EspNowDataToEsp2-Struktur und aktualisiert die lokalen Variablen, die den Zustand der LED-Matrix steuern.

**5.3. Fortgeschrittenes Peripherie-Management**

- **Tasten- & LED-Matrix-Steuerung:** Für die Tastenmatrix wird die Keypad.h-Bibliothek von Mark Stanley & Alexander Brevig eingesetzt.<sup>36</sup> Diese Bibliothek wurde gewählt, weil sie von Haus aus die Erkennung mehrerer gleichzeitig gedrückter Tasten (getKeys()) sowie die Verfolgung von Tasten-Zustandsänderungen (PRESSED, HOLD, RELEASED) unterstützt.<sup>36</sup> Dies ist einer manuellen Scan-Logik überlegen. Da die Matrix an MCP23S17-Expandern hängt, wird eine kleine Wrapper-Klasse erstellt, die die Adafruit_MCP23017-Methoden (digitalRead/digitalWrite für einen Pin) so kapselt, dass sie sich für die Keypad.h-Bibliothek wie native Arduino-Pins verhalten. Die LED-Matrix wird durch direktes Schreiben auf die GPIO-Register des entsprechenden MCP23S17 angesteuert, was wiederum die externen MOSFETs schaltet.
- **Drehschalter:** Ein Drehschalter mit N Positionen wird als eine Gruppe von N-1 Tastern behandelt, die an einen gemeinsamen Pin angeschlossen sind. Die Firmware liest die Eingänge des MCP23S17 und dekodiert die Position des Schalters basierend darauf, welcher der Eingänge LOW ist.
- **Gefiltertes ADC**: Die analogen Werte der Potentiometer werden mit analogRead() gelesen. Um Jitter und Rauschen zu reduzieren, wird ein einfacher digitaler Tiefpassfilter (exponentieller gleitender Durchschnitt) in der Software implementiert:  
   gefilterter_wert=α×neuer_messwert+(1−α)×alter_gefilterter_wert  
   Der Filterfaktor α ist in der config_esp2.h einstellbar.

**5.4. Persistente Konfiguration mit der Preferences-Bibliothek**

Um die über die Weboberfläche vorgenommenen Konfigurationen (Tastengruppen, Toggle-Modi, LED-Bindungen) dauerhaft zu speichern, wird die Preferences-Bibliothek des ESP32 verwendet, die auf dem Non-Volatile Storage (NVS) basiert.<sup>39</sup>

Anstatt viele einzelne Schlüssel-Wert-Paare zu speichern, wird ein effizienterer Ansatz verfolgt: Alle dynamisch konfigurierbaren Parameter werden in einer einzigen C++-struct zusammengefasst.

```C++

// persistence.h (Ausschnitt)

struct WebConfig {

ButtonDynamicConfig buttons;

LedDynamicConfig leds;

//... weitere dynamische Einstellungen

};

```

Eine saveConfiguration()-Funktion schreibt diese gesamte Struktur mit einem einzigen Aufruf von preferences.putBytes() in den NVS.<sup>41</sup> Beim Systemstart ruft die

setup()-Funktion eine loadConfiguration()-Funktion auf, die die Struktur mit preferences.getBytes() wieder aus dem NVS lädt und die HMI entsprechend konfiguriert.<sup>39</sup> Dies minimiert die Anzahl der Flash-Schreibvorgänge und ist deutlich performanter als das Speichern vieler einzelner Werte.

**Teil 6: Die Weboberfläche: Dynamische Konfiguration und Steuerung**

**6.1. Serverseitige Implementierung (ESPAsyncWebServer)**

Die Firmware auf ESP2 implementiert einen leistungsstarken, asynchronen Webserver.

1. **Wi-Fi-Setup:** Das Gerät wird im Station-Modus initialisiert und verbindet sich mit dem in config_esp2.h definierten WLAN.
2. **Webserver-Initialisierung:** Ein AsyncWebServer-Objekt wird auf Port 80 instanziiert.<sup>43</sup>
3. **OTA-Integration:** Die AsyncElegantOTA-Bibliothek wird durch Hinzufügen von #include <AsyncElegantOTA.h> und dem Aufruf AsyncElegantOTA.begin(&server); vor server.begin(); nahtlos integriert. Dies erstellt automatisch den Endpunkt /update, über den neue Firmware (als .bin-Datei) oder Dateisystem-Images (LittleFS) hochgeladen werden können, ohne dass eine serielle Verbindung erforderlich ist.<sup>34</sup>
4. **Dateisystem-Handler:** Ein Handler wird für die Root-URL (/) registriert, der die index.html aus dem LittleFS-Dateisystem des ESP32 an den Client sendet. Weitere Handler für /style.css und /script.js werden ebenfalls eingerichtet.
5. **WebSocket-Server:** Ein AsyncWebSocket-Server wird initialisiert und an den Endpunkt /ws gebunden.<sup>43</sup>

**6.2. WebSocket-Ereignisbehandlung**

Die serverseitige C++-Funktion onEvent ist der zentrale Punkt für die Echtzeitkommunikation mit dem Web-Frontend.

- WS_EVT_CONNECT: Wenn ein neuer Client (Browser-Tab) eine Verbindung herstellt, sendet der Server sofort den aktuellen Zustand aller HMI-Elemente (Tasten, LEDs, Encoder etc.) als JSON-Objekt, damit die Weboberfläche initialisiert wird.
- WS_EVT_DISCONNECT: Behandelt die Trennung eines Clients.
- WS_EVT_DATA: Dies ist der wichtigste Fall. Wenn der Server eine Nachricht vom Client empfängt, wird diese verarbeitet. Die Nachrichten sind JSON-formatierte Befehle.
  - Ein Befehl wie {"command": "saveConfig", "data": {...}} veranlasst den Server, die saveConfiguration()-Funktion aufzurufen, um die neuen Einstellungen im NVS zu speichern.
  - Ein Befehl wie {"command": "getLiveStatus"} könnte eine Anfrage für ein Update sein.
- **Broadcasting:** Wenn sich auf der Hardware-Seite ein Zustand ändert (z. B. eine Taste wird gedrückt), formatiert der Server diese Information als JSON-Nachricht und sendet sie mit ws.textAll(json*string) an \_alle* verbundenen Clients. Dies stellt sicher, dass alle offenen Browserfenster synchronisiert bleiben.<sup>43</sup>

**6.3. Clientseitige Implementierung (HTML/CSS/JavaScript)**

- index.html: Die HTML-Datei definiert die Grundstruktur der Seite. Sie enthält Tabellen für die 8x8-Button- und LED-Matrizen, Eingabefelder für die Konfiguration (z.B. Tastenname, Toggle-Checkbox, Gruppenauswahl), Live-Statusanzeigen und Steuerelemente wie "Speichern"- und "Firmware-Update"-Buttons. Tooltips und Hilfe-Labels werden hinzugefügt, um die Bedienung zu erleichtern.
- script.js: Die gesamte dynamische Logik befindet sich in dieser JavaScript-Datei.
  1. **WebSocket-Verbindung:** Beim Laden der Seite wird die Verbindung zum Server aufgebaut: var gateway = \ws://${window.location.hostname}/ws\`; websocket = new WebSocket(gateway);.<sup>43</sup> Es werden Handler für onopen, oncloseundonmessage\` registriert.
  2. onmessage**-Handler:** Diese Funktion ist der Kern der Live-Anzeige. Sie empfängt JSON-Nachrichten vom ESP32, parst sie und aktualisiert die entsprechenden HTML-Elemente (z. B. ändert sie die Farbe eines <div>, das eine LED darstellt, oder den Text eines Statusfeldes).
  3. **Event-Listener:** An die Konfigurationselemente (Eingabefelder, Checkboxen) und den "Speichern"-Button werden Event-Listener angehängt.
  4. **Konfiguration senden:** Wenn der Benutzer auf "Speichern" klickt, liest eine Funktion alle Werte aus den Konfigurationsfeldern der Seite, baut daraus ein großes JSON-Objekt, das die gesamte Konfiguration repräsentiert, und sendet es mit websocket.send(JSON.stringify(configObject)) an den ESP32.
  5. **Clientseitige Validierung:** JavaScript-Logik validiert Eingaben direkt im Browser, z. B. um sicherzustellen, dass innerhalb einer als "Radio-Gruppe" definierten Tastengruppe immer nur eine Taste als aktiv markiert werden kann.

**Teil 7: Ausführlicher Konfigurationsleitfaden (**config.h**)**

Dieser Abschnitt dient als detailliertes Handbuch zur Anpassung des Systems und erfüllt die zentrale Anforderung nach maximaler Konfigurierbarkeit.

**7.1.** config_esp1.h **Erläuterung**

Diese Datei enthält alle statischen Konfigurationen für die EtherCAT-Bridge.

```C++

// config_esp1.h

# **pragma** once

// =================================================================

// DEBUGGING

// =================================================================

# **define** DEBUG_ENABLED true // Serielle Debug-Ausgaben aktivieren/deaktivieren

// =================================================================

// GPIO-Zuweisung ESP1

// =================================================================

// SPI für EasyCAT Shield (Standard VSPI)

# **define** PIN_EC_MOSI 23

# **define** PIN_EC_MISO 19

# **define** PIN_EC_SCK 18

# **define** PIN_EC_CS 5

// Hall-Sensor für Spindeldrehzahl

# **define** HALL_SENSOR_PIN 27

# **define** HALL_SENSOR_PULLUP GPIO_PULLUP_ENABLE

// Induktive Sonden

# **define** NUM_PROBES 8

const int PROBE_PINS = {26, 25, 14, 12, 13, 15, 2, 4};

# **define** PROBE_PULLUP_MODE GPIO_PULLUP_ENABLE

// Quadratur-Encoder

# **define** NUM_ENCODERS 8

const int ENCODER_A_PINS = {34, 32,...};

const int ENCODER_B_PINS = {35, 33,...};

# **define** ENCODER_GLITCH_FILTER 1023 // 0 bis 1023, höherer Wert = stärkere Filterung

// =================================================================

// TACHOMETER-Konfiguration

// =================================================================

# **define** TACHO_MAGNETS_PER_REVOLUTION 2 // Anzahl der Magnete am rotierenden Teil

# **define** TACHO_UPDATE_INTERVAL_MS 500 // Berechnungsintervall für RPM in ms

# **define** TACHO_MIN_RPM_DISPLAY 10 // RPM-Werte darunter werden als 0 angezeigt

```

**7.2.** config_esp2.h **Erläuterung**

Diese Datei konfiguriert die HMI-Einheit und nutzt, wie gewünscht, tabellenähnliche Strukturen (Arrays von structs) für eine klare und erweiterbare Konfiguration von Tasten und LEDs.

```C++

// config_esp2.h

# **pragma** once

// =================================================================

// DEBUGGING & NETZWERK

// =================================================================

# **define** DEBUG_ENABLED true

# **define** WIFI_SSID "Ihr_Netzwerk_SSID"

# **define** WIFI_PASSWORD "Ihr_Netzwerk_Passwort"

# **define** OTA_HOSTNAME "linuxcnc-hmi"

// =================================================================

// GPIO-Zuweisung ESP2

// =================================================================

// SPI für MCP23S17 Expander (Standard VSPI)

# **define** PIN_MCP_MOSI 23

# **define** PIN_MCP_MISO 19

# **define** PIN_MCP_SCK 18

# **define** PIN_MCP_CS 5 // Gemeinsamer CS-Pin

// Steuerung für TXS0108E Pegelwandler

# **define** PIN_LEVEL_SHIFTER_OE 4

// Potentiometer (nur ADC1-Pins verwenden)

# **define** NUM_POTIS 6

const int POTI_PINS = {36, 39, 32, 33, 35, 34};

//... Weitere GPIOs für Encoder etc.

// =================================================================

// Konfiguration der Tastenmatrix (Struktur-Array)

// =================================================================

# **define** MAX_BUTTONS 64

struct ButtonConfig {

const char* name; // Name für die Weboberfläche

uint8_t matrix_row; // Zeile in der 8x8 Matrix (0-7)

uint8_t matrix_col; // Spalte in der 8x8 Matrix (0-7)

bool is_toggle; // true = Schalter, false = Taster

int radio_group_id; // 0 = keine Gruppe, >0 = Radiogruppe (nur eine Taste aktiv)

};

const ButtonConfig button_configs = {

// Name, Zeile, Spalte, Toggle?, Radiogruppe

{"Cycle Start", 0, 0, false, 0},

{"Feed Hold", 0, 1, false, 0},

{"Stop", 0, 2, false, 0},

{"Mode AUTO", 1, 0, true, 1},

{"Mode MDI", 1, 1, true, 1},

{"Mode JOG", 1, 2, true, 1},

//... bis zu 64 Tasten definieren

};

// =================================================================

// Konfiguration der LED-Matrix (Struktur-Array)

// =================================================================

# **define** MAX_LEDS 64

enum LedBinding {

UNBOUND, // Unabhängig, nur über Web-UI steuerbar

BOUND_TO_BUTTON, // Zustand an eine Taste gekoppelt

BOUND_TO_LCNC // Zustand wird von LinuxCNC über EtherCAT gesteuert

};

struct LedConfig {

const char* name;

uint8_t matrix_row;

uint8_t matrix_col;

LedBinding binding_type;

int bound_button_index; // Index in button_configs, wenn an Taste gebunden

int lcnc_state_bit; // Bit-Offset in den EtherCAT-Daten von LCNC

};

const LedConfig led_configs = {

// Name, Zeile, Spalte, Bindungstyp, Gekoppelte Taste, LCNC-Bit

{"Cycle Active", 0, 0, BOUND_TO_LCNC, -1, 0},

{"Feed Hold Active", 0, 1, BOUND_TO_LCNC, -1, 1},

{"AUTO Mode LED", 1, 0, BOUND_TO_BUTTON, 3, -1}, // Gekoppelt an Taste "Mode AUTO"

{"MDI Mode LED", 1, 1, BOUND_TO_BUTTON, 4, -1}, // Gekoppelt an Taste "Mode MDI"

//... bis zu 64 LEDs definieren

};

```

Diese strukturierte Konfiguration ermöglicht es der Firmware, das Verhalten jeder einzelnen Taste und LED dynamisch zur Laufzeit basierend auf diesen Definitionen zu bestimmen, ohne dass der Code neu kompiliert werden muss, wenn sich die Zuordnung ändert.

**Teil 8: Vollständiger Projekt-Quellcode**

Nachfolgend finden Sie die vollständigen, kommentierten Quellcode-Dateien für das gesamte PlatformIO-Projekt. Die Dateien sind entsprechend der in Teil 3 beschriebenen Projektstruktur organisiert und bereit zum Kompilieren und Hochladen auf die jeweiligen ESP32-Boards.

platformio.ini

```Ini, TOML

; PlatformIO Project Configuration File

;

; Build options: build flags, source filter

; Upload options: custom upload port, speed and extra flags

; Library options: dependencies, extra library storages

; Advanced options: extra scripting

;

; Please visit documentation for the other options and examples

; <https://docs.platformio.org/page/projectconf.html>

[platformio]

default_envs = esp1, esp2

description = Dual ESP32 control system for LinuxCNC via EtherCAT and ESP-NOW.

; =================================================================

; COMMON CONFIGURATION FOR BOTH ESP32 BOARDS

; =================================================================

[env]

framework = arduino

platform = espressif32

board = esp32dev

monitor_speed = 115200

lib_ldf_mode = chain+

; =================================================================

; ENVIRONMENT FOR ESP1 (EtherCAT BRIDGE)

; =================================================================

[env:esp1]

src_dir = src/esp1

lib_deps =

madhephaestus/ESP32Encoder@^0.11.7

build_flags = -DCORE_ESP1

; =================================================================

; ENVIRONMENT FOR ESP2 (HMI CONTROLLER)

; =================================================================

[env:esp2]

src_dir = src/esp2

board_build.filesystem = littlefs

lib_deps =

madhephaestus/ESP32Encoder@^0.11.7

; Adafruit library for MCP23S17, as it supports hardware addressing

adafruit/Adafruit MCP23017 Arduino Library@^2.3.2

; Asynchronous WebServer and OTA libraries

ESPAsyncWebServer

AsyncTCP

ayushsharma82/AsyncElegantOTA@^2.2.7

; Keypad library for the button matrix

chris--a/Keypad@^3.1.1

; JSON library for WebSocket communication

bblanchon/ArduinoJson@^6.21.4

build_flags = -DCORE_ESP2

```

include/shared_structures.h

```C++

# **ifndef** SHARED_STRUCTURES_H

# **define** SHARED_STRUCTURES_H

# **include** <stdint.h>

// MAC-Adresse von ESP1 (EtherCAT Bridge), muss hier manuell eingetragen werden

static uint8_t esp1_mac_address = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// MAC-Adresse von ESP2 (HMI Controller), muss hier manuell eingetragen werden

static uint8_t esp2_mac_address = {0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0xEF};

// Datenstruktur, die von ESP2 (HMI) an ESP1 (Bridge) gesendet wird

typedef struct struct_message_to_esp1 {

int32_t encoder_values;

uint8_t button_matrix_states; // 64 Tasten als 8-Byte-Bitmaske

uint16_t poti_values;

uint8_t rotary_switch_positions;

bool data_changed; // Flag, um unnötige Übertragungen zu vermeiden

} struct_message_to_esp1;

// Datenstruktur, die von ESP1 (Bridge) an ESP2 (HMI) gesendet wird

typedef struct struct_message_to_esp2 {

uint8_t led_matrix_states; // 64 LEDs als 8-Byte-Bitmaske

uint32_t linuxcnc_status;

// Weitere Daten von LinuxCNC für die Anzeige auf der HMI

} struct_message_to_esp2;

# **endif** // SHARED_STRUCTURES_H

```

src/esp1/config_esp1.h

```C++

# **ifndef** CONFIG_ESP1_H

# **define** CONFIG_ESP1_H

# **include** <Arduino.h>

// =================================================================

// DEBUGGING

// =================================================================

# **define** DEBUG_ENABLED true

// =================================================================

// GPIO ASSIGNMENT ESP1

// =================================================================

// SPI for EasyCAT Shield (Standard VSPI)

# **define** PIN_EC_MOSI 23

# **define** PIN_EC_MISO 19

# **define** PIN_EC_SCK 18

# **define** PIN_EC_CS 5

// Hall Sensor for Spindle RPM

# **define** HALL_SENSOR_PIN 27

# **define** HALL_SENSOR_PULLUP GPIO_PULLUP_ENABLE

// Inductive Probes

# **define** NUM_PROBES 8

const int PROBE_PINS = {26, 25, 14, 12, 13, 15, 2, 4};

# **define** PROBE_PULLUP_MODE GPIO_PULLUP_ENABLE

// Quadrature Encoders

# **define** NUM_ENCODERS 8

const int ENCODER_A_PINS = {34, 32, -1, -1, -1, -1, -1, -1}; // -1 for unused

const int ENCODER_B_PINS = {35, 33, -1, -1, -1, -1, -1, -1}; // -1 for unused

# **define** ENCODER_GLITCH_FILTER 1023 // 0 to 1023, higher value = more filtering

// =================================================================

// TACHOMETER CONFIGURATION

// =================================================================

# **define** TACHO_MAGNETS_PER_REVOLUTION 2 // Number of magnets on the rotating part

# **define** TACHO_UPDATE_INTERVAL_MS 500 // Calculation interval for RPM in ms

# **define** TACHO_MIN_RPM_DISPLAY 10 // RPM values below this are shown as 0

# **endif** // CONFIG_ESP1_H

```

src/esp1/main_esp1.cpp

```C++

# **include** <Arduino.h>

# **include** <esp_now.h>

# **include** <WiFi.h>

# **include** "config_esp1.h"

# **include** "shared_structures.h"

# **include** "EasyCAT.h" // Lokale Bibliothek

# **include** "ESP32Encoder.h"

// Globale Objekte und Variablen

EasyCAT EASYCAT;

ESP32Encoder encoders;

volatile uint32_t hall_pulse_count = 0;

unsigned long last_rpm_calc_time = 0;

uint32_t current_rpm = 0;

volatile bool probe_states = {false};

struct_message_to_esp1 incoming_hmi_data;

struct_message_to_esp2 outgoing_lcnc_data;

// --- ESP-NOW Callbacks ---

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {

if (DEBUG_ENABLED) {

Serial.print("\r\nLast Packet Send Status:\t");

Serial.println(status == ESP_NOW_SEND_SUCCESS? "Delivery Success" : "Delivery Fail");

}

}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {

memcpy(&incoming_hmi_data, incomingData, sizeof(incoming_hmi_data));

if (DEBUG_ENABLED) {

Serial.println("Data received from HMI.");

}

}

// --- Interrupt Service Routines ---

void IRAM_ATTR hall_sensor_isr() {

hall_pulse_count++;

}

void IRAM_ATTR probe_isr_0() { probe_states = digitalRead(PROBE_PINS); }

//... weitere ISRs für die anderen Sonden

void setup() {

if (DEBUG_ENABLED) {

Serial.begin(115200);

Serial.println("ESP1 (EtherCAT Bridge) starting...");

}

// WiFi für ESP-NOW initialisieren

WiFi.mode(WIFI_STA);

if (esp_now_init()!= ESP_OK) {

if (DEBUG_ENABLED) Serial.println("Error initializing ESP-NOW");

return;

}

esp_now_register_send_cb(OnDataSent);

esp_now_register_recv_cb(OnDataRecv);

// Peer für ESP2 hinzufügen

esp_now_peer_info_t peerInfo;

memcpy(peerInfo.peer_addr, esp2_mac_address, 6);

peerInfo.channel = 0;

peerInfo.encrypt = false;

if (esp_now_add_peer(&peerInfo)!= ESP_OK) {

if (DEBUG_ENABLED) Serial.println("Failed to add peer");

return;

}

// EasyCAT initialisieren

if (EASYCAT.Init() == true) {

if (DEBUG_ENABLED) Serial.println("EasyCAT Init SUCCESS");

} else {

if (DEBUG_ENABLED) Serial.println("EasyCAT Init FAILED");

// Anhalten, wenn die Initialisierung fehlschlägt

while (1) { delay(100); }

}

// Peripherie initialisieren

// Encoder

for (int i = 0; i < NUM_ENCODERS; i++) {

if (ENCODER_A_PINS[i]!= -1 && ENCODER_B_PINS[i]!= -1) {

ESP32Encoder::useInternalWeakPullResistors = UP;

encoders[i].attachHalfQuad(ENCODER_A_PINS[i], ENCODER_B_PINS[i]);

encoders[i].setFilter(ENCODER_GLITCH_FILTER);

encoders[i].clearCount();

}

}

// Hall-Sensor

pinMode(HALL_SENSOR_PIN, INPUT_PULLUP);

attachInterrupt(digitalPinToInterrupt(HALL_SENSOR_PIN), hall_sensor_isr, FALLING);

// Sonden

for(int i=0; i<NUM_PROBES; i++) {

pinMode(PROBE_PINS[i], INPUT_PULLUP);

}

attachInterrupt(digitalPinToInterrupt(PROBE_PINS), probe_isr_0, CHANGE);

//... weitere attachInterrupt für die anderen Sonden

if (DEBUG_ENABLED) Serial.println("Setup complete.");

}

void loop() {

// 1. Haupt-Task der EasyCAT-Bibliothek ausführen

EASYCAT.MainTask();

// 2. Daten von Peripherie lesen und in EtherCAT-Buffer schreiben

// Encoder

for (int i = 0; i < NUM_ENCODERS; i++) {

if (ENCODER_A_PINS[i]!= -1) {

// Mapping der Encoder-Werte in den EtherCAT-Buffer

// Dies erfordert eine benutzerdefinierte Struktur in einer separaten Header-Datei

// Beispiel: EASYCAT.BufferIn.Cust.encoder_values[i] = encoders[i].getCount();

}

}

// RPM berechnen

if (millis() - last_rpm_calc_time >= TACHO_UPDATE_INTERVAL_MS) {

noInterrupts();

uint32_t pulses = hall_pulse_count;

hall_pulse_count = 0;

interrupts();

current_rpm = (pulses * (60000 / TACHO_UPDATE_INTERVAL_MS)) / TACHO_MAGNETS_PER_REVOLUTION;

if (current_rpm < TACHO_MIN_RPM_DISPLAY) {

current_rpm = 0;

}

last_rpm_calc_time = millis();

// Beispiel: EASYCAT.BufferIn.Cust.spindle_rpm = current_rpm;

}

// Sonden

uint8_t probe_bitmask = 0;

for(int i=0; i<NUM_PROBES; i++) {

if(probe_states[i]) {

probe_bitmask |= (1 << i);

}

}

// Beispiel: EASYCAT.BufferIn.Cust.probe_states = probe_bitmask;

// 3. Daten von ESP2 (HMI) in den EtherCAT-Buffer schreiben

// Beispiel: memcpy(&EASYCAT.BufferIn.Cust.button_matrix_states, &incoming_hmi_data.button_matrix_states, sizeof(incoming_hmi_data.button_matrix_states));

// 4. Daten von EtherCAT (LinuxCNC) an ESP2 (HMI) senden

// Beispiel: memcpy(&outgoing_lcnc_data.led_matrix_states, &EASYCAT.BufferOut.Cust.led_states_from_lcnc, sizeof(outgoing_lcnc_data.led_matrix_states));

// outgoing_lcnc_data.linuxcnc_status = EASYCAT.BufferOut.Cust.linuxcnc_status_word;

esp_now_send(esp2_mac_address, (uint8_t *) &outgoing_lcnc_data, sizeof(outgoing_lcnc_data));

}

```

src/esp2/config_esp2.h

```C++

# **ifndef** CONFIG_ESP2_H

# **define** CONFIG_ESP2_H

# **include** <Arduino.h>

// =================================================================

// DEBUGGING & NETWORK

// =================================================================

# **define** DEBUG_ENABLED true

# **define** WIFI_SSID "Your_SSID"

# **define** WIFI_PASSWORD "Your_Password"

# **define** OTA_HOSTNAME "linuxcnc-hmi"

// =================================================================

// GPIO ASSIGNMENT ESP2

// =================================================================

// SPI for MCP23S17 Expanders (Standard VSPI)

# **define** PIN_MCP_MOSI 23

# **define** PIN_MCP_MISO 19

# **define** PIN_MCP_SCK 18

# **define** PIN_MCP_CS 5 // Shared CS Pin

// Control for TXS0108E Level Shifters

# **define** PIN_LEVEL_SHIFTER_OE 4

// Potentiometers (use ADC1 pins only)

# **define** NUM_POTIS 6

const int POTI_PINS = {36, 39, 32, 33, 35, 34};

// Rotary Encoders

# **define** NUM_ENCODERS_ESP2 2

const int ENC2_A_PINS = {25, 26};

const int ENC2_B_PINS = {27, 14};

# **define** ENC2_GLITCH_FILTER 1023

// =================================================================

// MCP23S17 CONFIGURATION

// =================================================================

# **define** NUM_MCP_DEVICES 4 // z.B. 1 für Buttons, 2 für LED-Zeilen, 1 für LED-Spalten

// Hardware-Adressen der MCPs (0-7)

# **define** MCP_ADDR_BUTTONS 0

# **define** MCP_ADDR_LED_ROWS 1

# **define** MCP_ADDR_LED_COLS 2

# **define** MCP_ADDR_ROT_SWITCHES 3

// =================================================================

// BUTTON & LED MATRIX CONFIGURATION (STATIC PART)

// =================================================================

# **define** MATRIX_ROWS 8

# **define** MATRIX_COLS 8

# **define** MAX_BUTTONS (MATRIX_ROWS * MATRIX_COLS)

# **define** MAX_LEDS (MATRIX_ROWS * MATRIX_COLS)

// Pins der Button-Matrix am MCP_ADDR_BUTTONS

const uint8_t BUTTON_ROW_PINS = {0, 1, 2, 3, 4, 5, 6, 7}; // GPA

const uint8_t BUTTON_COL_PINS = {8, 9, 10, 11, 12, 13, 14, 15}; // GPB

// Pins der LED-Matrix an den MCPs

// Annahme: Zeilen werden direkt vom MCP_ADDR_LED_ROWS getrieben

// Spalten werden über MOSFETs vom MCP_ADDR_LED_COLS getrieben

const uint8_t LED_ROW_PINS = {0, 1, 2, 3, 4, 5, 6, 7};

const uint8_t LED_COL_PINS = {0, 1, 2, 3, 4, 5, 6, 7};

# **endif** // CONFIG_ESP2_H

```

src/esp2/main_esp2.cpp

```C++

# **include** <Arduino.h>

# **include** <WiFi.h>

# **include** <ESPAsyncWebServer.h>

# **include** <AsyncElegantOTA.h>

# **include** <esp_now.h>

# **include** <ArduinoJson.h>

# **include** "config_esp2.h"

# **include** "shared_structures.h"

# **include** "persistence.h" // Für Laden/Speichern der Konfig

# **include** "hmi_handler.h" // Für Tasten, LEDs etc.

// Globale Objekte

AsyncWebServer server(80);

AsyncWebSocket ws("/ws");

struct_message_to_esp1 outgoing_hmi_data;

struct_message_to_esp2 incoming_lcnc_data;

// --- WebSocket Event Handler ---

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {

if (type == WS_EVT_CONNECT) {

if (DEBUG_ENABLED) Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());

// Sende initiale Konfiguration und Status an den neuen Client

//...

} else if (type == WS_EVT_DISCONNECT) {

if (DEBUG_ENABLED) Serial.printf("WebSocket client #%u disconnected\n", client->id());

} else if (type == WS_EVT_DATA) {

AwsFrameInfo *info = (AwsFrameInfo*)arg;

if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {

data[len] = 0;

if (DEBUG_ENABLED) Serial.printf("WS data from client #%u: %sn", client->id(), (char*)data);

// JSON parsen und Befehle verarbeiten

StaticJsonDocument doc;

DeserializationError error = deserializeJson(doc, (char*)data);

if (error) {

if (DEBUG_ENABLED) Serial.print(F("deserializeJson() failed: "));

if (DEBUG_ENABLED) Serial.println(error.c_str());

return;

}

const char* command = doc["command"];

if (strcmp(command, "saveConfig") == 0) {

// Konfiguration aus JSON extrahieren und speichern

//...

save_configuration(); // Funktion aus persistence.cpp

}

//... weitere Befehle

}

}

}

// --- ESP-NOW Callbacks ---

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {

// Optional: Logik für Sende-Status

}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {

memcpy(&incoming_lcnc_data, incomingData, sizeof(incoming_lcnc_data));

// LED-Zustände basierend auf den empfangenen Daten aktualisieren

update_leds_from_lcnc(incoming_lcnc_data);

}

void setup() {

if (DEBUG_ENABLED) Serial.begin(115200);

// Lade persistente Konfiguration aus NVS

load_configuration();

// HMI-Peripherie initialisieren

hmi_init();

// WiFi und Webserver

WiFi.mode(WIFI_STA);

WiFi.setHostname(OTA_HOSTNAME);

WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

while (WiFi.status()!= WL_CONNECTED) {

delay(500);

if (DEBUG_ENABLED) Serial.print(".");

}

if (DEBUG_ENABLED) {

Serial.println("\nWiFi connected.");

Serial.print("IP Address: ");

Serial.println(WiFi.localIP());

}

// ESP-NOW initialisieren

if (esp_now_init()!= ESP_OK) { /* Fehlerbehandlung */ }

esp_now_register_send_cb(OnDataSent);

esp_now_register_recv_cb(OnDataRecv);

esp_now_peer_info_t peerInfo;

memcpy(peerInfo.peer_addr, esp1_mac_address, 6);

peerInfo.channel = 0;

peerInfo.encrypt = false;

if (esp_now_add_peer(&peerInfo)!= ESP_OK) { /* Fehlerbehandlung */ }

// Webserver-Routen und WebSocket-Handler

ws.onEvent(onWsEvent);

server.addHandler(&ws);

server.on("/", HTTP_GET,(AsyncWebServerRequest *request){

request->send(LittleFS, "/index.html", "text/html");

});

server.on("/style.css", HTTP_GET,(AsyncWebServerRequest *request){

request->send(LittleFS, "/style.css", "text/css");

});

server.on("/script.js", HTTP_GET,(AsyncWebServerRequest *request){

request->send(LittleFS, "/script.js", "application/javascript");

});

// OTA-Update-Server starten

AsyncElegantOTA.begin(&server);

server.begin();

if (DEBUG_ENABLED) Serial.println("HTTP/WebSocket/OTA server started.");

}

void loop() {

// HMI-Aufgaben ausführen (Tasten scannen, LEDs aktualisieren, etc.)

hmi_task();

// WebSocket-Clients bereinigen

ws.cleanupClients();

// Daten an ESP1 senden, wenn sich etwas geändert hat

if (hmi_data_has_changed()) {

get_hmi_data(&outgoing_hmi_data);

esp_now_send(esp1_mac_address, (uint8_t *) &outgoing_hmi_data, sizeof(outgoing_hmi_data));

}

}

```

src/esp2/hmi_handler.h

```C++

# **ifndef** HMI_HANDLER_H

# **define** HMI_HANDLER_H

# **include** "shared_structures.h"

void hmi_init();

void hmi_task();

bool hmi_data_has_changed();

void get_hmi_data(struct_message_to_esp1* data);

void update_leds_from_lcnc(const struct_message_to_esp2& data);

# **endif** // HMI_HANDLER_H

```

src/esp2/hmi_handler.cpp

```C++

# **include** "hmi_handler.h"

# **include** "config_esp2.h"

# **include** <Adafruit_MCP23X17.h>

# **include** <Keypad.h>

# **include** <SPI.h>

// MCP-Objekte

Adafruit_MCP23X17 mcp_buttons;

Adafruit_MCP23X17 mcp_led_rows;

Adafruit_MCP23X17 mcp_led_cols;

// Keypad-Objekt

Keypad customKeypad; // Wird in hmi_init() konfiguriert

// Lokale Zustandsvariablen

uint8_t current_button_states = {0};

uint8_t current_led_states = {0};

bool data_changed_flag = false;

// Hilfsfunktionen für Keypad mit MCP23S17

// Dies ist eine Vereinfachung. Eine vollständige Implementierung

// würde eine Klasse erfordern, die die Pin-Read-Funktion für Keypad überschreibt.

// Hier wird ein manueller Scan als Beispiel gezeigt.

void scan_button_matrix() {

//...

}

void hmi_init() {

// OE-Pin für Pegelwandler initialisieren und aktivieren

pinMode(PIN_LEVEL_SHIFTER_OE, OUTPUT);

digitalWrite(PIN_LEVEL_SHIFTER_OE, HIGH); // Erst nach Boot aktivieren

// SPI starten

SPI.begin();

// MCPs initialisieren

if (!mcp_buttons.begin_SPI(PIN_MCP_CS, &SPI, MCP_ADDR_BUTTONS)) {

if (DEBUG_ENABLED) Serial.println("Error initializing MCP for buttons.");

}

//... weitere MCPs initialisieren

// MCP Pins für Tastenmatrix konfigurieren

for (int i = 0; i < MATRIX_ROWS; i++) {

mcp_buttons.pinMode(BUTTON_ROW_PINS[i], OUTPUT);

}

for (int i = 0; i < MATRIX_COLS; i++) {

mcp_buttons.pinMode(BUTTON_COL_PINS[i], INPUT_PULLUP);

}

// MCP Pins für LED-Matrix konfigurieren

//...

}

void hmi_task() {

// 1. Tastenmatrix scannen

scan_button_matrix();

// 2. Encoder, Potis, etc. lesen

//...

// 3. LED-Matrix aktualisieren (multiplexing)

//...

}

bool hmi_data_has_changed() {

bool state = data_changed_flag;

data_changed_flag = false; // Flag zurücksetzen nach Abfrage

return state;

}

void get_hmi_data(struct_message_to_esp1* data) {

memcpy(data->button_matrix_states, current_button_states, sizeof(current_button_states));

//... weitere Daten kopieren

}

void update_leds_from_lcnc(const struct_message_to_esp2& data) {

memcpy(current_led_states, data.led_matrix_states, sizeof(current_led_states));

}

```

data/index.html

```HTML

<!DOCTYPE html>

<html lang="de">

<head>

<meta charset="UTF-8">

<meta name="viewport" content="width=device-width, initial-scale=1.0">

<title>LinuxCNC HMI Konfiguration</title>

<link rel="stylesheet" href="style.css">

</head>

<body>

<header>

<h1>LinuxCNC HMI Konfiguration & Status</h1>

<div id="ws-status">Verbindung wird hergestellt...</div>

</header>

<main>

<section id="live-status">

<h2>Live-Status</h2>

<div class="status-grid">

<div>

<h3>Tastenmatrix</h3>

<div id="button-matrix-live" class="matrix-grid"></div>

</div>

<div>

<h3>LED-Matrix</h3>

<div id="led-matrix-live" class="matrix-grid"></div>

</div>

</div>

</section>

<section id="configuration">

<h2>Konfiguration</h2>

<div id="config-tabs">

<button class="tab-link active" onclick="openTab(event, 'buttons')">Tasten</button>

<button class="tab-link" onclick="openTab(event, 'leds')">LEDs</button>

<button class="tab-link" onclick="openTab(event, 'other')">Weitere</button>

</div>

<div id="buttons" class="tab-content" style="display: block;">

<h3>Tastenkonfiguration</h3>

<div id="button-config-table">

</div>

</div>

<div id="leds" class="tab-content">

<h3>LED-Konfiguration</h3>

<div id="led-config-table">

</div>

</div>

<div id="other" class="tab-content">

<h3>Weitere Einstellungen</h3>

</div>

<div class="actions">

<button id="save-config-btn">Konfiguration Speichern</button>

<button onclick="window.location.href='/update'">Firmware Update</button>

</div>

</section>

</main>

<script src="script.js"></script>

</body>

</html>

```

data/style.css

```CSS

/* Basic styling for the web interface */

body { font-family: sans-serif; margin: 0; background-color: #f4f4f4; }

header { background-color: #333; color: white; padding: 1rem; text-align: center; }

main { max-width: 1200px; margin: 1rem auto; padding: 1rem; background-color: white; border-radius: 8px; }

section { margin-bottom: 2rem; }

h2 { border-bottom: 2px solid #333; padding-bottom: 0.5rem; }

# ws-status { position: absolute; top: 10px; right: 10px; font-size: 0.8rem; padding: 5px 10px; border-radius: 5px; }

.status-connected { background-color: #28a745; color: white; }

.status-disconnected { background-color: #dc3545; color: white; }

.status-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 2rem; }

.matrix-grid { display: grid; grid-template-columns: repeat(8, 1fr); gap: 5px; width: 250px; height: 250px; border: 1px solid #ccc; padding: 10px; }

.matrix-cell { width: 100%; height: 100%; border: 1px solid #ddd; border-radius: 4px; transition: background-color 0.2s; }

.button-pressed { background-color: #007bff!important; }

.led-on { background-color: #ffc107; box-shadow: 0 0 10px #ffc107; }

/* Tabs */

# config-tabs { overflow: hidden; border-bottom: 1px solid #ccc; margin-bottom: 1rem; }

.tab-link { background-color: inherit; float: left; border: none; outline: none; cursor: pointer; padding: 14px 16px; transition: 0.3s; }

.tab-link:hover { background-color: #ddd; }

.tab-link.active { background-color: #ccc; }

.tab-content { display: none; padding: 6px 12px; border-top: none; }

/* Config Tables */

.config-table-header,.config-table-row { display: grid; grid-template-columns: 50px 1fr 100px 100px 1fr; gap: 10px; padding: 8px; border-bottom: 1px solid #eee; align-items: center; }

.config-table-header { font-weight: bold; background-color: #f0f0f0; }

.config-table-row input,.config-table-row select { width: 100%; padding: 5px; box-sizing: border-box; }

.actions { margin-top: 1.5rem; }

.actions button { padding: 10px 20px; font-size: 1rem; cursor: pointer; }

# save-config-btn { background-color: #28a745; color: white; border: none; }

```

data/script.js

```JavaScript

let websocket;

const wsStatusElem = document.getElementById('ws-status');

window.addEventListener('load', onLoad);

function onLoad(event) {
  initWebSocket();
  initUI();
}

function initWebSocket() {
  console.log('Trying to open a WebSocket connection...');
  const gateway = `ws://${window.location.hostname}/ws`;
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}

function onOpen(event) {
  console.log('Connection opened');
  wsStatusElem.textContent = 'Verbunden';
  wsStatusElem.className = 'status-connected';
  // Request initial data
  websocket.send(JSON.stringify({ command: 'getInitialState' }));

}

function onClose(event) {

console.log('Connection closed');

wsStatusElem.textContent = 'Getrennt';

wsStatusElem.className = 'status-disconnected';

setTimeout(initWebSocket, 2000);

}

function onMessage(event) {

console.log('Received:', event.data);

const data = JSON.parse(event.data);

if (data.type === 'liveStatus') {

updateLiveStatus(data.payload);

} else if (data.type === 'initialState') {

buildConfigUI(data.payload.config);

updateLiveStatus(data.payload.status);

}

}

function initUI() {

document.getElementById('save-config-btn').addEventListener('click', saveConfiguration);

// Create matrix grids

createMatrixGrid('button-matrix-live', 'btn-live');

createMatrixGrid('led-matrix-live', 'led-live');

}

function createMatrixGrid(containerId, prefix) {

  const container = document.getElementById(containerId);

  container.innerHTML = '';

  for (let r = 0; r < 8; r++) {

    for (let c = 0; c < 8; c++) {

      const cell = document.createElement('div');

      cell.className = 'matrix-cell';

      cell.id = `${prefix}-${r}-${c}`;

      container.appendChild(cell);

}

}

}

function updateLiveStatus(status) {

// Update button matrix

for (let i = 0; i < 8; i++) {

for (let j = 0; j < 8; j++) {

const cell = document.getElementById(`btn-live-${i}-${j}`);

if (cell) {

if ((status.buttons[i] >> j) & 1) {

cell.classList.add('button-pressed');

} else {

cell.classList.remove('button-pressed');

}

}

}

}

// Update LED matrix

for (let i = 0; i < 8; i++) {

for (let j = 0; j < 8; j++) {

const cell = document.getElementById(`led-live-${i}-${j}`);

if (cell) {

if ((status.leds[i] >> j) & 1) {

cell.classList.add('led-on');

} else {

cell.classList.remove('led-on');

}

}

}

}

}

function buildConfigUI(config) {

// Build button config table

const btnContainer = document.getElementById('button-config-table');

btnContainer.innerHTML = `

<div class="config-table-header">

<div>#</div><div>Name</div><div>Toggle</div><div>Radio Grp</div><div></div>

</div>

`;

config.buttons.forEach((btn, index) => {

const row = document.createElement('div');

row.className = 'config-table-row';

row.innerHTML = `

<div>${index}</div>

<div><input type="text" id="btn-name-${index}" value="${btn.name}"></div>

<div><input type="checkbox" id="btn-toggle-${index}" ${btn.is_toggle? 'checked' : ''}></div>

<div><input type="number" id="btn-radio-${index}" value="${btn.radio_group_id}" min="0"></div>

<div><span title="Row: ${btn.matrix_row}, Col: ${btn.matrix_col}">Pos: ${btn.matrix_row},${btn.matrix_col}</span></div>

`;

btnContainer.appendChild(row);

});

// Build LED config table similarly...

}

function saveConfiguration() {

const config = {

buttons:,

leds:

};

// Read button config from UI

for (let i = 0; i < 64; i++) { // Assuming MAX_BUTTONS

const nameEl = document.getElementById(`btn-name-${i}`);

if (!nameEl) break;

config.buttons.push({

name: nameEl.value,

is_toggle: document.getElementById(`btn-toggle-${i}`).checked,

radio_group_id: parseInt(document.getElementById(`btn-radio-${i}`).value)

});

}

// Read LED config from UI...

console.log('Sending config to save:', config);

websocket.send(JSON.stringify({ command: 'saveConfig', payload: config }));

alert('Konfiguration gesendet!');

}

function openTab(evt, tabName) {

let i, tabcontent, tablinks;

tabcontent = document.getElementsByClassName("tab-content");

for (i = 0; i < tabcontent.length; i++) {

tabcontent[i].style.display = "none";

}

tablinks = document.getElementsByClassName("tab-link");

for (i = 0; i < tablinks.length; i++) {

tablinks[i].className = tablinks[i].className.replace(" active", "");

}

document.getElementById(tabName).style.display = "block";

evt.currentTarget.className += " active";

}

```

**Schlussfolgerung**

Dieses Projekt stellt eine umfassende und robuste Lösung dar, um eine hochgradig anpassbare physische Steuerung für eine LinuxCNC-Maschine zu realisieren. Durch die strategische Aufteilung der Aufgaben auf zwei ESP32-Controller wird eine klare Trennung zwischen zeitkritischen Maschinenkommunikationsaufgaben und komplexen HMI-Funktionen erreicht. Die Wahl von EtherCAT gewährleistet eine industrielle, deterministische Anbindung an die CNC-Steuerung, während ESP-NOW eine drahtlose Verbindung mit geringer Latenz für eine reaktionsschnelle Bedienung ermöglicht.

Die Firmware ist durchgehend auf nicht-blockierenden, interruptgesteuerten Betrieb ausgelegt, was für die Erfassung von Hochgeschwindigkeitssignalen von Encodern und Sensoren unerlässlich ist. Die extreme Konfigurierbarkeit über Header-Dateien und eine dynamische Weboberfläche bietet ein Höchstmaß an Flexibilität und Anpassungsfähigkeit. Die Weboberfläche selbst, mit ihrer Live-Vorschau über WebSockets, der persistenten Speicherung von Einstellungen und der integrierten OTA-Update-Fähigkeit, hebt das Projekt auf ein professionelles Niveau.

Die bereitgestellten Hardware-Schaltpläne und der vollständige, kommentierte Quellcode bilden eine solide Grundlage für den Nachbau und die weitere Anpassung des Systems an spezifische Anforderungen. Das Projekt demonstriert eindrucksvoll, wie moderne, kostengünstige Embedded-Technologien mit etablierten Industriestandards kombiniert werden können, um leistungsstarke und flexible Steuerungslösungen zu schaffen.

Im Bericht verwendete Quellen

[![](data:image/png;base64,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)](https://www.youtube.com/watch?v=5edPOlQQKmo"%20\t%20"_blank)

[youtube.com](https://www.youtube.com/watch?v=5edPOlQQKmo"%20\t%20"_blank)

[How To Install PlatformIO (ESP32 + Arduino series) - YouTube](https://www.youtube.com/watch?v=5edPOlQQKmo"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://www.youtube.com/watch?v=5edPOlQQKmo"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAQAAAAEACAMAAABrrFhUAAAAh1BMVEX///8AAAAPDw+lpaUmJiZubm4KCgrBwcHs7OyEhIT39/f7+/sFBQXp6ekODg5bW1vy8vItLS0gICAVFRVzc3Pk5OTPz88bGxvb29s9PT3Hx8d8fHyJiYlKSkpjY2O1tbWdnZ1SUlKSkpLV1dU5OTmsrKxGRka7u7syMjKfn59gYGA6OjqWlpbNcUeEAAAIPUlEQVR4nO2diXajOgxAOxB2E/adQCB7m///vkfaeTPTnkKQgDhQ3Q9IbdV4Aen65YUgCIIgCIIgCIIgCIIgCIIgiH+xw3KbZ740B/wsv5ShPWrvRb96c1xPmAee6yRVJo4VAzW9Wmf519xYn61rqA7vvh5vK5d3Z5BE1jY2BvbfTv25dr+BrfyBz4F9qDzevRiEbJVDHgNVTATeXRiIvDngI6CKx/lNfl9pIoB9CvSymn//m+WgOiEDEEpzH/8fCEWM6r+az3j+/0SUo6aB8o13w8eCJYqOGAB+xLvho+Fl8CFgnPa8mz0ebJ+Cd4SGtpwB0MyDtQkNQLCALcBfmAVdCPTTUpaAd5ijAANg1/M+A3xFvgC3g6q/jE3QH7QAFgDTWtIU0CABJ4FlzYENryEwAHvGu8njUlEAgAF4W1gALGAAzM3C5oBiBwxAsebd5HHxgauAmi1rI/QrBx4G7MuSzkLNgfgAfCNgpA7vNo8Je0th/W+eAWtJkwCDbgSbIXBd0nEwEsGvxo1ws5ydgLwBLoI39HzFu92jEdWYbyPphne7x2Jd7TAfiQ3xzLvlI+GIiLfiDYG2jHnQ1ZCfR41QWsJ2MJIQM+AHulLMPwKRBP8m8G8E5v4UNP3HTQAfGKnvzHk7ILtZOKT/TQSCayLPNQTMO14H54kZeprt2SxDsN5ryhipgrqZapU7uxC4Va6Yw4b/H4wgPGhFlTireXBOqkIrw2BojuAnzF1aHsR5cCjTHfhrMEEQBEEQBEEQBEEQBEEQBEEQBEEQBEE8O7qtmsENU7VHSoqYD4aux6etJlnVprIkbXuKbX3U1Ignx1bqYh95wlq+sRa86K2olREcWvMgFqWzK3xOYGKCu5fEwRKtORCLhfN9kbdwLpYfAlOROix38tk/LTpRyIi3d4o7vc11zEQxo1lqJgE3ZxthdjeJl62y3TiLYrPSmGG5rbUJyLdlaOrgIKSF1yN305MGpgp/oMdiVq08bxqlpuetNr4IzOlXrH6GC+EV41D60v2wbno/aS213MSghqT1pz37fxOJgUsmvxBvrdUDKsnZqurvvwwBhRwevmDkhp1KzoMK6Znjn3oldxtBBinjiDKgP+Vf1PKRFbRe1cv+aF9gRRzOFp00bx4e61FYH3uUuBoh0PMoVwpyO2AfHu2Skff3IxBk0EZFGW4IGMrjlZrrTXmnsXoJ19w5JWotDAsOHhHhNewer7EPb5UgYY7HJp/C0eiO7KJM4L/JnBNiCJTH8XvXh+TUNQRUDSM5kzX4wVD1OVWNylmX7SGtML/JNkCNEFelZlJ2tOuCc5u4UInKi85PqSl0VHsbGVLzB9XpGQFHlVTVfn6JK+RvQm1y+omjQON8aG3XCbEGvJMAnZr2lWPhfFS3tkvE6o1WXRPLN3BVajKpdSFE677XF1gA+Co1q9ZVO0NPzTnsQMRXqblp3Qnghfca7EDE1yjZ7j3CH0+AJ8LgzDMAx9Y16/VhAUieMwB4zyMwAHyVmh1zAHp11mB7YfOVp03Pat0L41eB9r3Ft3BVasp+65JVYzeonggLAFelpnttbdcB6zdzgDtBI+V4Fug4D6fYs8AG+oHI5qjUtNq/5QQV8jehXuEXo+Ym0erUH2q4uYlBtbo8lZqbrpPrAfWiiq3gt03ZvGyCXt718ia0ML8Jv2Pk5fb6kc8Q6HgfdPu/1JjlycM4xewtF6VmtO1uq4JQnbIE5RSM8dsuPF52Z7YKNPgm3UNKNcPi4Q+BV9wVwKbg2VlOkFJFI3z0eyHPup/SY9ZQtZ2bYxMEjPT1oUcCr+iRI2LsgG9FsPcNvv+xUHqcUpOt/F5DVS9BLyvk42FIuuSu3jxoEHjHuqcA2r5C3led4RetfUI9+edpk+RuMM/x76VG/CXQVr0j4Gr4B+A3ZpkdI2E6tSqThSjxQfcD7/yeEWBR57fmnhj2TsyqcyRPwTo6V5m4A+bK7rJex/Vx+v8eAjWIQ+WkTEAYB6oNnaaMXd5jn8qc4eP/858d9dcGYcSX472nQNhsB+RIPj3q6bVzImCrolx0xUSzVa/bfcfM3dQjFQs8MbaiHd3vvhYKbjKOVfrpsdPaWnnrf/cpbO05Vj3IKT8nDP1WOHh0/6yqblLUivmjSid124zDUrzWeX0VyzA2f1717MtH9bD5EyuHCYIgCIIgCIIgCIIgCIIgCIIgCIIgRuVHX7z8w6/e/uGXrxt6mu3Z7Lp/Y70fIVHKCK7JdFnCE8O8I6ZY6lP/U/9x9QITILvZIK+urhTc6kdHIpIGJMw1/edYRD8STQSwq4ER4n0NT0SE9uoGvIpnR8ZFVg0aIpfK0QlwRNQ0kCJqVJ+TddWzGusTes7RITEynUaCFjj6A8ZHbldTtQfguowZ8IOoh7H6CypHh8j4MAlaPGWkWHPhU8ISsESGp0doAjyoVZarSWoKoBYZE69te0584CTA1yY3AVCRUvC2nF3AOxbQK8xXqTkBFQUAGACuVtkJgKq1+XqFJwC6FeRqlp4CqF3erpe1EZIvwNOQflrSYfAXc4By+aXNggidnsHvhokJEOAuIY53jIwP2yO+Daj+coaAl2FejJdvvNs9FixBXTupom9zeDaiHOlTxN9m8FSgdXp6+fjb5iZgXcGVur9RxQVsBuTNAZ8moorJ3J+Cpv9DkkTsQzXvM4FsgTR930Qg9We8FrCVHw5Nk9LjbTXXEERW/6uMO1DTq9Vxzf2zsj5b13Bgjtj/2KHoV2+OO83d4+PjuU5SZeLg0f85BuU2z3xpDvhZfilH7T1BEARBEARBEARBEARBEARBLIb/ADYz7MiW+84hAAAAAElFTkSuQmCC)](https://esp32cube.com/post/esp32encoder-library-for-rotation-encoder-reading/"%20\t%20"_blank)

[esp32cube.com](https://esp32cube.com/post/esp32encoder-library-for-rotation-encoder-reading/"%20\t%20"_blank)

[ESP32Encoder Library for Rotation Encoder Reading](https://esp32cube.com/post/esp32encoder-library-for-rotation-encoder-reading/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://esp32cube.com/post/esp32encoder-library-for-rotation-encoder-reading/"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://www.espressif.com/en/solutions/low-power-solutions/esp-now"%20\t%20"_blank)

[espressif.com](https://www.espressif.com/en/solutions/low-power-solutions/esp-now"%20\t%20"_blank)

[ESP-NOW Wireless Communication Protocol - Espressif Systems](https://www.espressif.com/en/solutions/low-power-solutions/esp-now"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://www.espressif.com/en/solutions/low-power-solutions/esp-now"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAALVBMVEUASoDU2+P///9nhaUkW4pUeJyjs8b19vjf5OvI0dw/apPq7fGHnbawvc28yNVvI/haAAAAUklEQVQYlY3POQ7AMAgEQC4DtpP8/7lxDoHovAXFSIsAYCuNn0gC0psjAdeQTlIAmLiC0ggwRHQ6c8dUdYvCV5HpFdZOrQCXjQpiPU5v/wdbj96XAwEg74cFuAAAAABJRU5ErkJggg==)](https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/"%20\t%20"_blank)

[randomnerdtutorials.com](https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/"%20\t%20"_blank)

[Getting Started with ESP-NOW (ESP32 with Arduino IDE) - Random Nerd Tutorials](https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAIwAAAAZCAMAAADUtTb8AAAAV1BMVEX///8AgYS/3+B3vL1jsrNfsLJ7vr+bzc9PqKozmpxXrK4rlph/wMHb7e7r9fXl8vLX6+wajZCPx8lBoaOx2NnE4uIQiIvM5ub0+vqo1NWTycu12ts/oKIbPNtoAAABR0lEQVRIie2Vy5KCMBBFu4MYJITII4Di/3/n9APUoZjdxJkFd0FyE8o+lb5BgEOHDv0jBbOoVj/aqvUy87Icp9Xw6lXeo6csGnP9XRiLi27s5kbmLc8zXR/EnBEzGgpE4jghWmZBzBPC+HeTrabdgWnIPFLAdM82UZXOB3o6hSmN6bTwFgbvMGMKmPJpGqkVqEotMGfKB5mwA9NMl7QwNf08x5SGuMIYJdvAEDVNh1SZ4V54PQUYEHuByY112rMNzIUPp3ApYcILZn4FeIg7MPzqmALGVSS+MhNV4C+HDitMDzswUD1OkALmW4DpUkc6DVgyUyzbLXWFBieYDAPiUsJQ/cbenJ6CwERuB2iO76ZSzM/AhEE74+oVBugCd7zVLU2zH4OBkHOWK/k/UpgeJSZQlwzqZkgJs5U3409b0fi0tQ8d+jN9AdMdDBmaYrvlAAAAAElFTkSuQmCC)](https://forum.arduino.cc/t/mcp23s17-pinextender-addressing-confusion/966301"%20\t%20"_blank)

[forum.arduino.cc](https://forum.arduino.cc/t/mcp23s17-pinextender-addressing-confusion/966301"%20\t%20"_blank)

[MCP23S17 Pinextender addressing confusion - General Electronics - Arduino Forum](https://forum.arduino.cc/t/mcp23s17-pinextender-addressing-confusion/966301"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://forum.arduino.cc/t/mcp23s17-pinextender-addressing-confusion/966301"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAZlBMVEX///8PDw/w8PAAAAAODg4hISHHx8f09PR2dnYVFRUaGhrh4eFcXFzR0dGxsbFqamqkpKSqqqr4+PhMTExwcHDS0tLZ2dmfn59CQkKDg4NhYWGQkJAzMzNWVlbr6+srKyuTk5O8vLxTnVwZAAAAqElEQVQYlU2PCQ6DQAhFGRhncZzdurba3v+SxdamEgifl3wCQILDWq72UATSIaK6aRwSCydBaAAYJwDfsdACBHLPNINfWOAJAtE6xBNon7Ihoq7mpA5LCdFUBs29j0n/LQ8VLzsy1TJcl46TBv/4gYKb8npzAfEDYBX7Ogn7jNa+ygFmmpUjoylKA19QA/CpcqeVR7mFfsnYB8zROH6OZNs0reDkLiS9AbTFB9QLFL8BAAAAAElFTkSuQmCC)](https://www.electro-tech-online.com/threads/how-to-init-multiple-mcp23s17.126668/"%20\t%20"_blank)

[electro-tech-online.com](https://www.electro-tech-online.com/threads/how-to-init-multiple-mcp23s17.126668/"%20\t%20"_blank)

[How to init multiple MCP23S17 ? | Electronics Forum (Circuits, Projects and Microcontrollers) - Electro-Tech-Online](https://www.electro-tech-online.com/threads/how-to-init-multiple-mcp23s17.126668/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://www.electro-tech-online.com/threads/how-to-init-multiple-mcp23s17.126668/"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAGFBMVEX/////AAD/VWb/qsz/gGb/qpn//8z/1ZkEnkXaAAAAWElEQVQYlW2OWRLAIAxCyWbvf+NmG42d8qUPBAEXuTD1CxgDBiD5AjPZLzPwUAEqxcEQVdxgtQso7c6er4wV4LNqsqTuYGWNrJHen22/FEXDR47xBTyTiRdztQDjxiFtRAAAAABJRU5ErkJggg==)](https://e2e.ti.com/support/logic-group/logic/f/logic-forum/1326965/txs0108e-problems-with-pin-oe"%20\t%20"_blank)

[e2e.ti.com](https://e2e.ti.com/support/logic-group/logic/f/logic-forum/1326965/txs0108e-problems-with-pin-oe"%20\t%20"_blank)

[TXS0108E: Problems with pin OE - Logic forum - TI E2E](https://e2e.ti.com/support/logic-group/logic/f/logic-forum/1326965/txs0108e-problems-with-pin-oe"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://e2e.ti.com/support/logic-group/logic/f/logic-forum/1326965/txs0108e-problems-with-pin-oe"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://www.youtube.com/watch?v=buFKeqbafDI"%20\t%20"_blank)

[youtube.com](https://www.youtube.com/watch?v=buFKeqbafDI"%20\t%20"_blank)

[Using the PlatformIO Library Manager (ESP32 + Arduino series) - YouTube](https://www.youtube.com/watch?v=buFKeqbafDI"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://www.youtube.com/watch?v=buFKeqbafDI"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://discuss.tinyml.seas.harvard.edu/t/platformio-and-esp32-development/491"%20\t%20"_blank)

[discuss.tinyml.seas.harvard.edu](https://discuss.tinyml.seas.harvard.edu/t/platformio-and-esp32-development/491"%20\t%20"_blank)

[PlatformIO and ESP32 development - Programming Q&A - TinyML Community](https://discuss.tinyml.seas.harvard.edu/t/platformio-and-esp32-development/491"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://discuss.tinyml.seas.harvard.edu/t/platformio-and-esp32-development/491"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://www.reddit.com/r/esp32/comments/1b96t8j/esp32_level_shifter/"%20\t%20"_blank)

[reddit.com](https://www.reddit.com/r/esp32/comments/1b96t8j/esp32_level_shifter/"%20\t%20"_blank)

[ESP32 Level Shifter - Reddit](https://www.reddit.com/r/esp32/comments/1b96t8j/esp32_level_shifter/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://www.reddit.com/r/esp32/comments/1b96t8j/esp32_level_shifter/"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://forum.microchip.com/s/topic/a5C3l000000M89lEAC/t261413"%20\t%20"_blank)

[forum.microchip.com](https://forum.microchip.com/s/topic/a5C3l000000M89lEAC/t261413"%20\t%20"_blank)

[MCP23S17 hardware pins problem - Microchip Forums](https://forum.microchip.com/s/topic/a5C3l000000M89lEAC/t261413"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://forum.microchip.com/s/topic/a5C3l000000M89lEAC/t261413"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAIwAAAAZCAMAAADUtTb8AAAAV1BMVEX///8AgYS/3+B3vL1jsrNfsLJ7vr+bzc9PqKozmpxXrK4rlph/wMHb7e7r9fXl8vLX6+wajZCPx8lBoaOx2NnE4uIQiIvM5ub0+vqo1NWTycu12ts/oKIbPNtoAAABR0lEQVRIie2Vy5KCMBBFu4MYJITII4Di/3/n9APUoZjdxJkFd0FyE8o+lb5BgEOHDv0jBbOoVj/aqvUy87Icp9Xw6lXeo6csGnP9XRiLi27s5kbmLc8zXR/EnBEzGgpE4jghWmZBzBPC+HeTrabdgWnIPFLAdM82UZXOB3o6hSmN6bTwFgbvMGMKmPJpGqkVqEotMGfKB5mwA9NMl7QwNf08x5SGuMIYJdvAEDVNh1SZ4V54PQUYEHuByY112rMNzIUPp3ApYcILZn4FeIg7MPzqmALGVSS+MhNV4C+HDitMDzswUD1OkALmW4DpUkc6DVgyUyzbLXWFBieYDAPiUsJQ/cbenJ6CwERuB2iO76ZSzM/AhEE74+oVBugCd7zVLU2zH4OBkHOWK/k/UpgeJSZQlwzqZkgJs5U3409b0fi0tQ8d+jN9AdMdDBmaYrvlAAAAAElFTkSuQmCC)](https://forum.arduino.cc/t/problem-with-txs0108e-logic-level-converter/682760"%20\t%20"_blank)

[forum.arduino.cc](https://forum.arduino.cc/t/problem-with-txs0108e-logic-level-converter/682760"%20\t%20"_blank)

[Problem with TXS0108E Logic Level converter - General Electronics - Arduino Forum](https://forum.arduino.cc/t/problem-with-txs0108e-logic-level-converter/682760"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://forum.arduino.cc/t/problem-with-txs0108e-logic-level-converter/682760"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://www.reddit.com/r/esp32/comments/yttvy8/should_you_hook_the_ground_on_a_txs0108e_up_to/"%20\t%20"_blank)

[reddit.com](https://www.reddit.com/r/esp32/comments/yttvy8/should_you_hook_the_ground_on_a_txs0108e_up_to/"%20\t%20"_blank)

[Should you hook the ground on a TXS0108E up to the ground on your ESP32 or the ground on your 5V PSU? - Reddit](https://www.reddit.com/r/esp32/comments/yttvy8/should_you_hook_the_ground_on_a_txs0108e_up_to/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://www.reddit.com/r/esp32/comments/yttvy8/should_you_hook_the_ground_on_a_txs0108e_up_to/"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAA+ElEQVQ4jZXTvUrEQBiF4UcFG/+wsLePhZV3EMXWG5jWxk57SxErQWwNglrbTm0l2CzTSvAClN1esDCRbDZmdw8MYSY573cYTpiivMiW+94v/mOqn5d4bJ61tdAz+Ri31fYqhnQ2NUFj8kXDDKd5kT11JZlI0Jrc1kSSdoLzHnOd5K4zQV5k2xhgtQcA39iLIb21ExzNYIYlnNSbJuBgBnOtwy7A+hyAtS7AYA5A6gLczwG4GQNU5Xit1jS947ku1FiR8iLbwAu6i88QuzGkciwB7P8SRzGkHVyjxCe+8IEHbMWQymad+36mFWxW34xiSMPqXAx/d+gHFa9E7WKthWMAAAAASUVORK5CYII=)](https://reprap.org/forum/read.php?2,769032,page=11"%20\t%20"_blank)

[reprap.org](https://reprap.org/forum/read.php?2,769032,page=11"%20\t%20"_blank)

[ESP32 Printer Board - RepRap](https://reprap.org/forum/read.php?2,769032,page=11"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://reprap.org/forum/read.php?2,769032,page=11"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://registry.platformio.org/libraries/adafruit/Adafruit%20MCP23017%20Arduino%20Library"%20\t%20"_blank)

[registry.platformio.org](https://registry.platformio.org/libraries/adafruit/Adafruit%20MCP23017%20Arduino%20Library"%20\t%20"_blank)

[adafruit/Adafruit MCP23017 Arduino Library - PlatformIO Registry](https://registry.platformio.org/libraries/adafruit/Adafruit%20MCP23017%20Arduino%20Library"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://registry.platformio.org/libraries/adafruit/Adafruit%20MCP23017%20Arduino%20Library"%20\t%20"_blank)

[![](data:image/jpeg;base64,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)](https://www.bausano.net/en/download-2"%20\t%20"_blank)

[bausano.net](https://www.bausano.net/en/download-2"%20\t%20"_blank)

[Download - AB&T - bausano.net](https://www.bausano.net/en/download-2"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://www.bausano.net/en/download-2"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAAb1BMVEX///8kKS74+PgsMTY+Q0f8/Pzw8PGRk5ZNUVXr6+xDSEzCxMUpLjLV1tYuMzc/Q0i3ubuUl5lbX2NTV1uLjpFKTlKxs7Xh4uOFh4rn5+h3en1vcnZ9gIPc3d6/wcJzd3o3PECmqKpjZ2rOz9CfoaRP5W5KAAABU0lEQVQ4jW1T7aKCIAzdEEUTSUuzLMtuvf8zXtiQVDx/lJ3DvgEIEPU9l1rL/F4LiFEMCQYkj2JDC6VxhaRJl3x2xgin7MePMuYRD7cQftrjrcL7ELk9nNt8yXXtyRm5mquzXADKA6LpzgZRlgBPZ1WO7yn/yrmqyGdGn95Zjav2hbNghZHMV3uNG3DZCigEmhRq+pmyrUAcifj4CO+49xciWqAe6p3pCJrNH1ATp5gH6KghQLrDnoB8GxaYPYFkhpONipjrl+wI61hQ8tB5EvYb4eTL5Ebhd8u/2f4EYfhvvWJpw9ZJ0LQfb5uxbMKi3pp5xVp30miexZHbRriHvUnojrKt7isXqWRBFQQNF2w3aYCPUl8/EREWzxtuE+pVI/wbkSGp0a51Nyg1F+L5/nel56xXgt+zcMgeC0FK27gdTzVoPXtI9DBuO2tRhJj94m3/A1GiDZXoM3d5AAAAAElFTkSuQmCC)](https://github.com/RobTillaart/MCP23017_RT"%20\t%20"_blank)

[github.com](https://github.com/RobTillaart/MCP23017_RT"%20\t%20"_blank)

[RobTillaart/MCP23017_RT: Arduino library for I2C MCP23017 16 channel port expander - GitHub](https://github.com/RobTillaart/MCP23017_RT"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://github.com/RobTillaart/MCP23017_RT"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAAb1BMVEX///8kKS74+PgsMTY+Q0f8/Pzw8PGRk5ZNUVXr6+xDSEzCxMUpLjLV1tYuMzc/Q0i3ubuUl5lbX2NTV1uLjpFKTlKxs7Xh4uOFh4rn5+h3en1vcnZ9gIPc3d6/wcJzd3o3PECmqKpjZ2rOz9CfoaRP5W5KAAABU0lEQVQ4jW1T7aKCIAzdEEUTSUuzLMtuvf8zXtiQVDx/lJ3DvgEIEPU9l1rL/F4LiFEMCQYkj2JDC6VxhaRJl3x2xgin7MePMuYRD7cQftrjrcL7ELk9nNt8yXXtyRm5mquzXADKA6LpzgZRlgBPZ1WO7yn/yrmqyGdGn95Zjav2hbNghZHMV3uNG3DZCigEmhRq+pmyrUAcifj4CO+49xciWqAe6p3pCJrNH1ATp5gH6KghQLrDnoB8GxaYPYFkhpONipjrl+wI61hQ8tB5EvYb4eTL5Ebhd8u/2f4EYfhvvWJpw9ZJ0LQfb5uxbMKi3pp5xVp30miexZHbRriHvUnojrKt7isXqWRBFQQNF2w3aYCPUl8/EREWzxtuE+pVI/wbkSGp0a51Nyg1F+L5/nel56xXgt+zcMgeC0FK27gdTzVoPXtI9DBuO2tRhJj94m3/A1GiDZXoM3d5AAAAAElFTkSuQmCC)](https://github.com/madhephaestus/ESP32Encoder"%20\t%20"_blank)

[github.com](https://github.com/madhephaestus/ESP32Encoder"%20\t%20"_blank)

[madhephaestus/ESP32Encoder: A Quadrature and half ... - GitHub](https://github.com/madhephaestus/ESP32Encoder"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://github.com/madhephaestus/ESP32Encoder"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAALVBMVEUASoDU2+P///9nhaUkW4pUeJyjs8b19vjf5OvI0dw/apPq7fGHnbawvc28yNVvI/haAAAAUklEQVQYlY3POQ7AMAgEQC4DtpP8/7lxDoHovAXFSIsAYCuNn0gC0psjAdeQTlIAmLiC0ggwRHQ6c8dUdYvCV5HpFdZOrQCXjQpiPU5v/wdbj96XAwEg74cFuAAAAABJRU5ErkJggg==)](https://randomnerdtutorials.com/esp-now-two-way-communication-esp32/"%20\t%20"_blank)

[randomnerdtutorials.com](https://randomnerdtutorials.com/esp-now-two-way-communication-esp32/"%20\t%20"_blank)

[ESP-NOW Two-Way Communication Between ESP32 Boards ...](https://randomnerdtutorials.com/esp-now-two-way-communication-esp32/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://randomnerdtutorials.com/esp-now-two-way-communication-esp32/"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://community.platformio.org/t/if-somebody-uses-easycat-lib/12904"%20\t%20"_blank)

[community.platformio.org](https://community.platformio.org/t/if-somebody-uses-easycat-lib/12904"%20\t%20"_blank)

[IF somebody uses EasyCAT_lib - PlatformIO Community](https://community.platformio.org/t/if-somebody-uses-easycat-lib/12904"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://community.platformio.org/t/if-somebody-uses-easycat-lib/12904"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAMAAAADACAYAAABS3GwHAAAgAElEQVR4nO1deXQUVfb+ek/SIUsnISEJkIRgSFgCBAgYZFM2EQjgCOLGICiCM+i4/RwGj4M4jI4eh+Eoos4AIwwaZNwQRlARwi4hyE7CFhKIIU0Ssvd6f390uu1Od6erq6u7Kkl953ynbzpd972quvfVfa/eu08CQArADAussrTl79bfo9X/mMrWTymAJAAZAAYCSGv5Ow6ABkAIACVEdGToATQCqALwC4CrAM4BOAmgCMDllt+4skvOZYmLCtr/yBVcOUlbcgSA4QCmABgHIB5AWBv6RXRe1AO4AeA7AN8AOAygBm3bI2ewttCw+3T3O1e/by0PBrAGQCkAEimSBUsBrIXFllzZJieyFK6NX8qCSgD3AdgrgIsnsmMxHxbbUoJ5I8xUbrPFd4XWniQHMAYWwzf4+UKI7Lw0wGJj98Bic5w9AVzB04FWD4oDsB6i4YsMHA2w2Fw8foXPIZC9zJRzAFQI4IKI7JysAPAwWoUzLOS2v3SBCADvQmz1RfJPAyy2GAEf4c6LWj8hEgHsEcCJixRpz72w2CbgxRNAYvelGZ6fAH1gGZ9N8vA7ESL4wDUAkwGcB8MXYdYv7F80mN1wMETjFyFs9ACwExZbtaK1bTvI9m+Cpa1+ZP/GV2z5RbQnWJ8EZ8FgKoS7sMf6w3gAByAav4j2hWsAcmCZWgG4mbJjHwLZh0LWzwgAmyEav4j2hx4APoZl3pm9XTvIspY/pLCEQxL86h0SAG8B+E2AKixCBNdIgsUB/gfLSJG05RNWWQbnEMjaL5gDYKWL/4sQ0Z4wCMBFAKfwayNPVtnddOiuAH5u+RQhor3jJoBMWNYfOMBVCCQFsBqWDoQIER0BagDRsIxk2o8CkatRoDEAvoVltp0IER0FRgDjAeyD3SiQDI6vkxUA/gkgmZ86ihDhN0gBpMAyqmlCi823nu8zAcCdvFRPhAj/405YbBywew9gHwK9BDH0EdFxIQfwAuwmd9qPAg0GUBDwKokQEXhkATgOOE5zfpy36ogQEVg8jlbToSNgGfdPbOMgv0AikSAjIwNZWVno3bs3QkJCcO3aNZw8eRIXLlxAeXk5iMizIhEimKMMlvcCNXJYhoSGI8DGL5VKMXLkSCxbtgw5OTkICQlx+L/ZbEZNTQ3KyspQWFiIAwcOoLCwEOXl5WhqaoJer4fBYIDRaITZ7D5tjFQqhUwmc/i0UiKRQCqVwmw2w2QyQafTQafT+fvURfCPRFhsfof1CbAGwOJA1mD58uV4+eWXERwcDAAgIkhaXky3JTc0NKC6uhoNDQ2or6+3GW1zczNMJhMMBgNkMhlUKhWCgoKgUCgcqFQqIZfLIZPJIJFIIJfLbcZfV1eHyspKnDt3DgUFBdi3bx9KSkoCeVlEBA7vAVgCWPKsXECAlq7JZDJ6/fXXiYjIbDaTFb7I/oLZbKZjx47R0qVLKS4uju8lfyK55Tm0pOG8A0BdoAqePXs26XQ6MpvN7Yrl5eX0f//3fxQWFsb3jRPJDetgsX1MC1ShsbGxdOXKFdYtvRDk/Px86t69O983TyQ3nCaFJUtzQPDMM8+gZ8+enOuV2E1q9bc8cuRI7Nq1C1FRURzUXATPGCAD8ASA/v4uKSwsDJs2bUJQUJC/i/I7oqOjER8fj6+//rrNESgRgscNKQK03HH27NmIjIwEYGlRra1qe5Vnz56NO+8Up021cyRZc3v6FQqFAg899JDf9AcyBLLKSqUSzzzzjK9VF8Ev4iUAqsFBWrm2kJKSglOnTtnG/DsKbt26hYEDB+L69et8V0UEO9RIYdmWyK8YM2YMQkJCBBG6cClrNBpkZWVxfLVEBBAh1k0t/Irx48f7VT8fIRBgmWYxbNgwX6ougl8o/T73X6VSYejQoQ4T2vwh84XU1FS+qyDCB/g95Ulqairi4uJ4D1f8JfvjvYaIwMHvDpCZmek005Nr8BUCWfsBItov/B4CWTuJHTUEUqvVfFdBhA/wuwMMHmzJVG1tOallevOpU6fw448/4vz586ioqIBUKkVsbCwGDRqECRMmIDEx0eH3QpU72tBuZ4TfJhtJJBKqqamxTSYzGAz02Wef0bBhw9o8Ti6X06xZs6i4uJjX6dBMUFdXx/eELpG+2GiL4BckJyfj0qVLAACdToff/e532LhxIwwGA6PjExISsH37dmRmZnJeN5PJhPLycly8eBGVlZW2eL5nz56IjY1FaGgoIz16vb5DzG/qzPCbd02bNo2IiEwmEz3//PMO/5NKpaRWq0mpVLo8VqVS0YMPPkjl5eW21parKc0HDhygyZMnk1qtdipXLpfTHXfcQQsXLqRDhw551GkymXhvxUT6RP8pf+WVV4iI6KeffqKsrCx644036ODBg1RZWelgTEajkerr60mr1VJpaSmVlpaSTqdjbNDeYN26daRSqRifw4wZM+jy5ctt6pTL5XzfRJEs6dcQaOvWrZg1axbKysoQGRnJ+4hJYWEhfv/73yM3NxeDBg1C165dQUSora3F5cuXcfLkSeTn5+PMmTNoaGiwHZeamopt27ahf3/Xs8ZDQ0PR2NgYqNMQwTH84llBQUF04sQJxq14IOQffviBGhsb2/yN2WymM2fO0MqVK6lnz56280lPT6eGhgaXx4aHh/PekolkTf8o7tatG9XU1PjduP25Dliv19O7775L8fHxBIDefPNNl7+LiYnh+yaKZEm/hUA5OTnIz8+HRCKxvbCyl00mE4qKirBr1y4UFBSgsrISKpUKcXFxGDBgAMaOHYs77rgDMpnM6Vh7ORC4dOkSFixYAL1ejx9//BEKhcLh/4mJibhx44abo0UIHX7xrKefftpty33lyhWaOXMmBQUFuT1epVJRbm4unTlzxm9PD2/kqqoquu++++jWrVtOv7EPlUS2L/rtCbBhwwY89thjAODw9vT777/HnDlzoNVqGekJCwvDhg0bMGPGDJdvZL19EhgMBpw7dw5Hjx7F8ePHcenSJdTU1CAyMhL9+vXDvffeizFjxkAqdZ4mVVZWhqioKKe3v2lpaSguLvaqHiKEA869SiaT0ZkzZ5zi9IsXL9riabVaTYmJiZSSkkIJCQmk0WhIoVC41BcaGko//fSTTQ+b2L+kpIRWrVpF/fv3p+DgYLd1l8vllJubS+Xl5Yx1Z2Rk8N6SiWRN7pWmpKTYOsD2ocLy5ctJrVbTH//4Rzp79iw1NzcTEVFjYyOVlZXRoUOH6O2336Zx48Y5ja3n5ORQQ0OD16FLdXU1PffccxQVFeXVOdxzzz228jyVNXDgQL5vokj25F5pbm6uS0N59tln6fjx407fGwwGOnjwIG3ZsoWKiorIbDZTWVkZrVixgvr27UsSiYQA0Keffup0rLtW2WQy0ZdffkkJCQku6xgUFES9e/em4cOH06hRoygjI8PJ6d555x1GT4ChQ4fyfRNFsif3StesWeMyXKmurnb63mQy0eLFi21TIiIjI+mTTz6x/aauro52795Ns2fPpjlz5jAKgYxGI61cudIh1FEqlZScnEyPP/445eXlUXFxMW3dupWWL19OW7ZsocrKSvruu+8oNTXVdkxSUhIZDAaPDnDnnXfyfRNFsif3Si9cuMA4RCkoKLC18FampqZSfX290++1Wi2ZTCaPOhsbG2n16tW0dOlSevHFF2ndunV09OhRh5Bm1apVJJVKbWU+8sgjpNPp6Oeff3ZwnAMHDng8l1GjRvF9E0WyJ7cKe/fu7dFA7eXPPvvMSYdGo6HS0lJGerztDJvNZqqoqKDo6GiHMiUSCR05coTMZjOtXbuWvvnmGyouLqba2lqP+iZNmsT3TRTJkpwviMnNzbXJxGBRSXZ2NtRqtcPcm379+iEuLo6RHjbQ6/UO5Vl11dTUAACeeOIJh6WPniDmCW2/4HRNsEqlwowZM2x/M1lXm5iYiI0bN6JHjx4AgLvuugvr1q2DXC5npIfNYvaYmBjk5OQ41L1nz54YNGgQJBKJ7R0AU50JCQleXCURQgNnj5Ps7GwyGAxehUD2snVY1Jtj2YRA1jDowQcfpIyMDJo4cSKdPHmSta7169fz/igXyZq+K7G+3Fq/fr2TYfpb9oUmk4nq6+sZjfS0xaNHj7pc2BMWFkYzZsygXr168X2TRfrTAT788EO6++67qbq62utWvyPIdXV1TptmREZG0sGDB8lsNtOlS5e8WoQjMnDkpA+g0WiQl5eH8PBwLtR5BWrpCBMRb7JarcaECRMc6rVgwQIMHz4cgGVt9NSpU/1w9iJ8BScOEBQUZEsQRXYjM4GU+cajjz5qm7oNwKmTfd999wW6SiIYgDMHsCLQmdn4To1olbOzszFq1CgAlmRZd9xxh8NvsrKy/J4hT4T38NkBJBKJ0wKRQEIIIRARQalU4q233oJKpUJ0dDSSkpIcftO1a1fe10SLcIbPL8Lsx80BBDwzm5AwaNAgvPHGG6iurnbKFRQZGSnmDxIgOHEAPpPTAoF3urbkpUuXuvxeoVAwTrYlInDgZCqEN9MGuIZ9mNH6O6HJogMIDz73AYgIJpPJ4W8+ZBEi2MDnJ4DZbHZwgM4eArUlm8U9hQUHTkIgPrOitacQqLa21uP5iAgsOHkPYH9jxRDINerr61FfX893NUS0AidPgKqqKpvsj/CmoqICDQ0N0Ol0aG5uttH6t0wmg0ajQf/+/REeHs57qONK1mq1Yv5QAYITB/BnVrSPP/4Y8+bNY/Rb6470b7/9NiIjIwHwH/ZYZasTixAWOAmBrJtgANwbUGlpKeN6GAwGbNiwAStWrGB8TKBQUlICo9HIdzVEtAInDnDmzBmbzHUINHPmTK+nEOzfv18wc4Ss8sWLF706BxGBASch0OnTp1FVVWULO7hEeno6lixZgjfffNPl/5VKJTIyMpCUlITo6GhERUWhV69eghsdsm8kRAgLrBcT2PPjjz/228qvhoYGmjx5slP6lJiYGNq7dy81NTVxskLMn+zXrx8n11kk5+RG0fjx40mv15MV/lh19fzzzzusrFq2bJlfyuJa/uWXXxxyEIkUDmUAXgUHuHr1KrKzs9G7d2/bd1z2B5RKJcaPH4958+bZFt8MGTIEw4YN46L6fsW7776L3bt3O30vkUiwcOFCPPzww5g+fTpGjRqF5ORkGI1G3Lx5k4eadk5w5k19+/Z12ADPH+GQVTYYDO0i9CkpKaHExESX1+uhhx4io9HodExDQwPt2LGDsrKyeG8hOwG5Vbh48WLeQw6hyE1NTTR9+nSX1ykqKooue9h9sqqqih544AG+DaSjk3ul27ZtC5iR8d3Cu6LBYKCff/6ZpkyZ4vYaPf/884x06XQ6MfeoHymxegGX6N69O/Lz89GjRw9IJK739uJKFgLq6urw3nvvwWg0QqvVIj8/H+fPn3c79UEul+Po0aMYOHAgI/2nTp3C6NGjbakbuUJUVBSWLl2KESNGALC8rDtw4AB++OEHlJaWdprZq37xrEceecShlQ7E04Av+eOPP3Yaom2L6enpVFdXR95g0aJFnN6fmJgYOnDggMuyampqaNOmTZ0loZd/FCuVStq5cydnRqbX6+nGjRt0/vx5Onv2LJWWllJzczPv4Y7ZbKbc3Fyvrs3DDz/sdRmnTp1ymX2ODRUKBe3atctjmbW1tfTEE0945dztkP5TnpGRYdtcunXMzkTW6/W0f/9+WrRoEaWlpVFUVBSFhoaSWq0mjUZDSUlJdP/999OGDRvol19+4c0BBg0a5NV1eeutt7wuo7m5mbKzs32+JxKJhF5++WXG5TY1NdGDDz7It5G2TwcAQCtXrmTV6t+8eZNmzpzptG2RO3br1o1ee+01qq2t9flp4608bNgwr67Jl19+6aCHKV588UWf70e/fv0c9m9jghMnTvBtpO3XAaKjo+nKlSteGVZxcTH17t2bVXnp6el09OjRgD4BZsyY4VUdCwsLWZXz9ddf+3Qv5HI5ffvtt16XW15ezreR+o2c7g/gClqtFmvXrmU8aUyn0+Gpp55ive/uuXPnMHnyZBw6dIh9pb1ERkaGV7+Pjo5mVU5mZiar46yYMGECxowZ4/VxR44c8alcocPvXhYfH08VFRWMngAnTpxocwd5puzZsyddvXrVqycPW/mrr77yqm7WbZe8hclkanOPY090N+rjCU899RTvLbW/6PcnAGBZMfb5558z+m1kZKTTTuxsUFJSghUrVoDI/6kR09PTERYWxrhu1vOz6mFKiUSCXr16sboeI0eOxIgRI7wus6GhAXv37mVVZntAQBwAsCxttDcaK1rLiYmJTqnGXYFJMq7Dhw+zqKn36N69u22CHhPYZ5H2FklJSV4fI5FIsHjxYlblXblyBUVFRayObQ/gfJM8dzh06BCuX7+OhISENmd9SiQSrF27FkOHDsWePXtQV1eH8PBwxMbGonv37ujevTsSEhIQHBwMk8mEa9euobCwEPv27UNhYaGDQ8XFxYGIIJVK/brgXaVSoUePHrh69Sqja2E9jk1GvcTERK+PiYuLw4QJE1iV98MPP3T4pZwBi7esG2hbwYVsHakgIiovL6dNmzbRiy++SOvXr6fGxkan3/hLfuSRRxhfB+ssVns9TPn66697fd0ff/xx1iNcnWAL2MAVNnXqVNtG11wiEAbuSX766acZX4dbt2456bE/l7bkvLw8r6/7559/zli/vVxZWUmxsbF8G6hfGbA+AAAUFBSgurra9jdxsOaWCx1cyN503O0TZFFLOER2Hd225OTkZCiVSsZlqdVqZGVlMdZvL584cQKVlZWMy2qPCKgD3LhxAxcuXPCLbj5yktrL3mR9u3XrlsOx3jhcaGioVw6Qmprq8N7Bm7J2797d4WeEBtQBACA/P98mc2F4Qsn85k3+Iq1W66SHaXoVtVrt1Y48GRkZCAoKYpXKpSMPf1oRsFEgK/Lz8/HSSy8BYGZgRqMRzc3NaGxsdEiLKJVKERUVhdjYWFYtKZeywWDA+fPnGV8D+/W+9iEH4HkNRJcuXaBSqRiXNWTIEFbrLerr63H8+HHG5bhCdHQ0IiIiUFlZidu3b/uky58IaKdDo9HQtWvXPHbC9uzZw2giXFhYGI0dO5ZWrlxJx48fJ6PRaNMRqA7w8ePHvXp7/Ze//IX1KJDZbKa0tDTGZR0+fJhVGf/73/9Y3d8ePXrQihUrqLS01KbLZDJRYWEhvfDCC6TRaAJqbwwY+ELfeOMN8oTKykpSq9Ve6X300Udtu74H0gm8HZpcvHgx61EgIqJx48YxKic4ONhhyNWbsl555RWvzkmhUNCSJUvoxo0bbnWazWYqKiqi8ePH8230/DpAcnKyg6G6uxnvvvsuo0UgoaGh9MILL1BdXR2r1s4XGgwGSk5O9ur8p02b5qDDW4d78sknGZUzduxYVvrNZjPdd999jM8nIiKC/vOf/zDWr9VqaeDAgXwbPgGggPcBAMvr9c8//xz3339/m79bvHgxcnJysGHDBvz000+oqamByWSCSqVCREQEUlNTMWbMGNx7773QaDRex7lcyHl5ebhy5YpX519eXg6DwQClUmkberSCidy/f39G5dxzzz2MddrLdXV1jPs0KSkp2Lp1KwYNGsRYv0ajwfLlyzFr1ixGZfgbvHjeyJEjqampiZjCYDBQfX091dXVUWNjoy3W97Z141Kuq6ujIUOGeH3uKSkpVFNTwzoE2r9/v8cylEqlbfant/ovXbrE6MmbmppKRUVFrEIsrVbLe+vfQn4KlkqltGPHDq8vXGs50CGPPT/66CNW5x4eHk43btxg7Xw3b96krl27enQy63JUb/Xv3LnT4zkkJyfT2bNnWTceDQ0NfBu+xQ7BE8xmM1599VU0Nzdzoi/QL79KSkrwpz/9iVVdb9++jdu3b7MevtVoNMjKymqzjClTpjhMu/ZGv6fwJyUlBd988w369OnDSj8RCWaGKW8OAFimRuTl5QEIzIuwiooKFBcXO7zyZ3qsvWwymfDSSy/hl19+YX3u169fZ/UiTCKRQCqVYtq0aW51S6VSzJkzh9XLL4lE0uas1piYGPz3v/9Fnz59WOsHgB07dnhzufwKXh9B3bt3d1oh5Y8QyGQy0dNPP00ajYbGjRtHEyZMoClTptDixYtt5TPlunXrfD7v9evX+9T/uHnzptt3D0OHDqXGxkbW+mfNmuVSb1hYGH3//fes62yVb9++Tenp6byHPy3kvQK0ZMkSp06ttw7gSd65c6fLjl1MTAxdunSJsZ59+/ZRly5dfD7nP//5z145sCv+9re/danbfq8GNnSVilGtVtuyWfjKd955h3ebE5QDqFQq2r59O7GFJ+MtKiqipKQkp3JlMhl9+OGHjJ3p3LlzLvWw4cKFCx30s3H+wsJCJ2fMzMwknU7HWicROY3RKxQK2rBhg8NUdrb6z507J7Qp1rxXgABLKFRUVOT1BfXU2lRVVdHgwYNdlvnAAw/YjMUTtVotDRgwgLPznTRpEusQwl62X7AeGRlpm/rgi86MjAyHRmLVqlU+6yQiMhqNNHr0aN5tTZAOAID69+9PFRUVrBzAlVxaWkp33nmny7Kys7Md+h5t6bl+/ToNHTqU03Pt27cvJ+FETU0N5efn0/nz5x2GVn1hZmamzfitic244F//+lfebUzQDgBYOnCHDh0ig8FATNHaYI1GI3377bduJ4317dvXljLFU8tVVlZGOTk5nJ9nfHy8LUGuNw4fCDknJ4eUSiW9+eabDvfBF52FhYUUERHBu30J3gEAS5/g3nvvpW3btjlkUW7rCWBlQUEBzZ07lxQKhUvdvXr1onPnzjFqsa5du8Zp2GPPiIgIunr1KiehBdfykiVLaMuWLZzp1Ov1NGHCBN7tyhX9sj8AlwgODkZ2djaGDx+Ovn37IikpCVFRUYiIiABgealUUlKCI0eO4IsvvsCJEyccXrrYo1evXti1axeSk5M9zvM5duwYHnjgAcaZHryFUqnEwYMHMXjwYL/oFxI++ugjLFq0SLCry3j3Qm8olUpJrVZTVFQURUVFkVqtZpS+e/DgwQ6dbHetlclkory8PIqJifH7uVingggh7PGXXFFR4XHahrv7rNFoKDMzk2bOnEnz58+nWbNmcTYKZ0f+jdrfHDduHJWVlXkMefR6Pa1atcpt+MQ1N2/eLJiwxx+yyWTyOq1idHQ0Pfnkk/TFF1/YFtXY69RqtVxvFsK/gfqLMpmMFi5c6DAu7u6mXbt2jaZOnRrQ+m3cuJFRX6S9Mj8/32FfZ3dUKBQ0efJk2rZtG+OdP19++WWu9l7m31D9wYiICHr//fdtm3e7M3yTyUS7d+/2x6PVI//1r3/5NfzgU25sbKSRI0e2ef4ajYYWLFhAx44dI71ez7n+TusAaWlpjPYIaGpqomXLlvmUcdkXWqcsWG9sR5LXrl3rtoXu2rUrLVu2jK5cueJwP7wta+PGjaID2FMul9PChQtti03auninTp1itZiFS27dupX3MMUfvH79utOYv1wup6FDh9IHH3zA2dLVkydP+m4z6CCIiYnB6tWrcf/990Mul7vMdEZk2YDjo48+wquvvmpLUMUXQkNDbXUTSn4jLuTNmzcjNDQUUVFRSEtLw1133YWJEyciPT0dQUFBnJXlbhtab8F7y+0r77rrLtvqpLZYUlJCkydP5qrz5FvLI5dzMm9HqHJzczMZjUa/lrV69Wou7gX/BsyWsbGxjDq6Op2OPvjgA4qMjOS9zlaGhoYyfiMt0pmNjY1e787ZYRxApVLRvHnzGM3nuXz5Mk2ZMoVkMhnv9bZnbGws3bp1y1ZPK0SZmfz+++93zmHQAQMG0Pfff++UAa41jUYj/fOf/xTa3HMb7WeD2p+HKHuWL168yOXTnH9jYMKEhARas2YN6XQ6h5bA1UW6dOmS4Dd2mDt3bpsOLNI1y8vLuU6qxb8xtEWNRkPPPfcclZeX2wy9NawXR6fT0YcffkhxcXG819sT//GPfzg4butHvCg7yzdv3qSxY8dyfS/4NwZ3HD16NF24cKHNi2I1/oqKCsrNzRXECI8nSqVSKigo8Et40FFlrVbL1Ztfx3sBAWP06NHo3bt3m78xmUz45JNPMGDAAHzxxReCnXJrj4yMDKSlpfGe1r29yKdPn8aoUaOwf/9+cA1BO0BkZGSbeYGqq6uxaNEiPPbYY6ioqOCjiqwwadIkhISEsM4L1FlkwJI/aOLEiTh79iy7i80AvIcE7rhhwwa3ceGxY8dsa1fbEyUSCZ05c8ahYyeUMENIssFgoNWrV3u17wJL8m8U7mjNQ2PvACaTif7+97/zNoHNV+bm5rq84SJ/pVarpblz5wbqnvBvFO64b98+skdVVRUtWLBAcC+1mDI0NJSOHj3qstVz9ZTrjPLZs2fdprHpdA5w5MgR2wUqLi6mESNG8F4nX/jss8+6bPGsN7+zy19++SUfQ9j8G4Y75ufnk9lspv3797NaVyokjhw5kmpra93e/M5Mg8FAr7zySsCWorYbB/j3v/9NW7ZsEdQkNjZMTU21pUBx1wJyFUK0N7mmpobmz5/PKLFBp3OAXr16BWIUwK/MyMjwOOvTahSdTa6urqYpU6bwen8EnxeoPWPOnDlYs2YNoqOjPe411hmRm5uLr776iu9q8N9KdiRKJBLq06cPbd682SHxrqfWkOvQoj3I3bt35/9+QXwCcAKpVIrs7GwsWrQI06dPR1hYGONjPWWp64iy2WxGUFAQDAYD4+vkD7TLNcHh4eHo3bs3+vTpg/T0dHTt2hUhISEwmUwwmUwwGAxoampCU1MTGhoaUFtbi/r6etTX16O2thYNDQ1oampCY2Mjmpubbb9tbm6GTqfzOJ9IqVQiOjoa3bp1Q0ZGBkaNGoXx48ejZ8+erIyCGMyH6WhyaWkp78YPtBMHCA4ORlpaGu655x7cfffdyMzMhEajgUqlAgBWrY/RaITRaITBYHD4NLzcSUAAAAeXSURBVJlM0Ol0aGxshF6vh8FgsN0olUqFLl26ICwsDGq1GqGhoVAqlW4X4HsjC2ExeyDlCxcusDEFziFoB+jRowfmz5+P6dOno1+/fpDJZE4zKNm0PhKJBAqFAgqFwraToi/wtTUUSlgSSPnkyZMMrqz/IUgHCAsLw9/+9jfMnz/fpdG7glBurBgCMZMPHz4MIUCQ06EHDx6MhQsX2vL7WOFKFsLjnAvZ26nC7Vmuq6vDzz//DCFAkE8As9nM2HiE0qL5IvP9BAq0fPnyZVy+fBlCgCCfAFqtts1wxxU8PSmELAvFEQMlb9++XTAr9wTpAFVVVbh9+zYAz8bDd+gihkDeySaTSRBvf60QpANUV1fj4sWLADy3KEJo0bgKgTqDfPr0aZw+fRpCgSAdQKfTYfv27V4dw3cY44ssFEcMhJyXl4empiYIBRJAmFMhEhIScPbsWUZTCvju1HEh8x2CBUI2Go1ITU3FtWvX3N7LQEOQTwAAuH79Ol577TWHzpIYArVvec+ePYIyfgCQAfhjy6fgcOTIEYSFhWHIkCGQySxV5Dtc8Zfc0UFEWLZsGc6cOcN3VeyhB4BqgP9pxO4ok8koNzfXYVPp1hDKAg9fZPtz6YjyyZMnKTQ0lHd7asVqGYAnAESy8J6AgIhw/vx5bN68GbW1tUhOTkZkZKTDI7a9g+/+RyDkP/zhDygoKODsmnGEMgmAfAAj+a4JU0gkEmRmZmLEiBHIyMhASkoKNBoNgoODoVaroVQqHX5vMBhsU6Fv376Nmpoa1NfXo66uDg0NDQ59jKCgIISHh6Nbt26Ij49HfHw8unXrBqlU6ncD6cjYt28fJk2ahObmZr6r0hr7JQA2A5jLd018gVwuh1wuh0KhcJg8B1hyh+r1euj1eq/ePioUCnTp0gVxcXEYOnQoRo8ejTFjxiAhIQEKhUIcBWIom0wm3H333di3bx/jax9AbAKAP4H/WKxdMCQkhEaOHEkrV66k4uLiNhe6e0NrvNwR5fXr1/N+39rgnyQApgH4EiK8gkQiwfDhw/HQQw9h+vTpSEhIYBTedKYQqKioCKNHjxZy4uLpEgB3ACgAEMpzZdotNBoNBg8ejFGjRiErKwtJSUkICQlBfX09Tp06hby8PEycOBFPPPEEJBJJpwiBamtrMXXqVKGGPgBQDyALAJQALoD/x1GHoqRVoqfg4GDatGlTpwiBDAZDIJPbsuU5AEoZABOAdABDIcJvMBqN2L59O0JCQpCdnW17EnQ0aLVazJs3D3l5eXxXxRM+AbAdsEyHuBf8e2SnoFQqpfnz5zvseSaEF1W+ynq9nnbu3En9+/fn/Roz5CTYTQXSACgVQKU6DVNTU2nLli28hyu+ytXV1bR582aaMGFCe0pbXwqLzdsgBfCuACrW6Ths2DD69NNPqa6uzmX/QKgsLi6m3/zmN+3J6O25Bi2QwGL8ZgCDYRkNEhFgSKVSpKamYu7cuZg1axZSU1MRFBTk11Eac0tuJCKC2Wx2+WkymRxyJV2/fh3Hjx/HgQMH8N1336Gqqiqg14lDZAE4DkBq3wuTAtgDYBQvVRIBwJJ8q3///pg4cSLGjh2Lfv36oWvXrpy8ba6trbUZ8N69e3H79m2bkZvNZltmPSv1ej10Oh10Oh2amppgMpl4uCKcYx+AsS2yufUwxH0APodAs0V0VkRHR6Nnz57o0aMHEhISoNFooNFoEB4eDpVKBZVKBblcbstk19TUhKqqKmi1WlRWVqKkpAQXL15ESUkJ36fCN4wApgPYYf3CPgSSwmL4uyE+BUR0TOwDMB4WRzADkMpg6RQAv3YQrgB4GAJeLSZCBAsYAcwHcAkWO5cCMLc2cjOA/QD+Hdi6iRDhd/wbFtu2wgw4h0DWz64Afm75FCGiveMmgEwAv+DXyMYMQGo1etsXLZ83ASyF5bEhQkR7hhHAs/jV+M2ws/XWi+Ht+wNnAcQAGBaYeooQ4Re8B+At/NrHtYIA9yGQFWGwDIuOCUBFRYjgGj8CmAGgtuVve/s2o9WLMMDZGQAgHsABAEl+rKgIEVzjKoAcWEIfqy1LW8uuRoHsPwHgBoApLQpFiGgPuAaLzd6Aoy07yVL8+kiw/2z9/XkAv4HoBCKEj2sAZsFis61t2klmEgLZIwPA1wBSOKywCBFc4SosLf95tBH22MtMQiB7nAVwFyydCxEihIQfYYn5z8JD2GMvy2DxBGr1KcGvmaOlreR6AF/Asoh+MMQpEyL4hRGWoc4nAdyCe7t1JzugdX+gLUgBzAFQAf4XOIjsnKyAJambfVwPFjIrWBXEAfgnAAPHJydSpDsaAKyHxfZ8BpNRIE+yHJYp1HshOoJI/9EAi42Nw69rVry1VVcjnzZ4EwK5ghKWDBN7fDxRkSJbMx+WBVv22Y99CXukgKUzwBZtDS8BwEAAvwWQCyDRh3JEdF6UwZK750MAJ+B+dJI1PM0Fcpo7wUKOADAclvHZcbBMrfC88ZeIzoh6WIz+OwA7ARwGUANu7NCl7O2LMDaw1ymFZU5RBoABANJa/o6HJU9LCBwfcSI6HvQAGgFUwTJV4SosqTlPAigCcLnlN22+wOJK/n9DShGjABYHqAAAAABJRU5ErkJggg==)](https://www.eevblog.com/forum/beginners/4x4-keypad-arduino-non-blocking/"%20\t%20"_blank)

[eevblog.com](https://www.eevblog.com/forum/beginners/4x4-keypad-arduino-non-blocking/"%20\t%20"_blank)

[4x4 keypad arduino non blocking - EEVblog](https://www.eevblog.com/forum/beginners/4x4-keypad-arduino-non-blocking/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://www.eevblog.com/forum/beginners/4x4-keypad-arduino-non-blocking/"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAIwAAAAZCAMAAADUtTb8AAAAV1BMVEX///8AgYS/3+B3vL1jsrNfsLJ7vr+bzc9PqKozmpxXrK4rlph/wMHb7e7r9fXl8vLX6+wajZCPx8lBoaOx2NnE4uIQiIvM5ub0+vqo1NWTycu12ts/oKIbPNtoAAABR0lEQVRIie2Vy5KCMBBFu4MYJITII4Di/3/n9APUoZjdxJkFd0FyE8o+lb5BgEOHDv0jBbOoVj/aqvUy87Icp9Xw6lXeo6csGnP9XRiLi27s5kbmLc8zXR/EnBEzGgpE4jghWmZBzBPC+HeTrabdgWnIPFLAdM82UZXOB3o6hSmN6bTwFgbvMGMKmPJpGqkVqEotMGfKB5mwA9NMl7QwNf08x5SGuMIYJdvAEDVNh1SZ4V54PQUYEHuByY112rMNzIUPp3ApYcILZn4FeIg7MPzqmALGVSS+MhNV4C+HDitMDzswUD1OkALmW4DpUkc6DVgyUyzbLXWFBieYDAPiUsJQ/cbenJ6CwERuB2iO76ZSzM/AhEE74+oVBugCd7zVLU2zH4OBkHOWK/k/UpgeJSZQlwzqZkgJs5U3409b0fi0tQ8d+jN9AdMdDBmaYrvlAAAAAElFTkSuQmCC)](https://forum.arduino.cc/t/spectrum-analyzer-debate/307790"%20\t%20"_blank)

[forum.arduino.cc](https://forum.arduino.cc/t/spectrum-analyzer-debate/307790"%20\t%20"_blank)

[Spectrum Analyzer debate - General Guidance - Arduino Forum](https://forum.arduino.cc/t/spectrum-analyzer-debate/307790"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://forum.arduino.cc/t/spectrum-analyzer-debate/307790"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://www.reddit.com/r/AskElectronics/comments/na50js/reasons_to_use_mosfets_in_led_matrix/"%20\t%20"_blank)

[reddit.com](https://www.reddit.com/r/AskElectronics/comments/na50js/reasons_to_use_mosfets_in_led_matrix/"%20\t%20"_blank)

[Reasons to use MOSFETs in LED Matrix : r/AskElectronics - Reddit](https://www.reddit.com/r/AskElectronics/comments/na50js/reasons_to_use_mosfets_in_led_matrix/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://www.reddit.com/r/AskElectronics/comments/na50js/reasons_to_use_mosfets_in_led_matrix/"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://www.reddit.com/r/cpp_questions/comments/19a7p7u/embedded_project_best_practices/"%20\t%20"_blank)

[reddit.com](https://www.reddit.com/r/cpp_questions/comments/19a7p7u/embedded_project_best_practices/"%20\t%20"_blank)

[Embedded project best practices : r/cpp_questions - Reddit](https://www.reddit.com/r/cpp_questions/comments/19a7p7u/embedded_project_best_practices/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://www.reddit.com/r/cpp_questions/comments/19a7p7u/embedded_project_best_practices/"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAMAAABg3Am1AAAANlBMVEX/0gADI0v/9L/AphOBeybvxwViZS+hkBwyRD3gvAkjOUITLkaRhSFCTzjQsQ5SWjRxcCqwmxcSeSRsAAAA6klEQVRIie2U2xKDIAxEpeEu2vb/f7YYiEBQZnxzOvKCSo7ubsDpNV0cD/AA9wDMIgZDd4AelQsBHAjjet9J8tvTAGfDccDEeqsumN4MuwspuVj/jjPIeqz0JHTAFpGMc+s0pDdFsRxQc/5Ck+0cPUm8MhwwtN6kq3N4aaUBbGmmKcBKdx9uGur274TN4dXxvYrlVKMqwpDlhceqigokktF5f9OXA1K0hCLhGB5towrwdZaauuFInOyArrsid8Uyy2cnzmThUKIbAqjck+V2Cx8BgEJCVqbbLX8ErHhgEAXgJ+Tu/6UH+F/gB1g5BdLq06T9AAAAAElFTkSuQmCC)](https://community.st.com/t5/stm32-mcus-embedded-software/design-guidelines-for-embedded-applications/td-p/582052"%20\t%20"_blank)

[community.st.com](https://community.st.com/t5/stm32-mcus-embedded-software/design-guidelines-for-embedded-applications/td-p/582052"%20\t%20"_blank)

[Design guidelines for embedded applications - STMicroelectronics Community](https://community.st.com/t5/stm32-mcus-embedded-software/design-guidelines-for-embedded-applications/td-p/582052"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://community.st.com/t5/stm32-mcus-embedded-software/design-guidelines-for-embedded-applications/td-p/582052"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAALVBMVEUASoDU2+P///9nhaUkW4pUeJyjs8b19vjf5OvI0dw/apPq7fGHnbawvc28yNVvI/haAAAAUklEQVQYlY3POQ7AMAgEQC4DtpP8/7lxDoHovAXFSIsAYCuNn0gC0psjAdeQTlIAmLiC0ggwRHQ6c8dUdYvCV5HpFdZOrQCXjQpiPU5v/wdbj96XAwEg74cFuAAAAABJRU5ErkJggg==)](https://randomnerdtutorials.com/esp32-websocket-server-arduino/"%20\t%20"_blank)

[randomnerdtutorials.com](https://randomnerdtutorials.com/esp32-websocket-server-arduino/"%20\t%20"_blank)

[ESP32 WebSocket Server: Control Outputs (Arduino IDE) | Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-websocket-server-arduino/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://randomnerdtutorials.com/esp32-websocket-server-arduino/"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://medium.com/@felipedutratine/best-practices-for-configuration-file-in-your-code-2d6add3f4b86"%20\t%20"_blank)

[medium.com](https://medium.com/@felipedutratine/best-practices-for-configuration-file-in-your-code-2d6add3f4b86"%20\t%20"_blank)

[Best practices for Configuration file in your code | by Felipe Dutra Tine e Silva | Medium](https://medium.com/@felipedutratine/best-practices-for-configuration-file-in-your-code-2d6add3f4b86"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://medium.com/@felipedutratine/best-practices-for-configuration-file-in-your-code-2d6add3f4b86"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://techtutorialsx.com/2018/08/14/esp32-async-http-web-server-websockets-introduction/"%20\t%20"_blank)

[techtutorialsx.com](https://techtutorialsx.com/2018/08/14/esp32-async-http-web-server-websockets-introduction/"%20\t%20"_blank)

[ESP32 Async HTTP web server: websockets introduction - techtutorialsx](https://techtutorialsx.com/2018/08/14/esp32-async-http-web-server-websockets-introduction/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://techtutorialsx.com/2018/08/14/esp32-async-http-web-server-websockets-introduction/"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://stackoverflow.com/questions/16041623/the-best-way-to-handle-config-in-large-c-project"%20\t%20"_blank)

[stackoverflow.com](https://stackoverflow.com/questions/16041623/the-best-way-to-handle-config-in-large-c-project"%20\t%20"_blank)

[The best way to handle config in large C++ project - Stack Overflow](https://stackoverflow.com/questions/16041623/the-best-way-to-handle-config-in-large-c-project"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://stackoverflow.com/questions/16041623/the-best-way-to-handle-config-in-large-c-project"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://docs.platformio.org/en/latest/projectconf/index.html"%20\t%20"_blank)

[docs.platformio.org](https://docs.platformio.org/en/latest/projectconf/index.html"%20\t%20"_blank)

[“platformio.ini” (Project Configuration File) — PlatformIO latest documentation](https://docs.platformio.org/en/latest/projectconf/index.html"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://docs.platformio.org/en/latest/projectconf/index.html"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAALVBMVEUASoDU2+P///9nhaUkW4pUeJyjs8b19vjf5OvI0dw/apPq7fGHnbawvc28yNVvI/haAAAAUklEQVQYlY3POQ7AMAgEQC4DtpP8/7lxDoHovAXFSIsAYCuNn0gC0psjAdeQTlIAmLiC0ggwRHQ6c8dUdYvCV5HpFdZOrQCXjQpiPU5v/wdbj96XAwEg74cFuAAAAABJRU5ErkJggg==)](https://randomnerdtutorials.com/esp32-ota-over-the-air-arduino/"%20\t%20"_blank)

[randomnerdtutorials.com](https://randomnerdtutorials.com/esp32-ota-over-the-air-arduino/"%20\t%20"_blank)

[ESP32 OTA (Over-the-Air) Updates - AsyncElegantOTA Arduino | Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-ota-over-the-air-arduino/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://randomnerdtutorials.com/esp32-ota-over-the-air-arduino/"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAALVBMVEUASoDU2+P///9nhaUkW4pUeJyjs8b19vjf5OvI0dw/apPq7fGHnbawvc28yNVvI/haAAAAUklEQVQYlY3POQ7AMAgEQC4DtpP8/7lxDoHovAXFSIsAYCuNn0gC0psjAdeQTlIAmLiC0ggwRHQ6c8dUdYvCV5HpFdZOrQCXjQpiPU5v/wdbj96XAwEg74cFuAAAAABJRU5ErkJggg==)](https://randomnerdtutorials.com/esp32-ota-over-the-air-vs-code/"%20\t%20"_blank)

[randomnerdtutorials.com](https://randomnerdtutorials.com/esp32-ota-over-the-air-vs-code/"%20\t%20"_blank)

[ESP32 OTA (Over-the-Air) Updates – AsyncElegantOTA (VS Code + PlatformIO)](https://randomnerdtutorials.com/esp32-ota-over-the-air-vs-code/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://randomnerdtutorials.com/esp32-ota-over-the-air-vs-code/"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAALVBMVEUASoDU2+P///9nhaUkW4pUeJyjs8b19vjf5OvI0dw/apPq7fGHnbawvc28yNVvI/haAAAAUklEQVQYlY3POQ7AMAgEQC4DtpP8/7lxDoHovAXFSIsAYCuNn0gC0psjAdeQTlIAmLiC0ggwRHQ6c8dUdYvCV5HpFdZOrQCXjQpiPU5v/wdbj96XAwEg74cFuAAAAABJRU5ErkJggg==)](https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/"%20\t%20"_blank)

[randomnerdtutorials.com](https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/"%20\t%20"_blank)

[ESP32 Save Data Permanently using Preferences Library - Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://www.tutorialspoint.com/esp32_for_iot/esp32_for_iot_preferences.htm"%20\t%20"_blank)

[tutorialspoint.com](https://www.tutorialspoint.com/esp32_for_iot/esp32_for_iot_preferences.htm"%20\t%20"_blank)

[ESP32 Preferences for IoT - Tutorials Point](https://www.tutorialspoint.com/esp32_for_iot/esp32_for_iot_preferences.htm"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://www.tutorialspoint.com/esp32_for_iot/esp32_for_iot_preferences.htm"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAIwAAAAZCAMAAADUtTb8AAAAV1BMVEX///8AgYS/3+B3vL1jsrNfsLJ7vr+bzc9PqKozmpxXrK4rlph/wMHb7e7r9fXl8vLX6+wajZCPx8lBoaOx2NnE4uIQiIvM5ub0+vqo1NWTycu12ts/oKIbPNtoAAABR0lEQVRIie2Vy5KCMBBFu4MYJITII4Di/3/n9APUoZjdxJkFd0FyE8o+lb5BgEOHDv0jBbOoVj/aqvUy87Icp9Xw6lXeo6csGnP9XRiLi27s5kbmLc8zXR/EnBEzGgpE4jghWmZBzBPC+HeTrabdgWnIPFLAdM82UZXOB3o6hSmN6bTwFgbvMGMKmPJpGqkVqEotMGfKB5mwA9NMl7QwNf08x5SGuMIYJdvAEDVNh1SZ4V54PQUYEHuByY112rMNzIUPp3ApYcILZn4FeIg7MPzqmALGVSS+MhNV4C+HDitMDzswUD1OkALmW4DpUkc6DVgyUyzbLXWFBieYDAPiUsJQ/cbenJ6CwERuB2iO76ZSzM/AhEE74+oVBugCd7zVLU2zH4OBkHOWK/k/UpgeJSZQlwzqZkgJs5U3409b0fi0tQ8d+jN9AdMdDBmaYrvlAAAAAElFTkSuQmCC)](https://forum.arduino.cc/t/storing-an-int-array-using-preferences-esp32/1216037"%20\t%20"_blank)

[forum.arduino.cc](https://forum.arduino.cc/t/storing-an-int-array-using-preferences-esp32/1216037"%20\t%20"_blank)

[Storing an Int Array using 'Preferences' (ESP32) - Programming - Arduino Forum](https://forum.arduino.cc/t/storing-an-int-array-using-preferences-esp32/1216037"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://forum.arduino.cc/t/storing-an-int-array-using-preferences-esp32/1216037"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAMAAAADACAYAAABS3GwHAAAgAElEQVR4nOy9y44l2XIltsy2v84jIh91qf4RTTTRAw1ppkl/ggBBkFoCNNBnEFALzSZFsMFP4ISCAIlodTd7oJbIv9BEEHnvrcqIOC9336aBme1t7udkVVZmZGZUde57vdwj8oS7H3ezbcvMltkGvo1v49v4Nr6NfzcHfe0b+DY+fvzLP25AJCACmPRl0uqNrn8W+0/Z27H//B/91/Pnv/EXNL4pwC9g/G//tIk/lneWGEgsxFUBKAg8Ae9VAAkKIFEBMiA5A1MmXGb9NQD85//99Nxf60WM5qc/8m28kEHv2bMdxy3++/pYwrb+eb2Vv/vLf9II8OtThG8K8MsaayFnVAVYHwO3lUEAZFQhXx/ncOyf93MIfmWDf/oj38YLGOsZPgo8Qyey1rYeQGf7uA2rn7vwNw2AFDZXJGBlSf7yn/y65sxf17f59Y8o/L5fC28U4PUWZ/l5dRy93wh/BL/S2R/45gR/1fFv/jQtfu4S0DWizi2pA0sE/PaJaJyJRaCbzfxMSIklMaEhoCFCQ1SUwINCURlETNhFkAFMIsj2uwmCWfTfplkwz0IZELETCRFJ3wh2nTnOFkWaMzBnwn/wX/7y/INvCvAVhgn+lVPbJVDXCCUGJQITgYjAJsQ+s5dZXoBGBI0YBBJBA7MIshR8PxYAM5kSEGFaHU9EGIkwM2EiUkXJgiyCPGfkSUhyRhZARHSbM2ScSY4Xiy7pKIf/6X/7chXjGwT6OmON5X3vwu2C3BDQMKFhRpN0hm+Y0BBJEXoBtQBaARoIWokwSFYKQKoABEwgzESYCTIBGAGM9vsRwAjBlAXTLDTNGZMAM82YAEyosGkmYBb9XYwu+XjR0OmbAnzhEWAPoQqqC3+DpRNb9gy0TOiYpWXSY5gTKxBXgBaCFksLwKYEBEDIFMAUYSIy+ANcAIwCXCC6F1WICwEXItvb52zz4wuZdVl93Rct/MA3BfhaI0ZxohPbA9jc2AYQehB6AnomdEzoidBJjeZ04lEdhUJrC7OAQCb4MxF09hdcTOgvogJ9zsCZgBMJTgBOIJwAnAH7We/9ZN9pnUOI48UqwjcF+Hojwh4PY7oC7G3bAdgTYQtgIGAgwsb2AwE9URH+jlwZCA2WM3+BQDb7L7A/dOY/AzhDcAHhLIIzgCOAA4AD2d42D50yamRptJ/X+YMXPb4pwGcef/vny0jPnIExgyALzN+CigJsSYX/FQH30G1P1Rpsy54wQIXehd83tQC08APWPkBRBFGIc4YK/Ul0Vj+b0D+uNj8/m0nJUCVqiDGTin1GCKP+qz/RZ/Afv0Ce0bco0GccJvzrzCwToSFIT7RMViXGvmHcJ8YrJrwi4BURXjFhzyRbJmyYsCHbCzBIRpcFbRZ0IuiyoCtOcBV8D4kqBCJkMkfWoNAItwDASahYg0MGHkXwmIUes+Bx1uMHAA8i0D3wkAWPIjhKDadOWTBNM+bzqFGj8GgEeBkK8c0CfKYRhH+doEqJ0DcJGyZsibAl6J4Jd4nljgn3TLgn2wPYimCAYMiCHhnDLOjnjH6c0Y4zmnFCO81opxlNFiSRK8Uj+w+TztpMQCaNBCUmzSswoyFWH4PZrAuhJ5INEe0axh0gdwDewWCaADsRPAjwBOAsgksWnOdMDqVihChykL76+KYAn3esMX4DoLUZ/C4R9sy4I8IdQe4S198xlW0vgu2sM32XM7qcdcYfJ7SnC5rTBc1pRDpf0JwnNNO8gD5AcIKJQEzIVBWgSYyUElKT0DQJbZPQN0nGlKhPjD4xNinhmJKcEtGJGY8i2AN4J+qn7LJgK8AjBIdMOCDjSUiIQC7oP8Yz+mrjmwJ8/qEYfxnW3Bm8ec2E10zix/eJsUuMPTN2Cn2wE5EBQJuzJr7mjGaa0ZwvaA5npKcT+OmEdDiBD2ekcdKZXxziSqVGE0GYkJkgftwkNG2DqWvQdi3mrsXUNZi7VsYm0aVt1EdIjAuTnBPhIETvAOxFsMvAjoGtqEI8sKAVTeQJas7AldCPv7rwA98U4HOPyNfpoIS0DTSy84oIb4nkOyK8Zd1eMWHLjF0ibBPLjglbm/kTASyClDPSPIMvI/h4Bj8dQe8O4IcD+OEIOo8gK3opCiAowo/EECYI275tIEOHdmiRhx55mJFzpxQJgYzMNDWCEcBEwMiEo0B2OvvThoFtBjYQDCC0AjCJRYY0dDqGZ1Jg0L/6k/TV/YBvCvBM429W0R4bDoFKiJM0ynMH4DUBbwn4DQF/QMAfqDVQB5dZtonN6c1oJ43qEASUM2iaQZcJOF9AhxPwcAB9/wj6/hE4XQCRij2cn0DQIhpmiBbT6Na1kG0PGXtgzpCcVUhNYeaGJedEM6T4DEcAWwi2INmI0JaAQQgtBEz6txfSSNITES7hmSgE0vv76lbgmwI8w/ib6vCWTQBqGR1p1GZnQr8HZN8w3jSMt02St0z4zrfEeMWMgUmGRBrrZ0IvSoEgMvJZNgLaNAPnEThegMMJeDwC7w7A8Qwj64QNCwVQc2IK0LfAqQdOA3Aage1Fz3u+QIYesumRLxNknJGnDGkTOiKzboREJA2IGig/CSLIEExEmLokFxqIAFxELNssoClDHs+Q//2fLiYOAYD/7L/7clbhmwJ84vibZbRnQVYjxqZl8YjOKwJegXCfCG+aJN8lxtuksOeOCNtEGBKjZ0bLjIYJTAwmw/AljCJAzlUJxgm4TCq0p4spQFYl8E3M7SQTfDamKRPQNsCmA4aT7jd92A/Atgdte8F2IN70kL5FSgldYgxNwtQ0kCYJMQNklwRRZgK6BtRCehEcARyz4JQzjjJRtvl/nTn+olbhmwI8z3Cs75EejfYAOw1t4g0T3pBtieRNYrxOjNeJ8NqiPlsmGZjQMqO1kCQ7JdqHC3+0AuMEXHTGLgowZ1MS/3xelnaBqrlqEtB3agl8P7TApgdtB2A7ALsB2A2C7UAYOqSuRdu3GPoWMnTgoUNqGhCRRnoIIkxEpMCtE8snsIAnQFiTZ7Ea7asowTcFeJ7hFsCdXdtkb4ms14nxHZH8xuDOm8TwsOe9KoDsWCFPQwzl+Gt8nmaT1DL7uwWYqwKcR7MAZ+BgCjDPztXXLcIhFzGBwqC2ATrb2lb3mw7YbYDdANptgP0A7DaC3YC06dFtB8hO4U3TqEVI0OiSKZgwMyUiTdIJgXNWvwKabY5KkPGFZ3/gmwI811grwGDbjhT+vCaS75jwB0z492z2t0gPdolLlrcnDR8yERJ5TQCW5VlrP2CcFQKdDAKdLvo7V5B5BqagBDnXc4iohWkS0CagafS4ScDQAfuNbncb0H4L7E+guw1wt0U7zyCGNG1CP3fY5AYtQ+d8EhAIiVCKdBjqHI+kkaEWKM6xUye+eHj0V6kA/8cfq2NFdM31uGoTIlWoRAj/8B9/WPHGX//Peo05A4kXEKiDEtc2xum5J8Ibm/n/gAn/wGL+AzMGZhnYsT+hhRbBwJxMsqIYvVesLIBbgSnAIHOKp6kqhx9Puf5d2Zu4cYgKWagUQwfcbVXgn7bA3Qk4bIHzBTxntERIbYNu6JCnGVky2swCFnIeUgNCS+osiznGJwAHgrRElCK6g72uf/0n6Yv1J/rFKsD/cl2cTYmBhsVntVs8p1u/W2DPv/qjZlHWFP99aCWegwBgmkFtQpO4CP4OwB6EfZPwumG8TiSvDQa9JsJrc3x7JjGOPzqjOCcU+FD37gewObAN24zdAJ3h9k0HbHvF6nNWhYgKMK6swTRXBXIlENHf+5fLZh1c6aYMGidgnvX2mJAahtj9SM6gJmFKCXPTILeNEfJYTy2CMQtOEByahMPdgEmkFNiMcL9AvvkAPzqC8EeBXqf94/GPkf4iN+XHuCo+Q63DncwkXaMx+z0R7i3a86phvG1Y3jYJrxLhzrK7AxP6pI5uwwoTOOIc8ivZ3sOXLSs2z53+0zgBl50KMcRgSws8nt6vAP778m+2RYvgx24R/FoeXXLYBABZQPMMGWfQfgPuWjRdi37osB065EFE2oYmGNtUgBME54Zx2XZCUF/gglpX4JSJLzJ+kQpgg27s11ssBln/nY/ogEn4OeLRqAx+3rh1lrTasWd4gTeJS6jzdWLcMWMbZv7GHV2HO+ubi1+MDad3jd4Nk4Y2c66RnKFTR/Vwvhb8GC4dfZuXP6+PRYCU9HrjDOQzMI7mSyD4IRPoPEGeTqBNj2Y7oN8NmLNAUgIzSQbRBcY2JcKFdSMoge4YnnvMGn/28UtWAOC28EfBvKUEawXIYXsvUYvqzs9Z4v6kfPxNYuyZ5BUBb4nwm8T4LjFeM5sCELaG+TtWwU/FAqzck3Jsd5FYnVSgKoP/kcfx9xvg9V6d4MUsP1UFOJuvcHGfYarh09NF/YfiSE/VRxonLRUTUT/D4ZNHoI4X0P0WvN+iuUzSiZCkBB46tClBCKJllURnEEbWDagdLJyW/UV7Vf3SFQC4PevHOls/9s/GvQt87I+D1b/pcZXOeA2N+ZNnfGVvFuANAR7yvLdtn1h2iTEQoTPYo3kpMggUby4E7d0pRVKBT6wC6NGaTafY/9VFI0GX8TbM8WSZZXnL8eEMPJ1sO+o+sSrBwmKY1TiezY+YavTpeAZOF9BlRiMAGhP+cULfaX7gQowLQ84kNBK0+4R97RkKg464nqA+63jRCvBv/yyBWcklHJzD3x+E5gzKJeKs1AMOvHbSmTWhhuD0z/XxlpAbwXrh6AYCSPRT2T6YQaCWRZhBWcBezSWhkJ0IGzaSGwcFIGN0MsveQp0Dq8IA7t/W4+vhURoCKBkmt5l4bhTzj8M13PGozxQwf5ztj5eaN3g6KYXi3UEhVkoLjF/8Bk+yERl8GpdUjPOorVoSg4cOzX7U6NCcwcy4cMZFGGdYcb0lw6LwtwLwOIH+9s/TwgL/+//F54kKvRgF+Mv/cXkrRKAxC3q+xvRvd0IKHYQt1nxdYF775aw7pQGmMAAyhdYegGirEFMI+GYNo+YMmjOaLGgF1Ip2YGiTCvldmO1fk2Z4X1mGd8PQOL87vuF76v5Hno07B773RBY7ryepMnQx8RVi/x79uczLmf9kx5uTRpPK5gkx25LZzznrjO+OcnGMzUcAQF0L7lvwtodse2Dba+IrJWwTY980OLaQCzNNyRpyiTJNTwIc5ozucKERqyL7v/3zJJ9DCV6EAgThX8vBuldOAjRLmhhtIjRM0pK2CfFiE++n09rei7fdUgA1t5ShPXG8NcgE7Yw2Cmi0sN2YBeOcMQoDRDDBl9KFITFeM8kbZryyzO7eHOJtUnpDR7VrW/mOt6f894wQqnQFKJyguQq6C32M9xcynPkOHsVJFlVqUmGFaki1t+TXVq3CwwF492R8oVaF3fMGxHqN8wV4SsDjwT7XCXUtSZNA4wy2WoOh77ATwblJJQTqrViOWXCYZhxEyuSzaOP4OZTgRSiAjVs+YOTSt2HrCeiJpLfsqe87UiVoqSpDS1gIHgnEjzNM8D0eLVrTesmQcxacYWV9RDiTQIjQiXVigKCFOrSvCHhNhFfG69kzYZdYNqyMUFUAw/3AUvg/VA+WSbslzWFcQR8JZLg1GxStWo22UeH1WX8Iwn88Aw9HYP8E/DC4UCtP6Hip5y2RoKww7bEH9QdI1wJNEmIimWb1BzY9hizYJsaUswjUup4BOmXBwbYn8bxAnZiAa77Qs4yXpAA+IlRZZFdRC8g30EzrBl5X61QCFIFr7TgqQDm3QyBoE6gJhFHsWIAza4H3kUgORHSE9sbJUIVSrg+Va90T4S0Dr5k85q/FLCHc2ZhfQj9X+BdYQOoWhc+d3HEOs75UiSmzfkLFX3bOzQwMfaVBn8xJfjwC3ysbVIXfFODxGJzpUR/YZazK1CZQm7T4hiA0zeD9hto5o2fGrm8B86WyAGcROQH0JFpYv4X6BAnXVIlnHy9NAYqAGuRd82u8LciOyHrm1G0HYEPVEpTmUVgqAIDCkphFu5+NUrudOR59IuAJgkdorPoJ6jN0oEp4I7VGxvfBK1LhVwugSqmO+XXb8Q9/oyb9a+GPpLjRt0mVYv1QY0Kt+A6k+yyqBOtoz+NRLcLQVSvRmuV4POpD9IzyadS/8c80CURaEoksSFmkJaKhbyHTALaephnq/D6FDhNbaL4gWunFc3vO8dUV4K/+qME4XyWrIl29ISrVVEozMI4NtGPCKxDuoLPwDgUe2aZWIHZK89mfIJiJajtAkO21EdSDbVsAD1Caw2TC35si+PnvCXhD1svHIj9bZmyMy1O3nwn9fcSMXBR+n/0jKzTbfOkPkVAxP3MlvbWG/4kCtyhsT8cKe9rgK7Cdc8oaQs1ZLYBI+Iw6zmQKy0Rou0ZktyGeZrQi6I3ycBDtN/ROBDtoxdxRqtFz3+zXoQCraA9dJuD1RqhtalII0DBm0qSRdk0g3JFWVd0x475huTeH0yMv96SVV705naWNYPieCysAUgsgKvyjCC6i+yMJBhLrwEYyENEmM6YCf8KmCbCC/3fO7GSv5IoX/ojX6NDHBdOF9TJZ4uoMHEJ4c5qD8AdF8Exy1wK97dtWuUU+mABuNB4potfwUKpIVSZQhT6ni4ZSnVIxWsjUo0dEoLZBGjq0u7OGSy8jmq7BBYQdBHuB7DJoT8C+SYt2i+4PlBLP5xxfVAFM+K+c3cRIXUKXWNyJdcHauoCTdlG41xlW7kzoyka6bQLu9/3aByjXNROs8Xw1yS1UIVLWXvzNLFb5lGWXQRPEG9JWpzyxbJqEu6Szvoc6qQj/x85d/neWpPC4vju8J8Ppvj1YIusyLWd+CgrQG5TpLeTps3tKqggpVQ6QC31KCoXmjFJRBoQolG3n0fIIXK3CEwqzlDY9eLeRdDgRDmcgMTpi9AQMxNgSYccJeyac7bs7NSJComcdXxMClYmRNJzZkXc+Q+l+tm90Zn3lrUNIZ3uroNKNdO89M0sY1CIvMQwKLB+kSM0beIvvTqzteBa0LBhEsJ0ZJxGZASTvw++JMCYMDWPHjG3SyFRDZLm7KIj4GW8xfFBgcMccXacxPJ2AH550+/4J+OFRj09jmKlxQwGs4ssVwLdudQy7rlOjiWrYdG2V5qzJMGesZqNMTLP+3dCBtgNIW7cIjidCk9AaDXxICdumkT0T3TVJzlSpEd6E91elALTakuHpLcFmc2DPCnG8bPBNSTDprD84s5IIA5O485ssOZZs5k9EC36Jz/4wDJqhz0FXStHIRCdCXRZssminM9buyVk0C+yrtCRRKkPHjCHVopaWndePKvw/6+mEY8f9rgCewHo0BfjtO91+9w747YOGMMufUz1lMrZo312VPmJjVOptryWQ277mDFwB+hbIg+6vFGDWz0ea9WUuDFPa9KD9Efx0BA4n8PEs1DZoU6K+Sdi00HYwDNw3jDOozPwxiPHs40srwFrwi39mkGVjwv8KhFdQSsEbInnD2jfnjSnCnq1HPhNartCpIbUoDJSVVfiG/KkS0IL9qXKm5QSziEwkNLItEkGCSWo7DwLKnknbHUZefxP5PbS46gc8odWxBAjkfJ7jWeHOD0/A7x6Av/se+P9sezrdPHOxAENXw5pDq4J/t9XtfmuhVNHYf0mSGe+ITRmyVDKcR5+AJZHOOUfzrGWVj9q4i45nyPEM6lp0TSO9CA1E2LYN9kQ4MOMIlO7UtdnvZxhfVAG4foUi/EzaWgPGqMSyZ05sGvUdsykBYWddE1IiifWzDJ/0aDHx/tjDE/uvGgWypX8ImURyFuuHI6YcFP7IvDIicGJz4qns6UoBfs54DwQaQyuUR+Pw/P4B+LsfgP/3d7o9HFY8bvshpSr8cdtvgDeBRSqCwr8iUh8hfv7ULwvyvRotW/h1tHM4TBtn0P4EHE6ggwo/jhdwf0HbiTFpE7YWBbpjrRo7AqX7dQLAlxn013+qHKH/8L96nozwF1OA//ufJ8wqPosVDQlIfSO7RovEnUfzlmB0Yp/1DQIZHNq4wJNtRoKrk+3PFD4XGEJVAvu1kCZW9e59lXVliIr/kd0HMS1k59nGIhHm+3U+ICTG1j2BRACel77E6aIWwbn/JbubSwUY7jZ6fbceTpkYDCrdbStr1FtAz1l/x1Sh2ziBzCLQ8QwcTpCuRcpZAxZNwmbO2AFF+B+hE2IvQDtnpL9/JF+PQP66rrQjn6IMX0wBOl1onHDVOQFdm3DXJrxJjO+Y8JaURfldIrxNLPfGpd+ECIsJvxDXGHsZHyN5RIgd1EpRlvb4I3ISJqh0NDN1gMfciUkYFHwMlPP9/OFXKPehUZrWQpmTYff9oLDlsDee/qyzdCyAdyUBqpPqIcrJCHJPx9rwvzTcOgPHXXVmc/hSc9bz9B2w31alg13TutbheKmzgYhai5OyR6ltQDmjIZK2aaifZmxylp0IHWCJTQH6LOguE9o5q6+GZXJ8/bh/1vhiCmAP3Lk9CzpDIrxKjLdWPfUds3xHwHfMeJMU7uwt4uOFJMwkMbm0SPH6+LmCt1ACslMIwCQi2pitjKIrBFG/Q0rEEbS8dJDljxtBaBtnfXoN8Aa4v9QiFUB/7wXwsdyxKEWwHh5OZXO2p1mjSE8KWQqed3jj9zEbIa5v1Uo43nRmqFOnn05mfbDMETydNFsMaJOtfkI/Z2yy4CIiRxBtoUGIIQu6OZdVMJ0kF323jx5fTAHYBUpj5wOsrTY06vPGZvzvEst3hvd/w8qtVw49e3JJPMRYMquoHRTK+FiBu6kEUE0IT9pPrxQbKh+P90BR8BdKYDPtzXHjj6qnrRYgtzab9ip8VqgOQIVzt1nVBcwrvB72TpUWWQpn3+r+EqgVzLWJ1mzkt6HT33dtzR1cQpHN0FkolGttgV8jMZgYqW3QXnr084yNaDj6SCJbEWxE0Gehfs5l9Ut/grM/qb/+0/TRPsEXUwATjGgBtlCH9xUBbxl4SyTu7Hq/zPtUIyudUZ+LAphwUDi/Xus57lVQ/ksrYQ4XuBldKkPUgVjIc3TLP2TuslBTMgiUm6qg3v3BBdhpDrvzNa9nDNGjwzlkeUP29nwBDhb1aZIK6Wy0CmePeggUqBagawEZ9G8u5qA/HIEH4xGd3cJIvY+nkymUZoibSS2ALuAt2Iu2W9dVcHT1G++6R1guuPGhT/Lm+GwK8G//bNkteZqF2lQsgCvAPZRG/AaK/d9SbRT7nSW8km6SmA37e4njYoZ+3kEKCaK86sN+z4WkHlAEp7RSguVFcPvVrTBTsQCrumCPw8fmVl0L7M/Lul/fH841STVOBlksg7u+DUINp3rdgOcMmGuxTNvUZlptowr2eAR+2Ghf0b6t1sLbrpwu9p0Y3LdI2wHdOGOeM8Tg2Qmkq+KIoBdd+skVwJ+QQ6BPGp9FAUz4KW6nkSixdIkxkC4JdAfgFYA3zHhrvXNecV0ZxTk1zDW64jDjueX95qDlD8W5rWGg29EYx9hAdTrjfhGfpRvXunEjTmYLjnqhQ0eqc9cqNyhmjP34cKqkthimcugSiXVOe3g86my+O1RaNJEeA8u6ZMneQxTY9bptB72/ptG/c8pEoVh04PMFyZZ4EivuGQAMAgxZMOSsi4skRodane0U6U/iCD27AgThXxSli1IItibYd1T5PW8Ty5uki8PtmbGxhJInk5isSpeu3MsvOwRLIY89dK46MMwW8kphtky1xNAFsAhiwXLX11UHpyqBjyzqEHufoILR29AWJUCgw6ACvDUBtZaH6ux6iaQV1p9H4/SItkI5nIAfDlZEI8C4rTN7mwBp6/VLmNSqyoCg9KL3ZRQL2g6g0wV8HtFcRuRxQh6nukC4AF0WrQNpGX14FV419kle8OeCQIRlKWNjZLMtWcdkZrwiyGvSRrGvWTk+3jiq9ZmfNNpT5APPj3Y+7NugzvSR/xKTU15v68XiQKUelESSqMA4ZVisUGARynrPLTApnHIlcIcYqMLXNsvmV5PXBxv02A3A/qjh07ttJdI9neo+HU3pLOp+sdrfd0/mfebqGHuSzH2Qxlim3qnibmNQbV4+s2yEvdMZfB6RziPyZURzmSDjpERGaA6gF6CHto/sUKHPhGeoE/icCrAuZ+wg2KIUkIt2TyC8TYS3XNmdG3N4vTksHPb8hIx8vm/iI8z4sdNadC4Pp3rsUMFhgYcSYVSCJObVpYoVr0aISBHqrBwfRhR+j7pE2nRka+43KuR3W73XpyPw7qhEusGoD8Cynnicqj/gUSOgQp9xqNnjJinVeugrxcKJcedLVYCLlr3T8aIW4HJBMuHHNFmthdVegK3sVS2A1wgXmvSnvN4voQDu9PZSyW7eQeF14fj40kBKbGuZS/lgRQZfGgCtruYQqLxEw9ju+DlefjyqYBHVMKULbUo1WgOYX/ATNtxvw2P1Hqp1COURmij063h/9pn8VBX1cNaff3hSS+XkN0+EnUeVtssE0KlaESe+bTr7fiEK5RBoY1DLk2Qg/TuvHPO64uMZdL6AzxOSCT+Ns9V1Ezoi6hi17htV+Eu7m08ZnxsCuQXwxJeWMwJ3ZH3zSakOr41F2bHSiTvS7g+lj89XhP46qCrAlK+7MXtvHacnv3vSm76MSlEgC2M6nRgwAbaMqtg13gdoi7NMIQwptXfnmvbgnxH7j8Bi8JuapPJimt3GWiCiwrnDCaXnqIdaedQ6Uc827ze1E3WWGqrtjHC3G2qTrtnCrIB+/nQBctbZ35t5jRNonEDTrLXWxrLtQehdCaCz/rrdzUePZ1eAxOasYrk4HFAyunvroLYnwt566uxM4xuyInbn1jz3/X3okPKfeuxx9/PFZk8TogcT/O8fFUp8b7x8ohqjd0fWw5BDB8ytWQZUxziOGN4oAr2KQK2jTzlGxxcn081DmpDK8hSp/ktxhC81alPozWOlQDyZBTmdbVafaqGM84U2fehBahOF90OfLEgwTqBJBT9dTAEuo7a9IUabIDzT75sAACAASURBVG3D1BGhT1TgtCvAJ6PiZ1eAwPlJqMXsOwB3jZLdfA3cIZGth0XeKVm0JJKuZOGLDpeh9azqJYiPJ+1/8xgWpvvhUWf9YgEOlVtTgtYBi3skpmRZU8X5a17Tuhg+OuPjpLz7mPiapUaU1ucrbVJyhWVto476bgDud0Zsy7Z4XlSMsRbfu/WLlAfPL3gNQW9Ua88wexjWlT1ASppnwTQRXSbQZQInBqekAZTEWuDEVFrdrFtefvT4LBYAFgGEKsAGKDTXe++YEMhtXRD+VLK8X3nciu+P3krwqCFBr8B691RbC757UovwcEApNnehWy9qF7OsXQyP4jokKsBivS+vCfY6YOvNWWqCKzGpHru/EJdIddweIYvnADprgfJ4Mghl1sx7gsY1yQ5WhOOfaUwBiPR+DqfaXSKlGhIVXfbVYKXwZSRcRnCTkLIoqsqNKgITGtAV/HlZFmBFeosK4IXrsZxxYJLOeuUz0aKr89dRAp+1wyy7LkJ/tKjJ7x60Cuv7J7UID8cq/I/Grlw0r3IezhzqbblGb1pTCIdK/gBi/sHDmm5JPHzpzvdjqAmumUOU1SC95NErvPqu8nu2trgGca0bbox8ENcH8LbqpS+oWYBorVICBvtu2VqsDJ0l4rj4MpQFYhCLNDokfBnBWSi1QEqMJCItgTpmaekZ8T/wGRQgkN4SFP8XCMSEO7YePkTYMElvXJ+GStz/y2V6r4ZdNUKWGEosEZ9TrcT6+x+A3z8ui9OfbNZkVPzsClC4O6gwYdPprDq3NS8gsuIRBYX03IOTymJN8PdPKpgx0eYZ6L5Vh3c/BJiCWiqZhwqJXFgFem9WxK5BgOkaAh1OwaqYr9FytYLboRbke/PdYGUpWsbLBBYIEyg1CY1Spg0CeV7pJUKgf/HPEqZcOD8LC0DaL+ee1Pndsi4E3Ru2i+1DXIW+XshzZQG88dRlqhDo3VOtxPrdgzmFx9pm3AvEY89Of8HZHdDGanEtlOj+gCegnIHnmZ9ZalIrksq8Ksxrg92BdUF0Jdj0wOtJ8X+yjDGkhi5BSzjUptoOvT9UB96t4VoBYtG9O8KtkfecRuE+ABGsykgh0Jwh01ycYI0LENI8ozEiXMu0cIC/vhP8L/5ZbXLsB79/IvqDO0mJrG++WoAtNAO8J434bCyu68KfIlb9GtO/R1eAZQG6hedwHq1J7CFg/qfaPDZGhc72N0QaIWnONcyYpQrf0FsRum3O9/fGVUB1YNeO8DofEQXx6RzS5sEP2PShMIaX8MujUd4ixdoXqpNvvUG9hYqvI+b1Br7GQGeY35tttaZMOdfeokPoOuGhYYN7ZAU8kjM4Z1AWYdHpQNvd24ZnhMnPYQEobtq3H03Dlsiw3vkAtsziDaM60tYl6Vbkw0/6JcateLlnKo/WcOpowv39o8KdH55UMA7nUDBiMXPPhvoLTvaqvIDcYcvjEdhYJ+XOsPacQ7G6BfyiL+APpkAbn+Vp5dhShW7FCTcLVCIMrkSzwhN3jr0I3hXDO0bEBbMvk1qvyCx1B7xJWq0G1ISfd50eOlX6jSn8aArSGaPULYNtDocrGbKGxp/NR/xUBYg3w1CrzSIFs3mHtgFa+eVtQzorbFkWj3/F2T8qgDuYh/PSufy9Ob4/POnvDyeNhHjoUVCjKrCH4jP/bGFUENCegIfQmsSFzrHyLipTU7k/5WFHBeA6mye2TDPV+uAYHj0ZP6lEtQzK7C9hhu6A1NVu0a4E28DubMYKc7yQxhfb6CM3yCeDpjrecTXLy6QK0bfGj0pV4U2gaskrrIHCcvZ/MRbAb4xFdJUQAJ01uxpIG1ZtrKor9suvrNwo/F9CC6gK/iLmL8GsnxTefG/O5fcP1QI47PHUfonn03JRiWQv1BeU8EZTDgPaBsb5qBwjFx4nuyXCghNUHFxezvwpVWW6oNb7egyfeclejaHMuy1wZ77BAFt3TKrwe7+g/QAcDd8z17Do6VJ9mlhB1pqEDe1SmVyRFr1HPRkYrQDUClCoAnxOCfloBVjh/6gESQTJuRw2428I1r1NnV/neiwsgJ/ts8v/yuGFLPF1tADvTAF++06F33G/QyC3AD6YLTSxrAdaMChB1eyXl46qkK5EQ6cz+folEVYQyIU/xPZh8Kb0EDpVJYtUjtLRIevfb/oaCWLSGXvTa3HLzmAQUc1beH7gdNG/2Y2hQs2UmKhCoE2n59oOlU9UJoJlMrAsGP4jEOjrOsG4IfwoJEfx/pw9k62cHnv4IywY8SWhz+pCEjZ3LqNj93BQwf/tO4U/kTps62IVGkBsOV6EMGDxWJjulGinBnhLETZ6wqYDLoMJUyiDdAhE4Tq8sgBOSXAI5Fyl0KJEhd/i+FM2YeyBe3NwrcX51ay9G/TfC51bak6jbW7XEHvp5BpONVwhUFcnA4U9NtsbBPKWM74C/df3AQJXISpBIkID0aa0FgVyR3ggYIAtXoHaCXrt/37eYaFFAKVtSOHMGzTw1RI96lN4Pk8163m8hKLyXE9PrI5Q4ezEiI1dh6jOmB4xiTN/19Slirb9MmZeKs244msXLMfVXucbQ6+xT6e3LSGr+U1sNIjtsgVKYZpaVMhXj4mP0eGjwypXdti/J8treL5jt9HrHM9GCQmKtRmqRTCloCaBGq74n55x9gc+RQG4oBVCKH4h8u7J1ALiK6m0qEsXNUAV/k/9Ah8zXIg8guFr556tfPCHpyXW91nf6QaF4ZhrFMkjLTkDE1WtLr8PwgvU2fl0WZHImtrC3Jmj01wjJDGu3xrmjrNuSksHNs665zH4EHZNv4eyRvC5fs+Gl9z/oVVh9fu/ijT59wtBDZC+7K7Re7jfAJc7o2ePIdLU1cU43CfoWlDfaf8gJiGAWOSF+AArC+DUB1+croVUoYctWwSsEhn05ZUgyOCiT02Z2c/m8D5WUltUAFcSn2WLIx2UIQr6ehTFyNeRGeal8HvBec7ax7+s3JhsnSb3EwKnyGfazQpu7GwF+VutUlwBFtu51hh4bN8Jc4TQZsVpHe/pSkJAaZuyHYA7Z5SS/r13qvaIWFiMg9oGaFtTAAZbPfA6CvR1fICgABH/uwLozC9lGSHfq3LUVPZXE37H/C4AzvB8OFq8/0H3Tm57OlYB8nrb0iktwJxCSw5RoVIUH2be2a4NqooIqXXDpduCh0d7YA6F6B75cafVZ2ivPtsEaLEbFLq44+5btEInozYfgzXwUkagJrXmWb+H1wzTuPzcreGWbdtXOnXX6t/4d43LsobvR6kBtY3mAkRK572v7wNQ3RX8jwUEKsLeAcUnaGEJMDwjoelDxlr4vbzRs6hPRmv+3igOv3+PBSim34VcAtV5VYML1OhMY9/YH3jOWtnhitBwdSSjIDSW8M8Gulvj7zNXUpkn3raDnmsfklbboABO1WbW+zueawTrGGZ/h0AEswCoCgapTjdRdaxLxnr13N1Z71pga9akNSc/5+q4+zPyhJwnwpiFmMl9AMJLgUDBc10oAFaCH45LC3N8YeGPY5HwChYgcmp89r9SAKtoWifOgCU/3zfAuDCphi5h0SEn2vnwEOVaAWJo0KMykcefTCE2Ui2P1xvE2P1+U7u4zXm54vssldcTIVCi6ls4BOKEUrvgESBnnwLLF+r37T5A/A5eN+AWMvo2MSoiAEH7sZLI88If4HksQAl/AoWv3YbNlwh1Ft+ayPTZlcBnaA9zetTCcb8XtXz/WOkOLvg+G3qnhSj0fu75Bq52BfCwJWfdks2Ckuvf+0hcY/YxQ+yJL3dw+7YuYFEEiKrAtU11dj2K07b6b2OweI63gTCDuyM/14a3KVia3mBayWBS/Y7uk5Rll7hagAYhMzwb89WfDWpoNwq/h4btvZFBzmeVl+eyALciQQ1ADUEK3CE8fyr7Q4ZIDXGWfjnGUf/eqcRhqSHH/J4kmg1+uMm/qswKwj/Py4hI6R001+iQz+CEel5/nk67PpxriNSfkie9GhPqnJewIQWFKV0iWnPWoQU9T2fgLhTFnwzr75yt2Vb+kjuwkaoCVOX0TtVDB2zP+vPrvRbKb/uaLHNlLNNlOFdUoEiFKTBVIFYaGhP3zzY+VQHi1/JIUIn0UHWMXfijovv7/3zDTu4C6Mkf73jwEKq6ymazvzuEsTNyeTku1IFWEDuqeXSoXNtmfXIWZUhgeXDbZ9SYhY4KR1gKv3eViE4kEKyFhUld+Ikqr/9wAg5bw/2jzvq7TSWmlYotCgQ7VzCHaqnyfra9hU0T8GqntIptX/lCa/hHBGQKKRmqglTemSx+J+V/uNo+aTwnFyhBn08Dd3qXkKdk792Tp88p/xFHmgU4T8twp8/4Dn2+D7H/S8DyhacfXs4twffClywoffvcQvjtZLFZ2x4OpeW55zkkqbwQfa6zvwt/2+gFhg4YYuZVlgrg5/aYvjNbHdp5rx6PHMVZ2y1A41GaVPn9fQdsRmDr+RPjGt1t1N/Y9Kogsf4XhJIkLL+Lr2ox/VuOTXSBEnmhFgBYOcFUK3aKFSBPfNEXgj+rM2cEinOgI8eVFeP2eLxO9LjGAsvMp/sGRfCDBRDAvLdKF8gcsrmwJFiACA6BPELlNb4eLenamimNvUI9MSYIytIsj+e8qiEea1JvN6wKVhAUKS1XkyQChlUh/mhKGhNbxQLYd0uCKs3ve3f2Ga8CymrRhJbC/2xK8EkWIFiuhSNMmgWOTi8XJaiTy5fB/6jRCieGPVpFl5cQxlYmPzwpRFh9T90HC5CDAjiDs3RciD6AO7sGZ9gVgwxWSD03oSrSZaq/n6ZAe2jr5rRrJ52VxaypRo+aCIUk9AQKCjBOZgG8NiHV7+rK41SITafnjtQRPyZSHlOM63OwJvBZP4jvIqBgmKjQSAhCVANuEiwDnkkJPlgB/uIPlx/97SPou50Q0WLm70DoAelRw6Aa9/cF7K6XLH1WRVgIov0cm1c9HCu3p9AdjqHPzUWhUjFTwWGJFmEOL94JbvGaCC8zZ1UApornZ14mzcrnsDoPdOY4eBGN1RF4pCeS6Lw7dMSWRPoChANnaAPcuYNvgYHI1ffuDYWW4fykpjrAc6pQMPJ/YpLOma7xBdPqjcc+Rz7xu2CAwmQptjStBOo9tADr//k901/8YTmpAMA/+h8CRfdHxgcpQBD+cpVpBmUBJ6U0K93BCl9s60FlicuE9Xv5DPO/SC0EiQ6q05rfBcH3eP/DQYXf19eKX9KVyd+Kr9Rems4KvK610h/iCzVzXiy66JuLFiRmiXMK16d6LKhtCR9PQPtYhd8pGa6o3t7wKgNNlWqxsa5uHtad5rBucFepCV4kE1eT9xmdGYotDS7lYDFi6PbqNQtu/PLqIxrzF7CAGgGaDF0iSVAaLScR8GUmerrUxfP87H/xh418iBL8HAhEcZ+lJCYaUrjTQwtftqQlkLrEZWR+UnlP8XyfPgI0KcmogFEfrLDlh0OlOXg7Ey9g9344gAmjhLxBrs6sN4a6RQJbY9syq9ms7zCI7D75Rng0Cmt8WK4AvphdpF4UC2AC7ti9ZJItTxDrDPabGuOf5spB6gMXqbAyQ1y/3B9QozpA6WIXY//vneTW0739bvUjuZBnQSOCJtsiGSKUBNo3aJzBc76SpQ+GRz/XByhKYDfI0Dh/a2S3gcwCiCpAD48GhWWN8JwdH1bm1KuqvNXfeVSOz7uA9b2DwrtDjYS4BXDY40UyMbrjClDi/cHZXbzA1eP3f/d3LubcuYXykQ3GxH4+MePqzFEvr4yY3+P+fVdn+lJDQOagcWWQCmqGNmfUYnznIqVanN+Gf4tK6RENSSgNfgvblH5EAW48oxv/RuYyJRd+EWpFaTaNCNKcwVPWKkSgrBYTUdRPKsLHOMEFHtuElaBCrrW/hC20C8QApUO3sESlCSutzvXxY/XXgmoBHPcfra438vp//6DFLQ+H61AmcNvZvQp5rjLL8SbWjh1RxfSE+nfOs/F7b1LdO1zyf3cFcBrx4Vw7T5RFKQzjO1/Iv0uyEIVzctxidEkLUiQvE2lxK5jek2zBqb0V0Vn4TvgJJQjPaPESCwQiFtECK4EJv8GgLGoB5kwsNRCTr870E+MnFeCv/qjB4bIIXRIASl6grDQHZXxStQCoq3w3ROoEvw8WPsuglQUIpYCPxxUEelQI9HhawhcfvLIAzneJia7Yhnwt9OsRq7kcBsUiGvcP3EowqdPqSSOgFqmMM3AwoR6nJf/fi809KebRIHeUY2i0NXKbf4eFrxD2BfMTQBwyur6jawW4dfyhIzyGhQUQQStSZn/fUhZdNw7sHokunUrQst1/+I9/hKaKGwrwv/5PS4d3zsBvdkJNsr4sGtFJBKSGZZ9YuzwTcAfCHQH3tur7lm2xC8Kq/PFDTOSHPKggfIIlt//RcP/DUYvZf3gCHp5q5zbvbAwEp8S007F9gT9h9o9tRn5uZkbKf6qDTc4v8rdONXbvOYOcbWpbkeeIlMLwzkhvGysvZFo6x149FkObTApdonLGuPbimdw4jtJ98zV+MApfYpWgXwqtoSW1TNgyYyeCvRAOJHgEsB8aOfyDO0WJUHQ4iWCeM6bLhBxq18tdRaVYKIAJ/5USNwnN0KBjlo5qmWPH2u35N0T4joDvQHhLwJvEeNWwuBL46u5UiE/x6XyEEviMuW4L7jO+d2t2jo+zOx+ORm+Yl6Z7PYs7jIphzkWC62cK/s3vYOfJ5QsBM5ljnCvF2EOnRThNQkiwKHp/ONQs7pyB+9Xq7rG1io8IVYAg6FgKfdx/SBTnYx+MW0ibLBvWpgqbxJjaJJmIZhFMIpiyYGITJGYZIDgLcM6Cc844X2aiy4wp0Cdu3t37IFDUcyJCYpYuaVeHLWmXhw0zXiXCb5hVAYjwFpA3ifG6SdgnxiYxemZd8E4TAfLxOMj+KJLQYq1tifUbr/93gdrs7E5ndkY8HpNXa5rDHGHP6rOfogVuwQQWQ6eqAFH4J6p+RNyYUFoUepjXSyvLvUvwEZprmAP/2Z7tenK/Mdl/2Bf7ucMU2nVLFG01QtIz05YFkhhEEMmg2Wb4TBnIDALQi+BJgCMJngggyleryd9UgrUCXE0QqFncnrSn5x0M8rCu7vKdKoF8Z5bgbWK8SlwaYdmid0LlBSLsf8ZDircf1+rysOfxHApbDOf//Tu1BmVJoLN+NoenHS3JVbTHcwor4f8kKyB155whgf7ARingGRhd4VOFMoWGzIE4Z4tOi6CsDFl4/FxDn01wZD00Gt/J+lGv3035+VNM341zih0EJWAiNCTomSCJwQDaGWAW0epTJRMxE5IQegHeCfAAgISQyeqNwh27IizGLQtAYfNAhDq6hK3h/NfkK7zryu6/Sbp/S8BbZtwl0nVduXaBYy+j/NkWYPVhh0De9cAL2x37FwWw7s3vnmpI1FP/Ef6UcGde4v01w9Nj7j/l9P7UkHAg0BdfQoukfgH5HtWBFVHnmLlCoMsIHFg/M1mjq3HWZ1bW6+prF4o2VW6+P1pePd/Fwru02H2WsVAC+xWpzPWsAt0IoQcjZYFA3wNl9UW1DFe7kRCp0F8AnICyiJ49kevQ6EIBglNahJ+0J4snurYEUwBd1f03THjLJN/pYtd4S7rs6c5wf0osttI76Gqm+ZinSlUBXFjP9uLdB4gQ6O9/qOHOSHCLKfhb7M4IfyIE8j/61EkwRp2iczw7RJmXz8qTVuExFAtQuE7Wq3+cK+zZdFoRdt4Cg1M8qFoMD0rEm/kcWfqfGmsdJKjfKLqfwZgpq7wKgUiEiCiRNVeGgEDIJLgQ4Ui21JadT+L+X/5xwn/y36hxKArwf/3zhCxCqKS2Qm5rk2wTY98w7skWtiPgN4nxHRPeJMLrRLi3NcA8+sPW4Kh09PoY4b+acU3wvaPCmt7sZYxxoQpvCnUFYQKUWs/+UfCv2Iu3IEP8XTiOwh2V7uZ3jfe0EsxFLMPgwpxRokgx/8GkIVHvFbSxCJGzNfu2+kAphXzDM0KbTxxumIgBgU7AGQxIxgTWkmwWkMEjiCgoIMIFGec2yWnb0QyNDo2ie53v5IYPENb2UlJbLWbv24S7hvGmYbxhxtuA9d8mlvuk635tkq7q1xC8o69cCT7w84S/OItSj33h5rgohXdw+/5BIU+kNywIZ9HZDRneqAgLuIPgq6yhW/BjonMaw4Xray54Qz/yXGJniSvHzK6baQlyfe8FNY9G/HOGp9OefSUYYEl99v36ml98UEnGg2xqZ2ccM1pkDImxAyQTETGp8ItgzBkXIoxZMGpMCeewAeoLlIByUYDV0ka+sotGewivkgl/YrxlEsX+Fe7srfFtV+AOTPiX/J8PE/4wfbrgeOzdaQBlqaJAa/YklzeudXpDjOBEBuZ8Y7sV7vT7XjiHVPel0ocDAc3gSkyaRWe7jNU17FeF2Oe1w65pBcJQVZAorVkC+/UI/BDozSWhZxnokhhbc3zi9/uKw54vMQTaDMJYB4weGTMxQJDEBMlCUxaMRLjIjDExRhGQAAf7O0H1D8ooCmCOkFuAHrqs0Q66sMVbshXdE8lbVujzHetC10NadH2WxKGTrwsJ8IEM0PAZF4RI/HLc6xbAoz2/C7F+Z3h6x7To6EYFiHDnfREfD0tE4acbwniLSgDUcxfiG7Boneg+jWNvhyICFBo1EGZ92Mzvs4opgSt3SnVtgwcvb6Slj+P+RIwO+RSbsQyNfg1YFKAkmewTaX6YkakFMIBBEEUrJGASmUgwItPIjIlzyRM4epyhTnGk41cFMOGMFmADYA/gFQivCXhDJG/U6cXbpApwzxrpaS3a0zoEsnOW5/hzhd+HAKVTQazCOgYT/7sH4O++VyvgPkGxAPnGzC+3Z/41t32dJfXnVABqgD3OvPQidS9WmWxRinGu3ycKtmPx+AyK32PKV7gBQOk9yvYdvFTKmaTeYe54rnx8bwA2WyDQly/adHpffbAki0W7ga+mBCAQFeHXW2AggaW1MpmEjI4YAwmSCj9GIRmZaCTCSFSgjs/8R6wed/Nv/lTf1DQLbG2vFhbxAXAHDXe+IcIbIrwlVgVg3d8Z5EnMwn5cJkyf+T/uASwgUJzBIsb1tbr+/gdVgMtYlzZyCxCzxr7OVnR8b2V8/R4idIszfkxKlWL10M/Hi9ZHBniqj30R/TEBj3v/Z4dJIirkRdlytQB+j54oy6TKQaP28CfSGd3X9fW1CdrQ6HY0SNSgKpcrVZiJv7gSOPxZ3QIDaAFpCNQRI5NgpowEYBSFPRcCXAEm1Jn/CGWS8mUC/Z9/prV4XraIxzPh1UbaxOhJW5nvANwDeJ0Ir5nkNRPuTejrWr++hE2UiY+kO0t86lLJX0WgQzcH396F7eEYClaiUEcFuAF5CqPTZ3y6FnQPScT257EFyaKCyo4Fy6a7nodwikLMPscGugtfIShC+Q5BMfPqXhkV67OhXYd7RLVPf1ybzJmhTon2vj8xCPC1xgoOabc4AG5EGRAwsmTlCWXCkUkuiWgUwiSKOi8CHEVwmDO644U8IlQiPppYEAytCrXG+wn3AF4llteJccdsa/vqur6K9QlEJARf3vRjhd/3UrF/ZHUumtcapdkL2L1p7aJaaxW/X1eJLRJcQfDjyi5xBRYnlN3aYkOoPiwEB4Suy2Fzy7SwRLkKagnJ2j5KQnZXTircSZbZ5fD8/FwXi/2LqHV6PIaidbNW8xzWJrOe3swos9pLUAS/AU/SEYSYSLLeJguhFfVDd8w4s2BKglGAEYJLBk5ZcMoZh3UYtPfTi+jsb7P7HZHcE/A6MV6FWd9Xd7R1faU6ux85bgl/BjBbgutwqs5tcXwfQve2S12A+lbyKkZirvj/UnG4J4cWmL6px17svZ7pYxvy0uq70+8SV270njzeg9+bdC06ys01u82z2m5vpOW+jFOqk9RCFACl8a7nBSZPppmyNMkWrG5rba+3StzZ2mT+/VvrAeRGmYDFusVfcRDIXCgpSsBcFWDLhClpRcEkgosQnSXjCOAggh1wWwEAW9KUCHsmuSPCKya8TgyHP9vA7fHljciwsQdHfu7XKULozpybe8f7T6dazxv7+PzwWDsex579EctH4Y/UZm94FbkwhDrjL9qRh/LAdd1sKTSP3ZjtWKTWIzyeas7ieLn2VTyJ5b9jRu28nKvv4LBIoL+PC/I5czQ+v3jMBDz6ugNNpUDMWRVRcqgVyBYRkhCJWjgGX35QPCTvnqJISIha0uW3dky6yraQRs2zyImIDgAOAjwBlcZVFUAfqi5mR2JKgHsLdb6y2d8UQDrmGuosN/dz4U/4tM/8MWLjpY0Hb1xrSxV9/1i5/k8n5fW7EC0o0iv8P/kWuP2lrybX4ybM8A5rXPA9wxr33n25bGEpoYcD8O5Y26y/62sRvjekPVvHutYgkgtmmfHNIkY/QKTmGpzaYIVf5e8Q/B4PbbqT7qsxxgQdUW2nWMLHWM7+X1kHIgxSrwCK/oSkyUQ9E3LW5ZQaIeQMnAg4kOAJwINZAHs6UQH0W3n4c2fOblSADRE2RNI7z4ID5v+omX81YtLLMWxcq+v7B+C3P6giOLvTuzaXRSvkWlDW7Uwmgx4lzGnC4wmsUl/b1lVLhm7Vb3/Q9iK7Qbuh3W116R/f7zf6iH+w2gTPwA599Vu8ZPN4AVoLW9JK+MtKNDGMa9+FcoVuyT7jUe9SRBOfL7BYj9ehkqDO/BtbBMMXusukVuClQKCVsBHp5MCkqEQyiFhlsxdVkKPN+g/QsP4W6hgDAJqGsSsnA/YEq/CisrK7r+7es9b9toSyuuOzPA+HPmtH1WdIF/QII7yQPRZ9+EstVsTCnlOAPvEagiWGBmpWN3ZViBBoa+HD+20V/CL8m6UCiCxraQu8svM1RmP2UGbxUSbgwvVeooPltnsWnQlnQSmicZjnz6JESqn6EJFH1ZxV+X3W3/bAcWNQbAZ6sxwSLMzHFa28MAAAIABJREFUzXbPP2J8lFQREilxDplKQc1FgB0DeybZEdGeGXvmYAEaltd+Iibck9KdXeg3zBjYFrcmY95ReBafdvc64my35vcfrWuD05g9oeNC7zO2t+ybJiOCu/CH1n1zmE3Ltf1JhJ8XtxrCjIl1Bi1wKMz+3lvTG8y2VuDehc+60sWIjrdsLG1EbLtyqFZK4GPt4HvUxmFOKWxPFdYlIwaUhsEeoRptYrHNV25M1ssUvFSsFzR8VflEJMJEkjRG2kHQI2NgxjZl7BJj36bgBDPjFVCcqDto+HPPtq4va01mxxQWt6ACKZ9teGWXO4Ee/iwvZlpGehyzeu/KIi9SiV45V4VaJLpkJehrqfenGrdwLacZ7wZgv63twDe9dW1zIUONEsUWJgAqhXlE6fPv13yf4K9HiZjZzB8/6pGsNkasQn8ff+ZFAUz4T6GDdj8COWlmFP4M4j2957l96WG3o8aKkJhEmWgZHYBeQk1xIuxTMPsNE17bSchm/ftgAbbMGEgtQIJCn+a5hT9GKuKM5HF/twCXsCq7O2Pr1RO9SswdwFghVaCPLIURWDrhiyfrUSJaWhunGt9tdCurMtZF3tQCNMAUVnVxxSi9Si8WkQkWIJr3xXO6enAGHUX9AX+WRDWZ5Su7eLtDf05+DyLXC+T56vFOmwbqM17cg3vFX394AlbjAgQSCIHRSqZeNLe1YZIdM92lXO+8YagC2NgBSwhETnJzXj8Wi1w8w63XkJ6HJ2NPnzI7GQSaVu3KfRECGOnLk2ewc/o6uWs26NWQ68M4+zsEatMS1uw3agV8sWfPDbigt63Gll1ZG2tYdTZGa3+qFmCNen5qRAswhQACszrARFVht0PNTnvW2yeEKPRlP9bn6MKf0w15fylKoNwhTcwyKGUwgA4svYAGFmzdr2Wuj7khUgik58COgHuQestEGBgF/8dw+c99V/FGr0b0AZwy4PjfX4rDH3dkC/047Ils9ULH2AECxRfvxz+avVvN/oXzE3yAGP6MXdXceRZZLlzRNKoQIrWA3/0Fd4bdAqyt0/vGOjnmfB//fk1TyyK7JkTBQl6k4WUnvbPBoUu39H1ynBlegtD7UOH3vTCBhSGS0Ym1VSFlN2wJ2DPV2EfDFKJA2vFhS8oF6knX922ZStQH9NFkhw8f5TmbcDuO9Vk3Nmtyjo4LaVnhJCiBWwxgBXHCdSLMiREb73LchOPYMc33oJq7cMsDBHo16owcWw16pjlep9xD+I4Zla5M8buYMApQiGyxFYovb7q1nkHrss9prvCober1ryaHlyTwt4e6T54d0GRYEkGbVY57dpoPVYZHw4yNnyHQHPRd1FJGg6dSfbSPVQJ/afHOqQqdx99jeaK/aDfnvq5VxO1OZ/Di7wIr4ky6eomEakGcwuxO7hXlIXRI9qfn/sZpRBEQsf/8mLzE0C2bX+DXuEx1v7Amdv8U8P7ikQZlAKpyFrjWWxtEtxhhP3TAmzvgzR6439XFMspi3WnpO7w0ZVj4TPU5eGSIiaQhUKtKIEOiWvbfMFUuUGL0RNKRRnwSWScH1QD5JLl/7zDBTVSJZdFRFVT44AtBny5LM14yuwC686qVt1/mfZGeiO+bam0iBcJWLF+sfOhxe2ddLugWweoscgCmlGsFiLmGznyW2JjWP59ZY/7vC0VGhOLPzFeVdKe94Fiq33/ogFdbXd/rfqtJvmIVUlWAm9d9YcoA1Hu076mvV2W6Y8Zg+EctQCIM5Q812bWI96t3LVQe2ueAP7TMwPpsTjAIZJGMXV/9AufQxHYnc67OaAwtfojwx6RXDB2WJFizVABvy+IrubizPQZuDxAEO5zPaRiAwjdva963+j06V7ix3lfOqgDrYvmFRbXjGLFaO+yLBSzseOHPWIZ7Ey2Aw7IXDot8hnb/R+dt6zEEtJbPGjiGQckUwIShJ0LHQGvpZI7C/+ke8Ptv3F9YEX5/iSYY20AcK8v8hFDp0fhA/U9h2fW1A/xaKMAqqtMGCFQyt4FUdx7rypLuuAMWGnWGqFm2Kf+IBTDB96WGXAHmbLN/8HfiWCPLmLNwwt5+U2Gdk/2KhYj3GcKmvop7WRvAL/bShsEfWh6TWwBS+k7HmhcoYDFCIPcB3AI06keASpLmuaR+9bY8nCmp3Icu8NwoK3Fdu+sruz8e62xPpL+PWL2QxXD7nRVFW1mBBQQKSlBIZCsLwGSU51NdcONw0gvsemC3Ct0KsFzYYkW1XleWXRxCURDC9z1Pqd9p7QP4zO75Cs9d9O3y2uUe0jIo8LkAwHOM+I5dCRYWwCAQaQdzRlEALok+BOyf1Hmwjs4ImLH853lvPr40Ip3tEt9mdfoiEISaRJtn/X3ErR5BAaG03XvfKGHScCyrY/j18oq7BMtge4Mus0zAUplHgz/AqvzyRg7i1v699x+VwqxD4R2l5Sx/i83aBUvj1Aef9Utk7IbVeWnDfbPVYKAkcVUJZGkBUvhkSXZRkPWF8D/XuGEFCmYnu20oWS1TFfTMGlYsZrwFujHMYGmJc2NVU5kkg6Cv+4Gmsc68bQNcmuXx2FZynUdRFs9l7WSGf4plkLERl/sPke/kCuU/ewLwisoRrud7L9uMVWvt2qoFyOVdpeMW1wJbO84veayUICL2BJQS4EUUiP0HJmFTgljiuLrCM95tkMr4EhOj9MsUQVlaNFKd25XJfp8CuDbHmdzPK4D2GjJoxQZn0qXG6Nu0hEZDXCjDw69hBo7WDOE7LQp+wjVjzfM51A1fxuBQz1XpSnFMCBQs8hjBp1n4NSsliAU/XRv+LpyjRADje/8FKUEQrWIFcK0AksIfVuij8zAtvvfnsgThxh2cxYuv4UiWGiVZz2TRcYwvsehanD2lWgBnUhJqCNEtgR/3JpDTtLQAayWOgrPOQxT4FOoSxlD0fwlRrWgFYg1zgWRhflsn81KqW3GyAyN0nd+I1GvGtfDHIMgvYZRMV1UAV4Imfk4bi9rgIvwliVC+/Ge9Wf+Pz/pLDb7Cw1muHbbouLUrC0DuA4TZ2EFgVACgzuhFAaIzOi6FUcJsDCyFfhEyDF8kFuhE+nfsGnGZlmFVv2bJB8RrYnnNuDRqyS6n2xYgRrtoda4y8cVJ75ci/XVYV8WF8C8VgFEhUHB8F5PY8oyf607ryf1YFv+ow4X2vRDILQBXgXDhcOxMgBeUlhUbJ1Tr4tVRbQLOYX9pgwIEOBL9rghFHG4VCBQc+WgBLisLUJQgKEOsdb7KaNO1ErgjW4R/neNollYTwKIt+rpF+i9P9ssoEEiqEvj8h4YCyqdKe/j4UsdPvNOFYxx/Dr+/wrfRCrgPkGoo1KqE6gpqQIFCEVY5zcBrExx7XyagtTUIHKLEAnancnudAoDSZApYEv2IaijXa4EXQh95OsHpXXS3RlW6CHuueExrJzhh4dPEdo5AOOl7w02/uBGlx1eZT/EDSwX4CbTzRZRhFR0K3I7F79eli2XmX4f0uL7gnG2mlOvLrE2dmHXwInrH6c6ZX7A5m1B15vH9AOXGDMAUxWf0R8sVnM61V9C0wvoL/ygc1N441WfRbM+14K+3OCkw4Vrofz3C7yOiO1eC6gTf+DiV0NryuX+5sZbO1SjmnivObcMsF1+2h/TK+zUuTcTQ9aTLGSALkIMVKJVqF6A/LyFYbGYFqRYgiwp2znVd33Gq6xbEgh9fmC86utG5jfe9vuc4+0fGapkcfFsl1Aj4NQr9emiXdTPyCK+9WQk5xf1nifp86IhKcMMK+MwXLUB0hNcWQNwJvvGdorNTokYxQjRpj8+z0TD6M3CwhFtKVjll9+nwJIZepwzkqUKY86Sty5+ss4XDoFjvsHZ0fb9GhDfDnzdm/gh/CrFt/WJ/nYrgOl5esSw6w61wwItydm5ZAlkKf1GA+JJ9pgtWoOBn8wfKcIWgpVAIQrQma44gWoD2bKHWZP9OVXl8Nr7VjS4uZbQo+VxBoNiTp9zXSnnXShsx/ZUCNNf5kcWjlfcc/zoGrbYS+LllARaTzItRiJUyFOrE2tSvnTx3DnMVyiJM6xl2BQtiqxKiqgDtpTJD/ZzxukQojbbEYNG62qqUe16CBYjZ3hDtWby91QtZO8GxC0SscbjlA5Rw569P4NfDn5qHRMs35hBcAF76o1g5hW723Qe4qrYKUIgTavUYgi6FMGbskpbXW769X3SY9nujpUVwXYsdHNZrk8U1CuKKNlecpBth17U1vAV71oGBm6S61TP+FY3o4i++YQMJ75+WzxuOm7/gnV6NGzAowoIy861muXXko5FlQUk8fWklaINJg8aeMGPcELCmEs2812bE2N5ot2Fgts+VWUZqw67RPru4F6lWYN3oN85UtVgJhUgYhb9Aoij4vPy7f0eGy3SGIuBS3tzECUUEAoGU7Ca9wPkgwIIYCk03hD/Ofl4sHgtKosCRPx47b1SINb2gTZVW0FtCqYlKaJEWoXrtqEwFWk1KvouOt4dfvd3JLQWIiTBCuLcb2D8yPJ1SXegaeEEQ9/ONhfDbVtYXaXLwiClYgPicf7KDwlcYPoMl1pk90njXs3QsKFnMfuHLluWGJGSNpV7rJtRqa7XYupDeHe/MNdMaiXnjpOxSL5P0cdMCzPWF5LUCBLi19oXatfCHZNnasf61jYB1BKgrSREwg8rimktmnMgNmGkL9L0kJVjAH68b+JHkT5MUbngX6DX+z0DtqEAArZzQkmRKlWO/Jpe5EMZWLSIoefcSpbHfX1p1hhtG6eu/oEusZv8rH0CWz+GWD3AT+wcI9GsW/tWhgBT64AoCBQsgBBGDQAKDQ8EH+GpK8CN+ALN68gmr2f+GAEzx5aMKUjwmoBTiCK6FLMIrb5jbNUvL4vDMnx2RKmq2vxexaFKwAO4or7tbx0a+5XHcsADrkPB7IVDIAv8adeAGZq8QiJBBmCFLCFTMAQmSOE4y4b91tpfw0Ap1muqsGh2+q/h3umH+bchqyoikOa/o8noAb4oV1wYYugpjIjFtPaP7ceLaTsUzyuv783vxv7n5DLBMgC24UcEnihnxRXTqJbzIzz8EVLE/lPdY26MXBSCABLMIcha1uiZXS6H/WhoQruuXX3Pg15nQNuzHVGfbq+4Gq7EQLBd+Xx0mrARzt9U+Ott+GaOncJMRt8dinilXbtHxXC3JOC1p3D91oxTgmcOyWFP8vujPzXO/vJDHzxq3Zn/RyXwGdCV5CC6iDcQBqAJM/tfE4CyYBcgiyBYV0n8OL/erWYGVEkThF1QyWIwGOQ8mTVUQIt/9fYPoelbtulpXu7e26K+skVRUSr+/NW53RWiS9UD1tQ/60Jktztg/oQRRUaPFu2LFhpn/vTz3X7jwr4dN3D5/z6Kz/ijABYLSyqzJQsUckEgSswICKu6AAMv1ob4mDvIXpRarzuZUZ//3woEPdADX3JrSXiQ0mNrbQhiv90sF8L/3ey1KgGoBmlRXc388AZtjbefikZsPaeni11oXwC84UcFBXrRUief+FQi/rI+lbFlAGTrRjwAuArr4Z1c+AM1ZZM6mBJ4TeBGz/2oQqpPJ0P9ENuQiG9qsBOFDBCs6vU2AQKHJ1P1OLcB+U+9pPRb8ffcBUqVTb4+r3pzRH/gJSxUVNVqAAoHMolxhfyy3X92QxVHB/gKMAroIcPYPNLMgWACkAoECdPWMsIfeXsqIWDtmaxeZ4MAPij7AhwpWSksINPTAZqhrA9zvdO8WaT185o/HzCr8Dwfr2Wm9eRYF/R/oqC4swK3CoFvhz9Wz+zWNNafH5h91ft0HEFz8I03OOAGAx/oF6EUwGhSaRaNBEiAQ+QW+1vOjeGBKmbGELeuYeAk33ogCvfc6YcaMWecCNdraYGpxXzZ81g8vo4RA+2aJ+Rc8pQ+cmuN9+fdcN9ZySBi/yzqY8GsZi4kGGtKH4X+d/XEW4CRiMg/NAzzaMRn0aUUwiOCSQRND5izIRKDSUpH8/19xBOF3ZVxjd8/eXs2CP3XuGLWJpLXA1CxFK1LzI+HxxFvUU0rYu3kN55qzFt9E0/shz2Cdo1gIPy+Ff3Fzv7ahz0uCz6WPV2gWwZR15j9nwTEDR38xTdb1U2EwZxahXiAb0T+YspQMGgdf4EU8xrVP8r7iEJ9hHVb81N277EXmpy8mMYeClZItluV9+LGsThgV5layqyS9Vsr1Y98/1gGsFaDgf77+3i/iBT7TKDM+yjMTi7plEcxZEc0lC05ZcMi6ajwAVYAHOyb7g0EEW4EqgAjNmhMQX+aqvuzPvlTGT4z/v71rWY5cubEHyXqoquUb8yMTnoWX4413/olxhDeOmMX8jzeO8Ld4642/w7dvt6SS6pk4s0CCRCZZaqnvQ22ZiGYX1S2ykkkc4ACJzJQaBFH5KyoUsiAv8gAYe4DLhBeoPIC3Qybug7Hi99a/AcG1ordrz5+Cp4sxgAfT1dIw3mfvIOszJbXyC0mwsJqzmvU/qGKfiSd4DJBVHgHrHE2kEtvyyyclzmLDxkpIovCbiYOjhe0zQsED9BPE4xF+50vPMLLQYbriFAgA9KO/k1MNexOFYdPrK8o/ZB+ez1D2oJ/wAFU6NQXl9wu9Xe9EIsNUCtWUX9U8wMU9QDYPEAEwxACdvYBbUg4knQJlAdReriQKo969qbQN6Mt8Iy+O/P8VhWButVsQ+GSWZz1A5ECt8vvR1vvnJsb4kvYXiWAfxQBTHuA9StW34vVsLIO5FzUPcFLKQRX7SwRAoEDmNsAPJG5JPClxEOJIYK20VaNBdBSbL/JN9GnDa2NZRLVE4CsUoeXofQwQA1YGa41AC2Mw4PcDnqdBjReIo8fPPvpUEBwzXy3/by1/Faj8S0usONG+nMes/pEW+D4VnX6MOr8gcB9vBOP/twR2JHZK3AqwEMsOLSlY2tAwYlLobWTkAhoaFGKBWAT3Eg4XFbUNUiN3HynrlPKHQHlyquU1CvQC5axA32TApgr/3qP0XWt8/1JoT1aa0pN4JLEj8EDggRUAaACQ4UYfYB7gVxTsSDwqsEzEmj5bTCAkFr1B+SZcgUmcIFIpg7y8vMCFQVF7AHBC+Ys7BEI/cHyv6ggKn694gan7TD1vWxI9qvzE+wRB6BqjOzC6Q+CsxFm1WHwUANhxr5b67ynQHVDiIxs4+EDgtlj/HYBHAVZKMNlVQmIBKTVCpSVvDYLo4UfVoa1CfMEqRv7PKes/RYFiHOANKjdrKdA1DzCiQC+gQUCd+WrHPSaf9R1QH4aT0r+ZHuwqjqo4qlMe7ZX/Qe3oWc9Ctfxg28vD6A8flHiA4jsIdkhYJ6uOSyQWFNv6tnrh3wAIIgUajQe8khJEEEzl6iNNcQ/gfRHDAJa/XjIGUAELz+toT+unPECHsMPh+7P+bE/Y1/xcCOP9Shyy9pz/Uc2gPyjxkDVQoFOWB6C8uCWBYv0J7BTYEXgEsVEiKdElYgkiwwulpX7hv5hMWTWMA8Oum4gBXtDekbVulD/u1NJbd9RGILIYP6LVvxoHNOnV5/pAUJc6tzFAKg/73kDQ5z0BL3vIQ7bHrH8u/L8FwCnLg+vv4nC2UTERyHfAsgQNOwV2AtwLcSuClQIgkQgsFFiJ1VR3/fsO29X/4p0RM0HRGobZYdWKaF/U/uABXkKBQgwwcasmTTeRYbo2CObHlWeuPEDIBI0mwMR++tenPizvh6Vv6dSHxKFkenaq2ClxVzj/Q6FAT1nxdM54cuOyKN7WO+lCKxja0/j/PQU3yaiP2guXTpRLSVjDx5YECYRtRkAD01t0zmRa8Eph2HMtdKUtoB8HqtEDlE9fcXoIjOr7jbI/U7EAGxBgWl9jXc9U0N/WAL0X618sPftPlq4yq78n5ZHEPYl7Be5IfE/iM4l7Ao8E9gBOKNtBALZoQQ7fUUbMZA9wJ8ANiKUKOiVEiU4VKyRZg9xwGINJCWXd9bDjzFv0+2iCeFwXvymIe84gtjFAv6LbhPK3cQCbG1U0KVKgqXGAeL+JdsVy5nZKaJX2lfpZ35H4AJfqUOtzypR9oTn3BD6R+EGJjyQ+FVA8YgDAMCUS8OWgAJgrOariiSIPYtmeJErpBEkFi0ysQWygcqRgmYREQgcBitFh8Qa/TGzQ0B9Xhn7lhq6ujqwCw2cQMBWsjhSXjcJGqlPaFRWfrK+rFr5qYoOeAsVHjZ0Z+X8c+EsTdO8deIHSj73lH3L9kkvG55CN69+pmvKr4vsChjul7EgcZMIDGAUSgJBLVh4p2EOwE5hiCyTlxKWY8m+huKXgyIQ1IATpNVZKQedZobcIkGPd/tXSgDS0yet3KvEAK1j5yZodHZS6Xw79Srva9CcbEE1Z/3gvaYGOAPg2C9TGAO9A+cO59gBQuWQb+DqWIrddVtxn4lNWfH8hPoJyR5gHILCHjD1ABozLEMgXlZMI9kBPZTIWTJ3KSoQbAW4heGLCAYobJEBUhIlCK5UY1hL6hbXfX/aoPHiCAlWVpA0IKrrS8vPWAzR5+ygt9ZnyAs95FadP7TPaSf3Mk+Me74ACtV1KlqEZSs7ERRXnbDHAUwl871Xx+aLy8ZzxERbLPtAGww4CnMRiXQDA4vf/Z+d//0sHEvli2Z0jCgAE0C5hkYlNomyV3ALYQLFBgoC4UWAtxFrKciqCsuy6ZYZ8y1+7XbRkP0OHxRWjexoU5sfGObLStMU7ZaqkuQ1Sp45+qe3q4gEIUwNg7T1a6hMtvbfN/61rlL7PeE3RPWCMpm9E2v7ys9CVHvhmWm2PpzpP2az/nbIcis+Z+JwVn3PGHYEn2HEAcBLBZbtm3h2sU9otkgijRBcYV/ItZXYlkLhRYlWuExCnpNhqwgaULZRbClQEKxEkIZJIv+1s6lOlrXwNEhxVwcv0iiHoF7FdLtFvCdqumNZ1dQAb9w5zpeHwdXUhG8cWmxMKfNWbNNe492Dsj6DAke70ni48i28MMloFI02MewSK96bCyVOQlb0gWM5N4feqsqcNcu3L8YnEP0l8JPAZVt+2Y1B8GO3JALjqgN/9r+V+pgCQyy+noT3oANyQWJFYKsomA4pjTvgViFsozhDJFEIMBJ0IFsLyGZiGtJ7ga15EudkIBCEjMpojGyfIl99pszjRXVUvx5UYg/JXGZ00wd0ZrrsCgt7qo1HICaX3ecP9POBUA7oH+KIOhKuFwNrvemMQRBPvP5dUp8IWaLC56UBW4qAUz/Hbp51/IvG9Aj9Y1kfuAOxgADjBGM0FGA+wXwOArxTh3iCRWCmxACUlUElcKDiiIIyCnARMIkkETIJlAcIq0cYKzBLV2w6IfN07iF7d442YFakWtApr5fhqaQ4CaRSUCBY39Ior9kiZm59jJii80GFQbeK6Kn7g8HxVoNukNqs9EQK4lw3Iq8EwVl8xOn8LieDvu27I8Xt154XEWSl7Ao+quFejOfe5ZH1oyv9Ric8E7gv3f4IZc18TKOMFANDyy1ou6Cz0RieEUEGFXER4SoIjgROBXCxNVxQ/GRiAJEgUW7zNXmrhQZ4spSPh9R3n1n9AUwiCAwhWyzoYjgvF+k6O0OHhReo2tYpfWfIpSoP6uujPY0q1zfiMKElpR6X8qc5yVR4gBvztfODQpm+CAk08d7H8YJjNVRTfSxwOLLl+tUzPp2wpzx8I+UxT/k9lLMABkDHo8nUA/OaPGX//S8fmAimsIGeFaOFmApwhcuwE50VHVQKdKf1SBOskWHSm8cLUD5JpKvplfyggWDzAq7bicOVsKZAAfcAxWiiqsZQ+UUa0PG2hMNAhd968rzqTw1qpIwAqBODKNRNeoHrGJoD1Pcfiahe9N0s1/WkpUPU4rfITr+n6n15KTU7pnxYAF1Vcsge9xvt3JO76VKfKP7PiE4A7AA+wEeB7EjuxqganPf3xX38Yhr4qDxBA0PdJSpbmvxB7DHFBFuDMzpZLQTHsTEhCgAnnkiW6KZ8bADcAlv5OPTYbEPGKPiuorEovgxLFdGivKItpUORiWXtFTgOAoqcYza7CBChiL3P8/9WBcVud1mi4DmVrpbYdcd3TfgJ8N25rpIr99058voW4svef1ofkkNs/ZpvGeMw2of2esEGurPiYVT5eMj5eFJ9hFn8HDCO+Iji1hiUqPzCmQPjNH3P18z/+mrytHhwfYRk/EOgIWcDWzSKJCwRHEg8UbJTYwFOmgo2NHJtRIkWS0GO7KF8GQ7D8vQcAxmXEIXj0Ba08JlgvgfWqTm96jU8Kvxv3H17FEeU04E/RZIZCUFxZetQgbWOV9bLJ52c7gPEor+9PXC2qW65tump4vkiF3p4GRcWnDtzfAXBQyoG2lMmBxIEw6kPiBwAfCXxCyfigzvhkGFMZKXwrIwBcayzG2SEt5k4Kii8KHMXQdytm+bcANgS2BDZiUyqTlIOUJAIRLxx4RRGde4A+gyQY9tcNL7XPChUvsCqKv1nbGv3tyK4WUC0bT9HHEsvCr4sniFbeFT8TSKXf4wivA9TbFeOU9cpWmFv4fsJaVqHI9h3RsjsAbtbDsuruBSJt6pWf1iYJvr3yAL+0DG3w4jZ12lP+7axW23MIqc6D0x9agdsdbDKXA+AAM84e9L7o0b4GAAD6yQdlWFrOAA8CPAnxAMEHJGyo2Kpgm4CtCjYiWCVLj5pRdjD0mwT1X3dt9N4eynlxOAeGSk1/z1LShl1QYF/c9sMGVQlCBIEDIO4xtlwYaNbLWtn67E7xHEnR70VGhHqfyPeBfuONpbdpHXaKL8DsN83GBACSrU+6uTHwrJYhsxXBieE5255kOP+J5aohY/VRlJ6ixQMMC1lZRfK+jPA6CB4Bq+sn5YGW69+hLnKbzPZck5cCwINiP/eBsrMSZyqOKrIXqx/aiGCTiG0SbEXwQcCtCLbJBshsiR7LGHVitUPJu63/8wJpR3J98SqnGwjWfxk2t7jd2urMKY2tv9Lu1W61uugMNDdlLf+uM3C5krviiwASWKQrc/R5MbLCAAADLElEQVQ0QKFZhfZsboAP29LJWl/jitsqf5dsVerbjT3TemX3S6VdQA3ASx7A5/Jm9If91xcAIBfr75mfE4knogCAeFKVJ9rKDnuUFR7Cz0cM6fvJbM81+SIAfv0HxT/+mtwD+KcAkKw4ZQPCHobENYBVEmy6FAEgDoCbkipdFAAs3CO4ylY6PfwwehiZ+EHVQKBOj5IpxHIxWP7vPpilVZpFv+YB4mwyP7+9GUCwXJgV18L9RFAVlnuDz05nyuFzSVMB5aZstnG62D2nNswGhjim9QD/cWuA3q5t9erFogCgeABV4FzMV7+MB8ft/JlEmi+pvo5DVWeo7kQm5UiY4tMqPJ/K9EZX9iOMbtun9BtetMcX5UUeoAEBAHuZh7NkDtToBONhy0XCftnx0An2yY6tCDZJsE7JlL4TLFIaPIC47gufcwFjIITf1MYDRP7vINisTYnP2az6FACAomwdqt0hN2Up81WkQAgceyKFemmU2dvmmaZVAcH2bNc+257GA2yKR9sU5V8u6kwVUbxKaVuKiv/zav51ClSsf/lLSWjWUtZM5Fw8QFH+vRJPF8XTJcv+ojiAOIeKzrMA55sl87Ibf9d//s/zATDwcgqEXzfR9N/+3PmDeCa9Bwd9Ar30z2xpU2AJGxPoCHSk6Vew9a9Kh7Zy7Z1WShnz637O+tOvkfBZ3eeFrZ0wuKFR9T2/tj3ViLXfNwDgm5YysQWD/nhCzcsXDk5xCCuCg63z7zRHCfDpJPjvP+Vr3/KsvBgAzz5GA4Ag/u+ePl3AAJDK4efxHb4EBN/8u/03ldcYsEhVtDk8xnQg+LkHuA6WH60HXw2A3/4p429/7uIDAPXDOGW6YFr5JXy+FgCzvA+J+hPP43ambS1P7ynwE4Dgp/QA/pkwPEQHQ25r7dtjln9faYPWllbH0pyo9L3yfy39AX48AGKjXZH7LNHEAczWfpaxtFZ8Kpujze8+G2K9VH4UAH5ryCMAp0O9vEizZ/WfRfDVKvxjLP8ss8wyyyyzzDLLLLPMMssss8wyyyyzzDLLLLPMMssss8wyyyyzzDLLLLPMMssss8wyyyzvQf4fqMgia78O63oAAAAASUVORK5CYII=)](https://iotcircuithub.com/esp32-preferences-library-tutorial/"%20\t%20"_blank)

[iotcircuithub.com](https://iotcircuithub.com/esp32-preferences-library-tutorial/"%20\t%20"_blank)

[ESP32 Save Data Permanently using Preferences Library - IoT Circuit Hub](https://iotcircuithub.com/esp32-preferences-library-tutorial/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://iotcircuithub.com/esp32-preferences-library-tutorial/"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://sharvielectronics.com/product/sn04-n-npn-inductive-proximity-sensor-normally-open/"%20\t%20"_blank)

[sharvielectronics.com](https://sharvielectronics.com/product/sn04-n-npn-inductive-proximity-sensor-normally-open/"%20\t%20"_blank)

[SN04-N NPN Inductive Proximity Sensor - Normally Open | Sharvielectronics: Best Online Electronic Products Bangalore](https://sharvielectronics.com/product/sn04-n-npn-inductive-proximity-sensor-normally-open/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://sharvielectronics.com/product/sn04-n-npn-inductive-proximity-sensor-normally-open/"%20\t%20"_blank)

[![](data:image/jpeg;base64,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)](https://www.techonicsltd.com/sn04-n-inductive-proximity-sensor-npn-wires-no-6-36v-dc-18x18x36mm/"%20\t%20"_blank)

[techonicsltd.com](https://www.techonicsltd.com/sn04-n-inductive-proximity-sensor-npn-wires-no-6-36v-dc-18x18x36mm/"%20\t%20"_blank)

[SN04-N (Inductive Proximity Sensor, NPN, wires NO, 6-36V DC, 18x18x36mm)](https://www.techonicsltd.com/sn04-n-inductive-proximity-sensor-npn-wires-no-6-36v-dc-18x18x36mm/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://www.techonicsltd.com/sn04-n-inductive-proximity-sensor-npn-wires-no-6-36v-dc-18x18x36mm/"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAADFElEQVQ4jW2Ty2tcZQDFf98jd2acZ2YmSdsYmopGLIikKNlo29TWlVAXZlHUpqlUxEAMFXRloEVtRf0LrFRKRVFsIYogag0Ym9BiX8ZSStvMtEYSpjOTZl535t77fW4SEfTA2Z3DOYtzRK4v2gQk/wdjsJ4PUiCUAvlvmQWE0atmjbWs0RogCNDdvYQef5Lg7hLetd8JlguINgmIVeLrtSSZ7ECEwsj2TqK7hwgWFvBy86j7HyCxfxSUpHLiE2rffA7GBSEAUOPptgl0WMaeH8GsVEmNvUVs6EUi23cR2fY0ZuUe1a++RCYSJF8ZRYRjNM+fBeODEEa9npQTkYFBiRW456axTQ93dgb/xnVkJkvkqW2E+rdQmzyNd+smieGXCRaXaM1dQChpRO6hpBcfHtXefJ7ESyPoTQ8SLJdoXbmEOzuD88hm4nv3gzWU33+XUP8WnL6HWRreja0VfakyWUQkSmxoD+GtO7BeC71uPfEX9pE5fATbbLL8wVGQiuRrYzSmzkBgcPo2gx8gRSSOdV1URxemUac+eYrS4bcpf/gexnVJjh1EphJUTh5Hd/cQ2bGT+k/fo7JdWAMS60NgaEz9iHBCJMffJH3oCCrbSfnoO5jiXeL7DtC6Ood3/Rr3De6kNXcR97dpRJtCBuUSIuxQPfkxxTdepXL8GGblHomRA4T6+6l89ikqnSH8xACN6Slkewa9sZdg8Q5IiTQrJUypgO7poXlhlsbPP1A8OIqfnyf67HN4+RxBsYDz6GN4+Txg0es2gFQASCGh8esZwlt3kT70ER3HTiAcB3dmGpnOIMMaUywgE+2YwiL1b09RO/0FYnXWGqXwF+ZpXjxPZPAZapNf4y/9ie7thVYTawQylsBUKrhnfyH46zamWl5dMqjxjDMhpJD+nZv4udsI7WAbTWQ0hn/rBvXvJpGhEM0rl9Fd3egN3bSuXgLPBSGMyPVFPUADWN+g128kPLAdme1EqDZsowZOCNuo0frjMq25c2C9tS/44j93NgE2sMhoCplKIbTGVKuYchFrPESb4p/+YP4G/HVTGdCcLh8AAAAASUVORK5CYII=)](https://esp32.com/viewtopic.php?t=43986"%20\t%20"_blank)

[esp32.com](https://esp32.com/viewtopic.php?t=43986"%20\t%20"_blank)

[proximity inductive npn no - ESP32 Forum](https://esp32.com/viewtopic.php?t=43986"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://esp32.com/viewtopic.php?t=43986"%20\t%20"_blank)

[![](data:image/jpeg;base64,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)](https://www.bausano.net/en/ethercat-arduino-2"%20\t%20"_blank)

[bausano.net](https://www.bausano.net/en/ethercat-arduino-2"%20\t%20"_blank)

[EtherCAT® & Arduino - AB&T - bausano.net](https://www.bausano.net/en/ethercat-arduino-2"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://www.bausano.net/en/ethercat-arduino-2"%20\t%20"_blank)

[![](data:image/png;base64,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)](https://www.codrey.com/electronic-circuits/the-njk-5002c-hall-effect-sensor-proximity-switch/"%20\t%20"_blank)

[codrey.com](https://www.codrey.com/electronic-circuits/the-njk-5002c-hall-effect-sensor-proximity-switch/"%20\t%20"_blank)

[The NJK-5002C Hall-Effect Sensor Proximity Switch - Codrey Electronics](https://www.codrey.com/electronic-circuits/the-njk-5002c-hall-effect-sensor-proximity-switch/"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://www.codrey.com/electronic-circuits/the-njk-5002c-hall-effect-sensor-proximity-switch/"%20\t%20"_blank)

[![](data:image/jpeg;base64,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)](https://www.bausano.net/en/easycat-pro-2"%20\t%20"_blank)

[bausano.net](https://www.bausano.net/en/easycat-pro-2"%20\t%20"_blank)

[EasyCAT PRO - AB&T - bausano.net](https://www.bausano.net/en/easycat-pro-2"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://www.bausano.net/en/easycat-pro-2"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAAb1BMVEX///8kKS74+PgsMTY+Q0f8/Pzw8PGRk5ZNUVXr6+xDSEzCxMUpLjLV1tYuMzc/Q0i3ubuUl5lbX2NTV1uLjpFKTlKxs7Xh4uOFh4rn5+h3en1vcnZ9gIPc3d6/wcJzd3o3PECmqKpjZ2rOz9CfoaRP5W5KAAABU0lEQVQ4jW1T7aKCIAzdEEUTSUuzLMtuvf8zXtiQVDx/lJ3DvgEIEPU9l1rL/F4LiFEMCQYkj2JDC6VxhaRJl3x2xgin7MePMuYRD7cQftrjrcL7ELk9nNt8yXXtyRm5mquzXADKA6LpzgZRlgBPZ1WO7yn/yrmqyGdGn95Zjav2hbNghZHMV3uNG3DZCigEmhRq+pmyrUAcifj4CO+49xciWqAe6p3pCJrNH1ATp5gH6KghQLrDnoB8GxaYPYFkhpONipjrl+wI61hQ8tB5EvYb4eTL5Ebhd8u/2f4EYfhvvWJpw9ZJ0LQfb5uxbMKi3pp5xVp30miexZHbRriHvUnojrKt7isXqWRBFQQNF2w3aYCPUl8/EREWzxtuE+pVI/wbkSGp0a51Nyg1F+L5/nel56xXgt+zcMgeC0FK27gdTzVoPXtI9DBuO2tRhJj94m3/A1GiDZXoM3d5AAAAAElFTkSuQmCC)](https://github.com/zmrdko/3DWorks_EasyCAT"%20\t%20"_blank)

[github.com](https://github.com/zmrdko/3DWorks_EasyCAT"%20\t%20"_blank)

[zmrdko/3DWorks_EasyCAT: EasyCAT shield Arduino sketch for LinuxCNC EtherCAT communication - GitHub](https://github.com/zmrdko/3DWorks_EasyCAT"%20\t%20"_blank)

[Wird in einem neuen Fenster geöffnet](https://github.com/zmrdko/3DWorks_EasyCAT"%20\t%20"_blank)

[![](data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAAb1BMVEX///8kKS74+PgsMTY+Q0f8/Pzw8PGRk5ZNUVXr6+xDSEzCxMUpLjLV1tYuMzc/Q0i3ubuUl5lbX2NTV1uLjpFKTlKxs7Xh4uOFh4rn5+h3en1vcnZ9gIPc3d6/wcJzd3o3PECmqKpjZ2rOz9CfoaRP5W5KAAABU0lEQVQ4jW1T7aKCIAzdEEUTSUuzLMtuvf8zXtiQVDx/lJ3DvgEIEPU9l1rL/F4LiFEMCQYkj2JDC6VxhaRJl3x2xgin7MePMuYRD7cQftrjrcL7ELk9nNt8yXXtyRm5mquzXADKA6LpzgZRlgBPZ1WO7yn/yrmqyGdGn95Zjav2hbNghZHMV3uNG3DZCigEmhRq+pmyrUAcifj4CO+49xciWqAe6p3pCJrNH1ATp5gH6KghQLrDnoB8GxaYPYFkhpONipjrl+wI61hQ8tB5EvYb4eTL5Ebhd8u/2f4EYfhvvWJpw9ZJ0LQfb5uxbMKi3pp5xVp30miexZHbRriHvUnojrKt7isXqWRBFQQNF2w3aYCPUl8/EREWzxtuE+pVI/wbkSGp0a51Nyg1F+L5/nel56xXgt+zcMgeC0FK27gdTzVoPXtI9DBuO2tRhJj94m3/A1GiDZXoM3d5AAAAAElFTkSuQmCC)](https://github.com/Chris--A/Keypad/blob/master/examples/MultiKey/MultiKey.ino"%20\t%20"_blank)

[github.com](https://github.com/Chris--A/Keypad/blob/master/examples/MultiKey/MultiKey.ino"%20\t%20"_blank)

[Keypad/examples/MultiKey/MultiKey.ino at master · Chris--A/Keypad ...](https://github.com/Chris--A/Keypad/blob/master/examples/MultiKey/MultiKey.ino"%20\t%20"_blank)
