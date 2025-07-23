Hier ist eine umfassende Checkliste mit allem, was Sie zur erfolgreichen Umsetzung des Projekts beachten und vorbereiten müssen, unterteilt in die Bereiche Hardware, Software und Inbetriebnahme.

**1\. Hardware-Vorbereitung und Montage**

Bevor Sie den Code aufspielen, muss die Hardware korrekt aufgebaut und konfiguriert sein.

**a) Komponentenbeschaffung:** Stellen Sie sicher, dass Sie alle benötigten Komponenten gemäß der Projektbeschreibung zur Hand haben:

- 2x ESP32 DevKitC Boards
- 1x EasyCAT PRO Shield
- 1x 12V Hutschienen-Netzteil (z.B. Meanwell, mind. 4.5A)
- 2x XL4015 DC-DC Step-Down (Buck) Konverter
- Alle benötigten Sensoren und Peripheriegeräte (Encoder, Hall-Sensor, Sonden, Potis, Taster, LEDs, Drehschalter).
- Logik-Bausteine: TXS0108E Pegelwandler, MCP23S17 I/O-Expander, IRL540N MOSFETs.
- Passive Bauteile: Pull-up-Widerstände (ca. 10kΩ) für die NPN-Sensoren, strombegrenzende Widerstände für die LEDs, Entkopplungskondensatoren (optional, aber empfohlen).
- Verbindungsmaterial: Breadboards für den Testaufbau, Jumper-Kabel, später Lochrasterplatinen oder eine gefertigte Platine für die Endmontage.

**b) Stromversorgung einrichten:** Dies ist der kritischste Schritt. Eine falsche Spannung kann die ESP32-Module zerstören.

1. Schließen Sie **nur die XL4015-Module** an das 12V-Netzteil an.
2. Stellen Sie mit einem Multimeter einen Konverter exakt auf **5.0V** und den anderen exakt auf **3.3V** ein. Beschriften Sie die Module.
3. **Trennen Sie die Stromversorgung wieder**, bevor Sie weitere Komponenten anschließen.

**c) Hardware-Adressierung der MCP23S17:** Damit die SPI-Kommunikation mit einem gemeinsamen CS-Pin funktioniert, muss jeder MCP23S17-Chip eine eindeutige Adresse erhalten.

- Verbinden Sie die Adresspins (A0, A1, A2) jedes MCP23S17-Chips nach einem festen Schema mit 5V (logisch 1) oder GND (logisch 0).
- **Beispiel:**
  - MCP_ADDR_BUTTONS (Adresse 0): A0, A1, A2 an GND.
  - MCP_ADDR_LED_ROWS (Adresse 1): A0 an 5V; A1, A2 an GND.
  - MCP_ADDR_LED_COLS (Adresse 2): A1 an 5V; A0, A2 an GND.
  - MCP_ADDR_ROT_SWITCHES (Adresse 3): A0, A1 an 5V; A2 an GND.
- Stellen Sie sicher, dass diese Hardware-Verdrahtung exakt mit den Definitionen in src/esp2/config_esp2.h übereinstimmt.

**d) Verkabelung:**

- **Gemeinsame Masse (GND):** Dies ist die häufigste Fehlerquelle. **Alle** Komponenten (12V-Netzteil, beide XL4015, beide ESP32, alle Pegelwandler, alle Peripheriegeräte) müssen mit derselben Masse (GND) verbunden sein.
- **Spannungslevel beachten:** Verbinden Sie die VCC-Pins der Komponenten mit der korrekten Spannungsschiene (3.3V oder 5V). Die ESP32-Boards und die "A"-Seite der TXS0108E-Pegelwandler benötigen 3.3V. Die MCP23S17, die LED-Matrix und die "B"-Seite der Pegelwandler benötigen 5V.
- **Pull-up-Widerstände:** Vergessen Sie nicht die externen 10kΩ Pull-up-Widerstände (von Signal auf 3.3V) für die NPN-Sensoren (Hall-Sensor, Probes) an ESP1.
- **TXS0108E OE-Pin:** Stellen Sie sicher, dass der "Output Enable" (OE) Pin der Pegelwandler wie im Code vorgesehen mit einem GPIO des ESP2 verbunden ist, um Startprobleme zu vermeiden.

**2\. Software-Einrichtung und Konfiguration**

Nachdem die Hardware steht, bereiten Sie die Software vor.

**a) PlatformIO-Projekt aufsetzen:**

1. Installieren Sie Visual Studio Code und die PlatformIO-Erweiterung.
2. Erstellen Sie ein neues, leeres PlatformIO-Projekt.
3. Kopieren Sie alle von mir bereitgestellten Dateien und Ordner (platformio.ini, src, include, data) in Ihr Projektverzeichnis und überschreiben Sie die Standarddateien.
4. Öffnen Sie das Projekt in VS Code. PlatformIO wird die in platformio.ini definierten Bibliotheken automatisch herunterladen und installieren.

**b) EasyCAT-Bibliothek manuell installieren:**

1. Laden Sie die "Library EasyCAT V 2.1 (.zip)" von der(<https://www.bausano.net/en/download-2>) herunter.
2. Erstellen Sie im Projektverzeichnis einen Ordner lib.
3. Erstellen Sie innerhalb von lib einen weiteren Ordner namens EasyCAT.
4. Entpacken Sie die Dateien EasyCAT.h und EasyCAT.cpp aus der ZIP-Datei in den Ordner lib/EasyCAT.

**c) Kritische Code-Anpassungen:** Sie **müssen** einige Werte im Code an Ihre spezifische Umgebung anpassen:

1. **MAC-Adressen ermitteln:** Laden Sie auf jeden ESP32 einen einfachen Sketch hoch, der die MAC-Adresse auf dem seriellen Monitor ausgibt (Beispiele dafür sind online leicht zu finden). Notieren Sie sich, welche Adresse zu welchem Board gehört.
2. **MAC-Adressen eintragen:** Öffnen Sie include/shared_structures.h und tragen Sie die korrekten MAC-Adressen für esp1_mac_address und esp2_mac_address ein.
3. **WLAN-Zugangsdaten:** Öffnen Sie src/esp2/config_esp2.h und tragen Sie Ihre WIFI_SSID und Ihr WIFI_PASSWORD ein.

**d) EtherCAT-Prozessdaten definieren:** Dies ist ein entscheidender Schritt für die Kommunikation mit LinuxCNC.

1. Laden Sie den **"Easy Configurator"** von der Bausano.net-Webseite herunter.
2. Starten Sie das Tool und definieren Sie Ihre Variablen (Prozessdaten), die zwischen ESP1 und LinuxCNC ausgetauscht werden sollen. Achten Sie darauf, dass die Gesamtgröße 128 Bytes Input und 128 Bytes Output nicht überschreitet.
3. Das Tool generiert zwei wichtige Dateien:
   - Eine Header-Datei (z.B. MyData.h). Kopieren Sie diese in den Ordner src/esp1/ Ihres Projekts.
   - Eine ESI-XML-Datei (z.B. MySlave.xml). Diese Datei benötigen Sie später für die Konfiguration des EtherCAT-Masters in LinuxCNC.
4. Öffnen Sie src/esp1/main_esp1.cpp und binden Sie Ihre neue Header-Datei ein (#include "MyData.h"). Passen Sie dann im loop() die Zeilen an, die auf EASYCAT.BufferIn.Cust... und EASYCAT.BufferOut.Cust... zugreifen, sodass sie Ihre selbst definierten Variablennamen verwenden.

**3\. Inbetriebnahme-Workflow (Schritt für Schritt)**

Gehen Sie methodisch vor, um Fehler leichter zu finden.

1. **ESP1 flashen:** Wählen Sie in der PlatformIO-Statusleiste die Umgebung env:esp1 aus, verbinden Sie ESP1 per USB und laden Sie den Code hoch. Öffnen Sie den seriellen Monitor und prüfen Sie die Ausgaben.
2. **ESP2 flashen:** Wählen Sie die Umgebung env:esp2, verbinden Sie ESP2 und laden Sie den Code hoch.
3. **Dateisystem für ESP2 hochladen:** Führen Sie in PlatformIO die Aufgabe "Upload Filesystem Image" für die env:esp2-Umgebung aus. Dadurch werden die index.html, style.css und script.js auf den ESP32 übertragen.
4. **System testen:**
   - Starten Sie beide ESPs. Beobachten Sie die seriellen Monitore. ESP2 sollte sich mit dem WLAN verbinden und seine IP-Adresse ausgeben. Beide sollten Meldungen über den ESP-NOW-Datenaustausch anzeigen.
   - Rufen Sie die IP-Adresse von ESP2 in einem Browser auf. Die Weboberfläche sollte erscheinen und den Status "Verbunden" anzeigen.
   - Testen Sie die Peripherie an ESP2 (Taster, Encoder). Die Live-Vorschau im Web-Interface sollte reagieren.
5. **LinuxCNC-Integration:**
   - Installieren Sie einen EtherCAT-Master für LinuxCNC (z.B. lcec).
   - Kopieren Sie die ESI-XML-Datei, die Sie mit dem Easy Configurator erstellt haben, in das Konfigurationsverzeichnis von LinuxCNC.
   - Passen Sie Ihre HAL-Datei an, um die EtherCAT-Prozessdaten (PDOs) mit den HAL-Pins von LinuxCNC zu verknüpfen.
   - Starten Sie LinuxCNC. Prüfen Sie im Terminal, ob der EtherCAT-Slave (Ihr ESP1) erkannt wird und in den OP (Operational) Zustand wechselt.
6. **End-to-End-Test:** Führen Sie einen vollständigen Funktionstest durch. Drücken Sie eine Taste und prüfen Sie, ob der entsprechende HAL-Pin in LinuxCNC reagiert. Ändern Sie einen HAL-Pin, der mit einer LED verknüpft ist, und prüfen Sie, ob die LED aufleuchtet.
