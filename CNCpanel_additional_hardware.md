Die passiven Bauteile sind das unsichtbare Fundament, das für die Stabilität und Zuverlässigkeit des gesamten Systems sorgt. Ihr Fehlen ist eine der häufigsten Ursachen für schwer diagnostizierbare Fehler wie "zufällige" Neustarts, falsche Sensorwerte oder "Geister"-Tastendrücke.

Hier ist eine vollständige Liste aller passiven Bauteile, die Sie hinzufügen müssen, aufgeteilt nach ihrer Funktion, inklusive einer Erklärung, warum sie notwendig sind.

**Zusammenfassung der notwendigen passiven Bauteile**

| Bauteil                                | Wo & Wie Viele?                                                                                                                 | Funktion (Warum ist es notwendig?)                                                                                                                                                                                                                                                       |
| -------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **10kΩ Widerstände**                   | **ESP1:** 1x pro NPN-Sensor (Hall-Sensor, Probes). Zwischen Signal-Pin und 3.3V.                                                | **Pull-up für NPN-Sensoren:** NPN-Sensoren haben einen "Open-Collector"-Ausgang. Ohne diesen Widerstand "schwebt" der Pin in einem undefinierten Zustand. Der Widerstand zieht den Pin auf einen klaren HIGH-Pegel (3.3V), wenn der Sensor inaktiv ist.                                  |
| **220Ω Widerstände**                   | **ESP2:** 1x pro Zeile der LED-Matrix (insgesamt 8 Stück). In Serie zwischen dem MCP23S17-Pin und der Anoden-Leitung der Zeile. | **Strombegrenzung für LEDs:** LEDs sind keine Glühbirnen. Ohne einen Widerstand würden sie unbegrenzt Strom ziehen und sofort durchbrennen. Dieser Widerstand begrenzt den Strom auf ein sicheres Maß (ca. 15mA).                                                                        |
| **1N4148 Dioden**                      | **ESP2:** 1x pro Taster in der Tastenmatrix (insgesamt 64 Stück). In Serie mit jedem einzelnen Taster.                          | **Anti-Ghosting / N-Key Rollover:** Verhindert "Geister"-Tastendrücke. Wenn Sie drei Tasten in einem Rechteck drücken, kann ohne Dioden die vierte Ecke als gedrückt erkannt werden. Die Diode wirkt wie ein Einwegventil für den Strom und verhindert dies.                             |
| **100nF (0.1µF) Keramikkondensatoren** | 1x pro IC (2x ESP32, alle MCP23S17, alle TXS0108E). So nah wie möglich an den VCC/GND-Pins des Chips.                           | **Entkopplung / Decoupling:** Digitale Chips verursachen beim Schalten winzige, hochfrequente Stromspitzen. Diese Kondensatoren wirken als lokale Mini-Puffer, glätten diese Spikes und verhindern so Instabilitäten und Abstürze des Systems. Dies ist eine fundamentale Best Practice. |

**Detaillierte Erklärungen und Schaltskizzen**

**1. Pull-up-Widerstände für NPN-Sensoren (an ESP1)**

Jeder Ihrer NPN-Sensoren (Hall-Sensor NJK-5002C, Sonden SN04-N) benötigt einen externen Pull-up-Widerstand.

- **Schaltung:**

```mermaid
graph LR
A["+3.3V (von ESP1)"] -->|" 10kOhm "| B[ESP1 GPIO Pin]
B --> |" -- Schwarzes Kabel --+--> "| C[Sensor]
```

- **Erklärung:** Wenn der Sensor kein Metall detektiert, ist sein Ausgang hochohmig. Der 10kΩ-Widerstand zieht den GPIO-Pin des ESP32 sicher auf 3.3V (HIGH). Wenn der Sensor Metall detektiert, schaltet sein interner Transistor durch und zieht den GPIO-Pin auf GND (LOW). Dies erzeugt ein sauberes, eindeutiges digitales Signal.

**2. Strombegrenzungswiderstände für die LED-Matrix (an ESP2)**

Sie benötigen **nicht** 64 Widerstände, sondern nur 8 – einen für jede Zeile.

- **Schaltung (für eine Zeile):**

```mermaid
graph LR
A[MCP23S17 Zeilen-Pin] --> B[220 Ohm]
B --> C[Anode LED 1]
B --> D[Anode LED 2]
B --> E[Anode LED ...]
B --> F[Anode LED 8]

```

- **Erklärung:** Die Ansteuerung der Matrix erfolgt per Multiplexing. Es wird immer nur eine Spalte gleichzeitig aktiviert (über die MOSFETs). Der Strom fließt vom MCP23S17 durch den Widerstand zu den Anoden der LEDs in der aktiven Zeile. Der 220Ω-Widerstand ist ein guter Allround-Wert für 5V-Systeme, um eine gute Helligkeit bei ca. 15mA zu erreichen.

**3. Dioden für die Tastenmatrix (an ESP2)**

Dies ist entscheidend für eine zuverlässige Tastatur, die auch mehrere gleichzeitige Tastendrücke korrekt erkennt.

- **Schaltung (für einen Taster):**

```mermaid
graph LR
A[MCP23S17 Zeilen-Pin] -->|" 1N4148 Diode "| B["MCP23S17 Spalten-Pin (als Eingang mit Pull-up)"]
```

- **Erklärung:** Die Diode (z.B. 1N4148) wird in Serie zu jedem Taster geschaltet. Der Strich auf der Diode (Kathode) muss in Richtung des Zeilen-Pins zeigen. Dies stellt sicher, dass der Strom nur von der Spalte zur Zeile fließen kann, wenn eine Taste gedrückt wird, und verhindert Rückflüsse, die zu "Ghosting" führen würden.

**4. Entkopplungskondensatoren (an allen ICs)**

Dies ist nicht optional, sondern für einen stabilen Betrieb unerlässlich.

- **Schaltung:**

```mermaid
graph LR
A["VCC (3.3V oder 5V) des IC"] -->|" [C] (100nF) Keramikkondensator "| B["GND Pin des IC"]
```

- **Erklärung:** Platzieren Sie einen 100nF-Keramikkondensator so nah wie physisch möglich zwischen den VCC- und GND-Pins jedes einzelnen integrierten Schaltkreises (IC). Das gilt für beide ESP32, alle MCP23S17 und alle TXS0108E Pegelwandler.

**Optionale, aber empfohlene Bauteile**

| Bauteil                                  | Wo & Wie Viele?                                                                               | Funktion (Warum ist es nützlich?)                                                                                                                                                                                                                                                                                             |
| ---------------------------------------- | --------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **10µF - 100µF Elektrolytkondensatoren** | 1x am Ausgang jedes XL4015-Moduls (also 2 Stück).                                             | **Glättung der Versorgungsspannung:** Diese größeren Kondensatoren helfen, niederfrequente Schwankungen und Restwelligkeit von den Schaltreglern (XL4015) zu filtern und sorgen für eine noch stabilere 3.3V- und 5V-Versorgung für das gesamte System.                                                                       |
| **0.1µF (100nF) Keramikkondensatoren**   | 2x pro mechanischem Encoder (z.B. KY-040). Jeweils von Pin A nach GND und von Pin B nach GND. | **Hardware-Entprellung für Encoder:** Mechanische Encoder können stark "prellen" (elektrisches Rauschen erzeugen). Diese Kondensatoren bilden zusammen mit den internen Pull-up-Widerständen einen Tiefpassfilter, der hochfrequentes Rauschen unterdrückt und die Signalqualität für die ESP32Encoder-Bibliothek verbessert. |

In Google Sheets exportieren

Wenn Sie all diese passiven Komponenten berücksichtigen, bauen Sie eine Hardware-Basis, die robust, stabil und deutlich weniger anfällig für unerklärliche Fehler ist.

Quellen und ähnliche Inhalte

[SN04-N (Inductive Proximity Sensor, NPN, wires NO, 6-36V DC, 18x18x36mm)](https://www.techonicsltd.com/sn04-n-inductive-proximity-sensor-npn-wires-no-6-36v-dc-18x18x36mm/"%20\t%20"_blank)

[techonicsltd.com/sn04-n-inductive-proximity-sensor-npn-wires-no-6-36v-dc-18x18x36mm](https://www.techonicsltd.com/sn04-n-inductive-proximity-sensor-npn-wires-no-6-36v-dc-18x18x36mm/"%20\t%20"_blank)

[RobTillaart/MCP23017_RT: Arduino library for I2C MCP23017 16 channel port expander - GitHub](https://github.com/RobTillaart/MCP23017_RT"%20\t%20"_blank)

[github.com/RobTillaart/MCP23017_RT](https://github.com/RobTillaart/MCP23017_RT"%20\t%20"_blank)

[ESP32 Save Data Permanently using Preferences Library - IoT Circuit Hub](https://iotcircuithub.com/esp32-preferences-library-tutorial/"%20\t%20"_blank)

[iotcircuithub.com/esp32-preferences-library-tutorial](https://iotcircuithub.com/esp32-preferences-library-tutorial/"%20\t%20"_blank)

**Drehschalter**

Bei den Drehschaltern, die an die digitalen Eingänge des MCP23S17 angeschlossen werden, sind **keine externen Widerstände notwendig**.

Der Grund dafür ist, dass der MCP23S17-Chip für jeden seiner Eingänge über **interne Pull-up-Widerstände** verfügt. Diese werden direkt im Code (mcp.pinMode(pin, INPUT_PULLUP);) aktiviert. Der interne Widerstand (ca. 100 kΩ) sorgt dafür, dass der Eingangspin ein stabiles HIGH-Signal hat, solange der Schalter offen ist. Wenn der Schalter eine Position schließt und den Pin mit Masse (GND) verbindet, wird der Eingang eindeutig als LOW erkannt.

**Potentiometer**

Für die Potentiometer sind ebenfalls **keine zusätzlichen Widerstände für die Grundfunktion erforderlich**.

Ein Potentiometer ist im Grunde bereits ein verstellbarer Widerstand, der hier als Spannungsteiler verwendet wird:

- Die beiden äußeren Anschlüsse werden direkt an **3.3V** und **GND** angeschlossen.
- Der mittlere Anschluss (Schleifer) wird direkt mit dem analogen Eingang (ADC-Pin) des ESP32 verbunden.

Dadurch liefert das Potentiometer je nach Drehposition eine saubere Spannung zwischen 0V und 3.3V an den ESP32, ohne dass weitere Widerstände nötig sind.

💡 **Tipp für stabile Messwerte:** Um das Rauschen bei den Potentiometer-Messungen zu reduzieren, ist es empfehlenswert, einen kleinen **Keramikkondensator** (ca. 10 nF bis 100 nF) zwischen dem mittleren Anschluss (ADC-Pin) und GND zu schalten. Dies ist aber optional und kein Widerstand.
