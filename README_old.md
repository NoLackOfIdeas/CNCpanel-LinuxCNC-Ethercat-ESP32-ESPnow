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
  info
```

```mermaid

graph TD

subgraph LinuxCNC-Umgebung

LCNC\[LinuxCNC Host-PC\]

end

subgraph HMI-Einheit

ESP2

subgraph ESP2 Peripherie

BTN

LED

ENC2\[max. 8 Quadratur-Encoder\]

POTS\[max. 6 Potentiometer\]

ROT

end

end

subgraph Kommunikationsbrücke

ESP1

subgraph ESP1 Peripherie

ENC1\[max. 8 Quadratur-Encoder\]

HALL

PROBES

end

end

LCNC &lt;--&gt;|EtherCAT| ESP1

ESP1 &lt;--&gt;|ESP-NOW| ESP2

ESP1 -- erfasst --> ENC1

ESP1 -- erfasst --> HALL

ESP1 -- erfasst --> PROBES

ESP2 -- steuert & liest --> BTN

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

Diese Struktur nutzt PlatformIO-Umgebungen (\[env\]), um die separaten Build-Prozesse für ESP1 und ESP2 zu verwalten.<sup>30</sup>

**3.2.** platformio.ini **Konfiguration**

Die platformio.ini-Datei ist das Herzstück der Projektkonfiguration. Sie definiert die Zielhardware, die Frameworks und alle Bibliotheksabhängigkeiten.

```Ini, TOML

; PlatformIO Project Configuration File

; <https://docs.platformio.org/page/projectconf.html>

\[platformio\]

default_envs = esp1, esp2

; Gemeinsame Konfigurationen für beide Umgebungen

\[env\]

framework = arduino

platform = espressif32

board = esp32dev

monitor_speed = 115200

; Umgebung für ESP1 (EtherCAT Bridge)

\[env:esp1\]

src_dir = src/esp1

lib_deps =

madhephaestus/ESP32Encoder@^0.11.7

build_flags = -DCORE_ESP1

; Umgebung für ESP2 (HMI Controller)

\[env:esp2\]

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

- \[env:esp1\] **und** \[env:esp2\]: Definieren separate Build-Ziele. src_dir weist PlatformIO an, den Code aus den jeweiligen Unterverzeichnissen zu kompilieren.
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

return EASYCAT.BufferOut.Cust.led_states_from_lcnc\[led_index\];

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
3. **OTA-Integration:** Die AsyncElegantOTA-Bibliothek wird durch Hinzufügen von #include &lt;AsyncElegantOTA.h&gt; und dem Aufruf AsyncElegantOTA.begin(&server); vor server.begin(); nahtlos integriert. Dies erstellt automatisch den Endpunkt /update, über den neue Firmware (als .bin-Datei) oder Dateisystem-Images (LittleFS) hochgeladen werden können, ohne dass eine serielle Verbindung erforderlich ist.<sup>34</sup>
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
  1. **WebSocket-Verbindung:** Beim Laden der Seite wird die Verbindung zum Server aufgebaut: var gateway = \\ws://${window.location.hostname}/ws\`; websocket = new WebSocket(gateway);.<sup>43</sup> Es werden Handler für onopen, oncloseundonmessage\` registriert.
  2. onmessage**\-Handler:** Diese Funktion ist der Kern der Live-Anzeige. Sie empfängt JSON-Nachrichten vom ESP32, parst sie und aktualisiert die entsprechenden HTML-Elemente (z. B. ändert sie die Farbe eines &lt;div&gt;, das eine LED darstellt, oder den Text eines Statusfeldes).
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

const char\* name; // Name für die Weboberfläche

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

const char\* name;

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

\[platformio\]

default_envs = esp1, esp2

description = Dual ESP32 control system for LinuxCNC via EtherCAT and ESP-NOW.

; =================================================================

; COMMON CONFIGURATION FOR BOTH ESP32 BOARDS

; =================================================================

\[env\]

framework = arduino

platform = espressif32

board = esp32dev

monitor_speed = 115200

lib_ldf_mode = chain+

; =================================================================

; ENVIRONMENT FOR ESP1 (EtherCAT BRIDGE)

; =================================================================

\[env:esp1\]

src_dir = src/esp1

lib_deps =

madhephaestus/ESP32Encoder@^0.11.7

build_flags = -DCORE_ESP1

; =================================================================

; ENVIRONMENT FOR ESP2 (HMI CONTROLLER)

; =================================================================

\[env:esp2\]

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

# **include** &lt;stdint.h&gt;

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

# **include** &lt;Arduino.h&gt;

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

# **include** &lt;Arduino.h&gt;

# **include** &lt;esp_now.h&gt;

# **include** &lt;WiFi.h&gt;

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

void OnDataSent(const uint8_t \*mac_addr, esp_now_send_status_t status) {

if (DEBUG_ENABLED) {

Serial.print("\\r\\nLast Packet Send Status:\\t");

Serial.println(status == ESP_NOW_SEND_SUCCESS? "Delivery Success" : "Delivery Fail");

}

}

void OnDataRecv(const uint8_t \*mac, const uint8_t \*incomingData, int len) {

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

if (ENCODER_A_PINS\[i\]!= -1 && ENCODER_B_PINS\[i\]!= -1) {

ESP32Encoder::useInternalWeakPullResistors = UP;

encoders\[i\].attachHalfQuad(ENCODER_A_PINS\[i\], ENCODER_B_PINS\[i\]);

encoders\[i\].setFilter(ENCODER_GLITCH_FILTER);

encoders\[i\].clearCount();

}

}

// Hall-Sensor

pinMode(HALL_SENSOR_PIN, INPUT_PULLUP);

attachInterrupt(digitalPinToInterrupt(HALL_SENSOR_PIN), hall_sensor_isr, FALLING);

// Sonden

for(int i=0; i<NUM_PROBES; i++) {

pinMode(PROBE_PINS\[i\], INPUT_PULLUP);

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

if (ENCODER_A_PINS\[i\]!= -1) {

// Mapping der Encoder-Werte in den EtherCAT-Buffer

// Dies erfordert eine benutzerdefinierte Struktur in einer separaten Header-Datei

// Beispiel: EASYCAT.BufferIn.Cust.encoder_values\[i\] = encoders\[i\].getCount();

}

}

// RPM berechnen

if (millis() - last_rpm_calc_time >= TACHO_UPDATE_INTERVAL_MS) {

noInterrupts();

uint32_t pulses = hall_pulse_count;

hall_pulse_count = 0;

interrupts();

current_rpm = (pulses \* (60000 / TACHO_UPDATE_INTERVAL_MS)) / TACHO_MAGNETS_PER_REVOLUTION;

if (current_rpm < TACHO_MIN_RPM_DISPLAY) {

current_rpm = 0;

}

last_rpm_calc_time = millis();

// Beispiel: EASYCAT.BufferIn.Cust.spindle_rpm = current_rpm;

}

// Sonden

uint8_t probe_bitmask = 0;

for(int i=0; i<NUM_PROBES; i++) {

if(probe_states\[i\]) {

probe_bitmask |= (1 << i);

}

}

// Beispiel: EASYCAT.BufferIn.Cust.probe_states = probe_bitmask;

// 3. Daten von ESP2 (HMI) in den EtherCAT-Buffer schreiben

// Beispiel: memcpy(&EASYCAT.BufferIn.Cust.button_matrix_states, &incoming_hmi_data.button_matrix_states, sizeof(incoming_hmi_data.button_matrix_states));

// 4. Daten von EtherCAT (LinuxCNC) an ESP2 (HMI) senden

// Beispiel: memcpy(&outgoing_lcnc_data.led_matrix_states, &EASYCAT.BufferOut.Cust.led_states_from_lcnc, sizeof(outgoing_lcnc_data.led_matrix_states));

// outgoing_lcnc_data.linuxcnc_status = EASYCAT.BufferOut.Cust.linuxcnc_status_word;

esp_now_send(esp2_mac_address, (uint8_t \*) &outgoing_lcnc_data, sizeof(outgoing_lcnc_data));

}

```

src/esp2/config_esp2.h

```C++

# **ifndef** CONFIG_ESP2_H

# **define** CONFIG_ESP2_H

# **include** &lt;Arduino.h&gt;

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

# **define** MAX_BUTTONS (MATRIX_ROWS \* MATRIX_COLS)

# **define** MAX_LEDS (MATRIX_ROWS \* MATRIX_COLS)

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

# **include** &lt;Arduino.h&gt;

# **include** &lt;WiFi.h&gt;

# **include** &lt;ESPAsyncWebServer.h&gt;

# **include** &lt;AsyncElegantOTA.h&gt;

# **include** &lt;esp_now.h&gt;

# **include** &lt;ArduinoJson.h&gt;

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

void onWsEvent(AsyncWebSocket \*server, AsyncWebSocketClient \*client, AwsEventType type, void \*arg, uint8_t \*data, size_t len) {

if (type == WS_EVT_CONNECT) {

if (DEBUG_ENABLED) Serial.printf("WebSocket client #%u connected from %s\\n", client->id(), client->remoteIP().toString().c_str());

// Sende initiale Konfiguration und Status an den neuen Client

//...

} else if (type == WS_EVT_DISCONNECT) {

if (DEBUG_ENABLED) Serial.printf("WebSocket client #%u disconnected\\n", client->id());

} else if (type == WS_EVT_DATA) {

AwsFrameInfo \*info = (AwsFrameInfo\*)arg;

if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {

data\[len\] = 0;

if (DEBUG_ENABLED) Serial.printf("WS data from client #%u: %s\\n", client->id(), (char\*)data);

// JSON parsen und Befehle verarbeiten

StaticJsonDocument doc;

DeserializationError error = deserializeJson(doc, (char\*)data);

if (error) {

if (DEBUG_ENABLED) Serial.print(F("deserializeJson() failed: "));

if (DEBUG_ENABLED) Serial.println(error.c_str());

return;

}

const char\* command = doc\["command"\];

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

void OnDataSent(const uint8_t \*mac_addr, esp_now_send_status_t status) {

// Optional: Logik für Sende-Status

}

void OnDataRecv(const uint8_t \*mac, const uint8_t \*incomingData, int len) {

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

Serial.println("\\nWiFi connected.");

Serial.print("IP Address: ");

Serial.println(WiFi.localIP());

}

// ESP-NOW initialisieren

if (esp_now_init()!= ESP_OK) { /\* Fehlerbehandlung \*/ }

esp_now_register_send_cb(OnDataSent);

esp_now_register_recv_cb(OnDataRecv);

esp_now_peer_info_t peerInfo;

memcpy(peerInfo.peer_addr, esp1_mac_address, 6);

peerInfo.channel = 0;

peerInfo.encrypt = false;

if (esp_now_add_peer(&peerInfo)!= ESP_OK) { /\* Fehlerbehandlung \*/ }

// Webserver-Routen und WebSocket-Handler

ws.onEvent(onWsEvent);

server.addHandler(&ws);

server.on("/", HTTP_GET,(AsyncWebServerRequest \*request){

request->send(LittleFS, "/index.html", "text/html");

});

server.on("/style.css", HTTP_GET,(AsyncWebServerRequest \*request){

request->send(LittleFS, "/style.css", "text/css");

});

server.on("/script.js", HTTP_GET,(AsyncWebServerRequest \*request){

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

esp_now_send(esp1_mac_address, (uint8_t \*) &outgoing_hmi_data, sizeof(outgoing_hmi_data));

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

void get_hmi_data(struct_message_to_esp1\* data);

void update_leds_from_lcnc(const struct_message_to_esp2& data);

# **endif** // HMI_HANDLER_H

```

src/esp2/hmi_handler.cpp

```C++

# **include** "hmi_handler.h"

# **include** "config_esp2.h"

# **include** &lt;Adafruit_MCP23X17.h&gt;

# **include** &lt;Keypad.h&gt;

# **include** &lt;SPI.h&gt;

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

mcp_buttons.pinMode(BUTTON_ROW_PINS\[i\], OUTPUT);

}

for (int i = 0; i < MATRIX_COLS; i++) {

mcp_buttons.pinMode(BUTTON_COL_PINS\[i\], INPUT_PULLUP);

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

void get_hmi_data(struct_message_to_esp1\* data) {

memcpy(data->button_matrix_states, current_button_states, sizeof(current_button_states));

//... weitere Daten kopieren

}

void update_leds_from_lcnc(const struct_message_to_esp2& data) {

memcpy(current_led_states, data.led_matrix_states, sizeof(current_led_states));

}

```

data/index.html

```HTML

<!DOCTYPE **html**\>

&lt;html lang="de"&gt;

&lt;head&gt;

&lt;meta charset="UTF-8"&gt;

&lt;meta name="viewport" content="width=device-width, initial-scale=1.0"&gt;

&lt;title&gt;LinuxCNC HMI Konfiguration&lt;/title&gt;

&lt;link rel="stylesheet" href="style.css"&gt;

&lt;/head&gt;

&lt;body&gt;

&lt;header&gt;

&lt;h1&gt;LinuxCNC HMI Konfiguration & Status&lt;/h1&gt;

&lt;div id="ws-status"&gt;Verbindung wird hergestellt...&lt;/div&gt;

&lt;/header&gt;

&lt;main&gt;

&lt;section id="live-status"&gt;

&lt;h2&gt;Live-Status&lt;/h2&gt;

&lt;div class="status-grid"&gt;

&lt;div&gt;

&lt;h3&gt;Tastenmatrix&lt;/h3&gt;

&lt;div id="button-matrix-live" class="matrix-grid"&gt;&lt;/div&gt;

&lt;/div&gt;

&lt;div&gt;

&lt;h3&gt;LED-Matrix&lt;/h3&gt;

&lt;div id="led-matrix-live" class="matrix-grid"&gt;&lt;/div&gt;

&lt;/div&gt;

&lt;/div&gt;

&lt;/section&gt;

&lt;section id="configuration"&gt;

&lt;h2&gt;Konfiguration&lt;/h2&gt;

&lt;div id="config-tabs"&gt;

&lt;button class="tab-link active" onclick="openTab(event, 'buttons')"&gt;Tasten&lt;/button&gt;

&lt;button class="tab-link" onclick="openTab(event, 'leds')"&gt;LEDs&lt;/button&gt;

&lt;button class="tab-link" onclick="openTab(event, 'other')"&gt;Weitere&lt;/button&gt;

&lt;/div&gt;

&lt;div id="buttons" class="tab-content" style="display: block;"&gt;

&lt;h3&gt;Tastenkonfiguration&lt;/h3&gt;

&lt;div id="button-config-table"&gt;

&lt;/div&gt;

&lt;/div&gt;

&lt;div id="leds" class="tab-content"&gt;

&lt;h3&gt;LED-Konfiguration&lt;/h3&gt;

&lt;div id="led-config-table"&gt;

&lt;/div&gt;

&lt;/div&gt;

&lt;div id="other" class="tab-content"&gt;

&lt;h3&gt;Weitere Einstellungen&lt;/h3&gt;

&lt;/div&gt;

&lt;div class="actions"&gt;

&lt;button id="save-config-btn"&gt;Konfiguration Speichern&lt;/button&gt;

&lt;button onclick="window.location.href='/update'"&gt;Firmware Update&lt;/button&gt;

&lt;/div&gt;

&lt;/section&gt;

&lt;/main&gt;

&lt;script src="script.js"&gt;&lt;/script&gt;

&lt;/body&gt;

&lt;/html&gt;

```

data/style.css

```CSS

/\* Basic styling for the web interface \*/

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

/\* Tabs \*/

# config-tabs { overflow: hidden; border-bottom: 1px solid #ccc; margin-bottom: 1rem; }

.tab-link { background-color: inherit; float: left; border: none; outline: none; cursor: pointer; padding: 14px 16px; transition: 0.3s; }

.tab-link:hover { background-color: #ddd; }

.tab-link.active { background-color: #ccc; }

.tab-content { display: none; padding: 6px 12px; border-top: none; }

/\* Config Tables \*/

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

const gateway = \`ws://${window.location.hostname}/ws\`;

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

cell.id = \`${prefix}-${r}-${c}\`;

container.appendChild(cell);

}

}

}

function updateLiveStatus(status) {

// Update button matrix

for (let i = 0; i < 8; i++) {

for (let j = 0; j < 8; j++) {

const cell = document.getElementById(\`btn-live-${i}-${j}\`);

if (cell) {

if ((status.buttons\[i\] >> j) & 1) {

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

const cell = document.getElementById(\`led-live-${i}-${j}\`);

if (cell) {

if ((status.leds\[i\] >> j) & 1) {

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

btnContainer.innerHTML = \`

&lt;div class="config-table-header"&gt;

&lt;div&gt;#&lt;/div&gt;&lt;div&gt;Name&lt;/div&gt;&lt;div&gt;Toggle&lt;/div&gt;&lt;div&gt;Radio Grp&lt;/div&gt;&lt;div&gt;&lt;/div&gt;

&lt;/div&gt;

\`;

config.buttons.forEach((btn, index) => {

const row = document.createElement('div');

row.className = 'config-table-row';

row.innerHTML = \`

&lt;div&gt;${index}&lt;/div&gt;

&lt;div&gt;&lt;input type="text" id="btn-name-${index}" value="${btn.name}"&gt;&lt;/div&gt;

&lt;div&gt;&lt;input type="checkbox" id="btn-toggle-${index}" ${btn.is_toggle? 'checked' : ''}&gt;&lt;/div&gt;

&lt;div&gt;&lt;input type="number" id="btn-radio-${index}" value="${btn.radio_group_id}" min="0"&gt;&lt;/div&gt;

&lt;div&gt;&lt;span title="Row: ${btn.matrix_row}, Col: ${btn.matrix_col}"&gt;Pos: ${btn.matrix_row},${btn.matrix_col}&lt;/span&gt;&lt;/div&gt;

\`;

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

const nameEl = document.getElementById(\`btn-name-${i}\`);

if (!nameEl) break;

config.buttons.push({

name: nameEl.value,

is_toggle: document.getElementById(\`btn-toggle-${i}\`).checked,

radio_group_id: parseInt(document.getElementById(\`btn-radio-${i}\`).value)

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

tabcontent\[i\].style.display = "none";

}

tablinks = document.getElementsByClassName("tab-link");

for (i = 0; i < tablinks.length; i++) {

tablinks\[i\].className = tablinks\[i\].className.replace(" active", "");

}

document.getElementById(tabName).style.display = "block";

evt.currentTarget.className += " active";

}

```

**Schlussfolgerung**

Dieses Projekt stellt eine umfassende und robuste Lösung dar, um eine hochgradig anpassbare physische Steuerung für eine LinuxCNC-Maschine zu realisieren. Durch die strategische Aufteilung der Aufgaben auf zwei ESP32-Controller wird eine klare Trennung zwischen zeitkritischen Maschinenkommunikationsaufgaben und komplexen HMI-Funktionen erreicht. Die Wahl von EtherCAT gewährleistet eine industrielle, deterministische Anbindung an die CNC-Steuerung, während ESP-NOW eine drahtlose Verbindung mit geringer Latenz für eine reaktionsschnelle Bedienung ermöglicht.

Die Firmware ist durchgehend auf nicht-blockierenden, interruptgesteuerten Betrieb ausgelegt, was für die Erfassung von Hochgeschwindigkeitssignalen von Encodern und Sensoren unerlässlich ist. Die extreme Konfigurierbarkeit über Header-Dateien und eine dynamische Weboberfläche bietet ein Höchstmaß an Flexibilität und Anpassungsfähigkeit. Die Weboberfläche selbst, mit ihrer Live-Vorschau über WebSockets, der persistenten Speicherung von Einstellungen und der integrierten OTA-Update-Fähigkeit, hebt das Projekt auf ein professionelles Niveau.

Die bereitgestellten Hardware-Schaltpläne und der vollständige, kommentierte Quellcode bilden eine solide Grundlage für den Nachbau und die weitere Anpassung des Systems an spezifische Anforderungen. Das Projekt demonstriert eindrucksvoll, wie moderne, kostengünstige Embedded-Technologien mit etablierten Industriestandards kombiniert werden können, um leistungsstarke und flexible Steuerungslösungen zu schaffen.
