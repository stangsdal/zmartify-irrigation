# Zmartify Irrigation Controller

# Complete Wiring Diagram and Connection Schedule

**Status:** Draft for hardware integration and bench verification  
**Target:** Zmartify irrigation controller using ESP32-S3, 16 relay outputs, 24 VAC valves, two flow meters, one analog pressure sensor and four DS18B20 One-Wire temperature sensors  
**Important:** The 230 V AC part must be installed and verified by a qualified electrician. Keep mains wiring physically separated from all SELV, sensor and logic wiring.

---

## 1. Hardware baseline

| Function | Selected hardware |
|---|---|
| Controller/display | Waveshare ESP32-S3 7-inch touch display board / ZIC-S3 target |
| 5 V DC supply | Mean Well HDR-30-5, 5 V DC, 6 A |
| Valve transformer | Hager ST315, 230 V AC to 24 V AC, 60 VA |
| Relay outputs | Two HL-58S V1.2 active-low 8-relay boards |
| GPIO expansion | MCP23017, I2C address `0x20` |
| Relay input drivers | Two ULN2803A driver arrays, one per 8-relay board |
| Pressure ADC | ADS1115, I2C address `0x48`, powered from 3.3 V |
| Pressure sensor | Three-wire 5 V, 0-10 bar, analog output |
| Temperature sensors | Four powered DS18B20, shared One-Wire bus |
| Flow meters | Two Hall-effect pulse-output meters |
| Valve outputs | 15 irrigation zones plus one master valve |

The current firmware assigns I2C to GPIO 8/9 and the two flow inputs to GPIO 4/5. The existing HAL defines the ADS1115 at address `0x48`, flow meter 0 on GPIO 4 and flow meter 1 on GPIO 5. A dedicated One-Wire GPIO still has to be selected and added to the board pin map.
<!-- @~≥±+=≠≠·newpage -->
---

## 2. Complete system overview

```text
                          230 V AC SUPPLY
                       L         N         PE
                       |         |          |
                 +-----+---------+----------+--------------------+
                 |     Main isolator / suitable branch protection |
                 +------------------+-----------------------------+
                                    |
                  +-----------------+------------------+
                  |                                    |
                  v                                    v
       +-----------------------+            +----------------------+
       | Mean Well HDR-30-5    |            | Hager ST315          |
       | 230 VAC -> 5 VDC      |            | 230 VAC -> 24 VAC    |
       | 5 V, 6 A              |            | 60 VA safety trafo   |
       +-----------+-----------+            +----------+-----------+
                   | 5V / 0V                           | 24VAC-A/B
                   |                                    |
         +---------+------------------+                 |
         |                            |                 |
         v                            v                 v
 +---------------+          +----------------+   +------------------+
 | ESP32-S3      |          | Relay boards   |   | 24 VAC valve bus |
 | display board |          | 2 x 8 channels |   +---------+--------+
 +-------+-------+          +--------+-------+             |
         |                           |                     |
         | I2C / GPIO               | relay contacts      |
         v                           v                     v
 +---------------+          +----------------+   +------------------+
 | MCP23017      |--------->| ULN2803A x 2  |   | Zone valves 1-15 |
 +---------------+          +----------------+   | Master valve 16  |
         |
         +------------------ I2C -------------------------------+
         |                                                       |
         v                                                       v
 +---------------+                                      +---------------+
 | ADS1115       |<---- pressure sensor analog          | Other I2C     |
 +---------------+                                      | peripherals   |

 ESP32 GPIO One-Wire ---- 4 x DS18B20
 ESP32 GPIO4 / PCNT0 ---- Flow meter 1 input circuit
 ESP32 GPIO5 / PCNT1 ---- Flow meter 2 input circuit
```

---

## 3. Mains and power distribution

### 3.1 230 V AC input

Recommended topology:

```text
230 V L ---- main isolator / protective device ----+---- HDR-30-5 L
                                                   +---- ST315 primary L

230 V N -------------------------------------------+---- HDR-30-5 N
                                                   +---- ST315 primary N

PE ----------------------------------------------------- enclosure PE terminal
```

Requirements:

- Use touch-safe DIN terminals.
- Keep 230 V wiring in its own wiring duct.
- Maintain physical separation from 24 VAC, 5 V and signal wiring.
- Bond a metal enclosure and DIN rail where required.
- Use appropriate branch protection and upstream RCD protection.
- Do not bond the isolated 24 VAC secondary to PE unless specifically required by the final electrical design.

### 3.2 5 V DC distribution

```text
HDR-30-5 +V ---- fused 5 V distribution ----+---- ESP32-S3 board 5 V input
                                            +---- Relay board 1 VCC/JD-VCC
                                            +---- Relay board 2 VCC/JD-VCC
                                            +---- Pressure sensor V+
                                            +---- Optional 5 V flow-meter supply

HDR-30-5 -V -------------------------------+---- ESP32 GND
                                            +---- MCP23017 GND
                                            +---- ULN2803A GND
                                            +---- Relay board GND
                                            +---- ADS1115 GND
                                            +---- Pressure sensor GND
                                            +---- Flow-meter GND, when required
```

Use separate fused branches for the controller and relay boards where practical. The ESP32/display branch should not share a thin daisy-chained supply wire with all relay coils.

### 3.3 3.3 V logic rail

Use the ESP32 board's regulated 3.3 V output for low-current logic only:

```text
ESP32 3V3 ---- MCP23017 VDD
           +-- ADS1115 VDD
           +-- DS18B20 VDD x 4
           +-- One-Wire 4.7 kOhm pull-up
           +-- Flow input pull-ups where open collector is used
```

Do not power relay coils or long external sensor cables from the 3.3 V rail.

---

## 4. I2C bus

The current firmware pin allocation is:

```text
ESP32 GPIO8  = I2C SDA
ESP32 GPIO9  = I2C SCL
```

Connection:

```text
ESP32 GPIO8 SDA ----+---- MCP23017 SDA
                    +---- ADS1115 SDA

ESP32 GPIO9 SCL ----+---- MCP23017 SCL
                    +---- ADS1115 SCL

ESP32 3V3 ----------+---- MCP23017 VDD
                    +---- ADS1115 VDD

ESP32 GND ----------+---- MCP23017 GND
                    +---- ADS1115 GND
```

### I2C addresses

| Device | Address |
|---|---:|
| MCP23017 | `0x20` with A0=A1=A2=GND |
| ADS1115 | `0x48` with ADDR=GND |

Use one set of I2C pull-ups to 3.3 V, normally 4.7 kOhm on SDA and SCL. Do not fit multiple strong pull-up networks in parallel without checking the resulting resistance.

The COM-KY051VT is not required in this arrangement because all I2C devices operate at 3.3 V.

---

## 5. Relay control wiring

### 5.1 MCP23017 to ULN2803A

One MCP23017 provides 16 output bits:

```text
MCP23017 GPA0..GPA7 ---- ULN2803A #1 inputs 1..8
MCP23017 GPB0..GPB7 ---- ULN2803A #2 inputs 1..8
```

ULN2803A connections:

```text
ULN2803A GND/COM logic ground ---- system 0 V
ULN2803A OUT1..OUT8 ------------ relay-board IN1..IN8
```

The ULN2803A outputs are open-collector sinks. This suits active-low relay inputs when the relay board internally pulls each input high.

Before final wiring, verify the HL-58S board terminals and jumpers. Some versions have separate `VCC` and `JD-VCC` supplies and optocouplers; others share one common ground. Do not assume galvanic isolation merely because optocouplers are fitted.

### 5.2 Relay channel allocation

| Relay | Function |
|---:|---|
| 1 | Zone 1 |
| 2 | Zone 2 |
| 3 | Zone 3 |
| 4 | Zone 4 |
| 5 | Zone 5 |
| 6 | Zone 6 |
| 7 | Zone 7 |
| 8 | Zone 8 |
| 9 | Zone 9 |
| 10 | Zone 10 |
| 11 | Zone 11 |
| 12 | Zone 12 |
| 13 | Zone 13 |
| 14 | Zone 14 |
| 15 | Zone 15 |
| 16 | Master valve |

### 5.3 Fail-safe state

All relay channels must be OFF during:

- reset,
- bootloader operation,
- firmware update,
- brownout,
- I2C failure,
- watchdog reset.

Fit pull-down resistors on the ULN2803A inputs if needed so the drivers remain off while the MCP23017 is resetting.

---

## 6. 24 VAC valve wiring

Use the Hager ST315 secondary as an isolated 24 VAC source.

Define:

```text
24VAC-A = switched feed
24VAC-B = common return
```

### 6.1 Relay contact wiring

```text
ST315 24VAC-A ---- secondary fuse ---- common feed bus
                                          |
                                          +---- Relay 1 COM
                                          +---- Relay 2 COM
                                          +---- ...
                                          +---- Relay 16 COM

Relay 1 NO  ------------------------------ Zone 1 valve wire
Relay 2 NO  ------------------------------ Zone 2 valve wire
...
Relay 15 NO ------------------------------ Zone 15 valve wire
Relay 16 NO ------------------------------ Master valve wire

ST315 24VAC-B ---------------------------- Common wire to all valves
```

Use normally-open relay contacts so all valves close when controller power is removed.

### 6.2 Valve suppression

24 VAC solenoids are inductive. Use an AC-compatible suppression method, for example:

- appropriately rated bidirectional TVS,
- appropriately rated MOV,
- or an RC snubber.

Do not install an ordinary DC flyback diode across an AC solenoid.

### 6.3 Transformer loading

Firmware should normally activate:

- master valve, plus
- one irrigation zone.

If simultaneous zones are later enabled, recalculate transformer VA loading using the exact inrush and holding current of each installed solenoid.

---

## 7. Four DS18B20 One-Wire temperature sensors

Use powered three-wire mode.

### 7.1 Shared bus

```text
ESP32 3V3 --------------------------------+---- DS18B20 #1 VDD
                                           +---- DS18B20 #2 VDD
                                           +---- DS18B20 #3 VDD
                                           +---- DS18B20 #4 VDD

ESP32 GND --------------------------------+---- DS18B20 #1 GND
                                           +---- DS18B20 #2 GND
                                           +---- DS18B20 #3 GND
                                           +---- DS18B20 #4 GND

ESP32 ONE_WIRE_GPIO ----+----------------- DS18B20 #1 DATA
                         +----------------- DS18B20 #2 DATA
                         +----------------- DS18B20 #3 DATA
                         +----------------- DS18B20 #4 DATA

ESP32 3V3 ---- 4.7 kOhm ---- ONE_WIRE_GPIO
```

### 7.2 Recommended field wiring

- Prefer a daisy-chain or trunk topology.
- Use DATA/GND as a twisted pair.
- Use the third conductor for 3.3 V.
- Connect cable shield at the controller end only.
- Add 100 Ohm series resistance and low-capacitance ESD protection near the controller input if cables leave the enclosure.

### 7.3 GPIO assignment

A One-Wire GPIO is not currently defined in the examined HAL pin map. Select a GPIO that is not used by:

- RGB display,
- touch controller,
- TF-card,
- USB,
- I2C,
- flow inputs,
- boot strapping.

Add the final pin as a single board-level definition, for example:

```c
#define ZIC_ONEWIRE_PIN  <verified_free_gpio>
```

Do not choose the GPIO from generic ESP32-S3 availability alone; verify it against the exact Waveshare board schematic.

---

## 8. Two flow meters

The current firmware allocation is:

```text
Flow meter 1 pulse -> ESP32 GPIO4 / PCNT unit 0
Flow meter 2 pulse -> ESP32 GPIO5 / PCNT unit 1
```

### 8.1 Preferred open-collector wiring

For an NPN/open-collector pulse output:

```text
Flow meter V+ ---- manufacturer-specified supply
Flow meter GND --- system GND
Flow pulse --------+---- 1 kOhm ---- ESP32 GPIO4 or GPIO5
                   |
ESP32 3V3 -- 4.7 kOhm pull-up

At ESP32 side after series resistor:
GPIO ---- 10 nF ---- GND
```

The 10 nF capacitor is a starting value only. Verify that the RC filter does not suppress pulses at the maximum expected flow rate.

### 8.2 5 V push-pull output

Do not connect 5 V directly to the ESP32 GPIO. Use either:

```text
Flow pulse ---- 10 kOhm ----+---- ESP32 GPIO
                             |
                            20 kOhm
                             |
                            GND
```

or a protected Schmitt-trigger level interface.

The COM-KY051VT may work for low-frequency logic, but it is optimized for open-drain buses. A divider or Schmitt-trigger is generally more predictable for a 5 V push-pull Hall sensor.

### 8.3 12/24 V pulse output

Use an optocoupler or protected industrial digital-input circuit. Do not use the COM-KY051VT directly at 12 or 24 V.

### 8.4 Separate calibration

Each meter must have its own calibration:

```text
flow_meter_1.ml_per_pulse
flow_meter_2.ml_per_pulse
```

Do not use one global calibration factor unless both meters are exactly the same model and have been individually verified.

---

## 9. 0-10 bar pressure sensor and ADS1115

### 9.1 Power

```text
HDR-30-5 5 V ---- pressure sensor V+
System GND ------- pressure sensor GND
```

### 9.2 Analog signal conditioning

Because the precise analog output range is not confirmed, design for up to 5 V output.

```text
Pressure sensor OUT ---- 10 kOhm ----+---- ADS1115 A0
                                      |
                                     15 kOhm
                                      |
                                     GND

ADS1115 A0 ---- 100 nF ---- GND
```

Divider ratio:

```text
V_ADC = V_SENSOR x 15 / (10 + 15)
V_ADC = V_SENSOR x 0.60
```

Therefore:

| Sensor output | ADS1115 input |
|---:|---:|
| 0.5 V | 0.30 V |
| 4.5 V | 2.70 V |
| 5.0 V | 3.00 V |

Use 0.1% resistors for consistent calibration.

### 9.3 ADS1115 wiring

```text
ADS1115 VDD  ---- ESP32 3V3
ADS1115 GND  ---- system GND
ADS1115 SDA  ---- ESP32 GPIO8
ADS1115 SCL  ---- ESP32 GPIO9
ADS1115 ADDR ---- GND   (address 0x48)
ADS1115 A0   ---- conditioned pressure signal
```

### 9.4 Sensor decoupling

At the pressure-sensor connector:

```text
5 V ---- 100 nF ---- GND
5 V ---- 10 uF  ---- GND
```

### 9.5 Calibration

If later measurement confirms 0.5-4.5 V output:

```text
0 bar  = 0.30 V at ADS1115
10 bar = 2.70 V at ADS1115

pressure_bar = (V_ADC - 0.30) x 10 / 2.40
```

If it is 0-5 V:

```text
0 bar  = 0.00 V at ADS1115
10 bar = 3.00 V at ADS1115

pressure_bar = V_ADC x 10 / 3.00
```

Confirm the sensor pinout and zero-pressure output on the bench before permanent connection.

---

## 10. Suggested terminal plan

### TB1 - mains input

| Terminal | Signal |
|---:|---|
| 1 | 230 V L |
| 2 | 230 V N |
| 3 | PE |

### TB2 - 24 VAC valves

| Terminal | Signal |
|---:|---|
| 1 | Zone 1 switched 24 VAC |
| 2 | Zone 2 switched 24 VAC |
| ... | ... |
| 15 | Zone 15 switched 24 VAC |
| 16 | Master valve switched 24 VAC |
| 17-18 | Valve common 24VAC-B |

Use multiple common terminals or a common distribution block rather than placing many field wires under one screw.

### TB3 - pressure sensor

| Terminal | Signal |
|---:|---|
| 1 | +5 V |
| 2 | GND |
| 3 | Analog OUT |
| 4 | Shield |

### TB4 - flow meter 1

| Terminal | Signal |
|---:|---|
| 1 | Sensor supply |
| 2 | GND |
| 3 | Pulse |
| 4 | Shield |

### TB5 - flow meter 2

| Terminal | Signal |
|---:|---|
| 1 | Sensor supply |
| 2 | GND |
| 3 | Pulse |
| 4 | Shield |

### TB6 - One-Wire bus

| Terminal | Signal |
|---:|---|
| 1 | 3.3 V |
| 2 | GND |
| 3 | One-Wire DATA |
| 4 | Shield |

---

## 11. Grounding and cable segregation

Use three physically distinct wiring groups:

```text
Group A: 230 V AC mains
Group B: 24 V AC valve wiring
Group C: 5 V / 3.3 V / sensor and communication wiring
```

Guidelines:

- Never route analog pressure wiring alongside relay contacts or 24 VAC valve outputs for long distances.
- Cross power and sensor cables at 90 degrees where necessary.
- Use shielded cable for remote analog and pulse sensors.
- Terminate shields at one end, normally the controller enclosure.
- Keep the ADS1115 and pressure input filter away from transformers and relay coils.
- Use a star or low-impedance 0 V distribution for the ESP32, ADC and sensors.
- The isolated 24 VAC circuit does not need to share a conductor with the DC 0 V system.

---

## 12. Protection recommendations

| Connection | Recommended protection |
|---|---|
| 5 V controller feed | Branch fuse and bulk capacitor |
| Relay-board 5 V feed | Separate branch fuse |
| 24 VAC transformer secondary | Secondary fuse before relay common bus |
| Each external sensor input | Series resistance and ESD/TVS protection |
| Flow inputs | Clamp/TVS, RC filtering and Schmitt trigger where needed |
| Pressure input | Divider, RC filter and ADC input clamp |
| Valve outputs | AC-compatible surge suppression |
| External cables | Strain relief and shield termination |

---

## 13. Commissioning sequence

Do not connect everything and energize it for the first time as one assembly. Use this order:

1. Verify mains wiring with all low-voltage outputs disconnected.
2. Verify the Mean Well output is approximately 5.0 V.
3. Verify the Hager transformer secondary is approximately 24 V AC with no load.
4. Power only the ESP32/display board.
5. Add MCP23017 and confirm I2C address `0x20`.
6. Add ADS1115 and confirm I2C address `0x48`.
7. Connect relay input stages without 24 VAC and verify all channels remain OFF at reset.
8. Test each relay contact using continuity mode.
9. Apply 24 VAC and test one spare lamp or test load before connecting valves.
10. Connect the master valve and one zone at a time.
11. Connect DS18B20 sensors and record each ROM address.
12. Connect each flow meter and verify pulse counts manually.
13. Connect the pressure sensor through the divider and measure zero-pressure ADC voltage.
14. Calibrate pressure and both flow meters.
15. Perform an emergency-stop and power-failure test.

---

## 14. Outstanding items before final issue

The following must be confirmed before this diagram becomes an installation drawing:

1. Exact Waveshare ESP32-S3 display-board revision and accessible GPIO connector pinout.
2. A verified free GPIO for the One-Wire bus.
3. Exact HL-58S V1.2 relay-board `VCC`, `JD-VCC`, input and jumper topology.
4. Exact flow-meter models, supply voltages and pulse-output types.
5. Pressure sensor Packard connector pinout.
6. Pressure sensor output characteristic: 0-5 V or 0.5-4.5 V.
7. Valve solenoid inrush and holding currents.
8. Final fuse values and conductor sizes.
9. Whether individual valve-output fusing is required.
10. Enclosure material, DIN-rail arrangement and PE requirements.

---

## 15. Final connection summary

```text
230 VAC
  +-- Mean Well HDR-30-5 -> 5 V controller, relay boards, pressure sensor
  +-- Hager ST315        -> 24 VAC relay contacts and valves

ESP32-S3
  +-- GPIO8/9 I2C -> MCP23017 + ADS1115
  +-- GPIO4 PCNT  -> Flow meter 1
  +-- GPIO5 PCNT  -> Flow meter 2
  +-- free GPIO   -> 4 x DS18B20 shared One-Wire bus

MCP23017
  +-- 16 outputs -> 2 x ULN2803A -> 2 x 8-channel active-low relay boards

Relay contacts
  +-- Zones 1-15
  +-- Master valve 16

ADS1115 A0
  +-- 0-10 bar pressure sensor through 10 kOhm / 15 kOhm divider
```

This draft intentionally separates verified firmware pin assignments from items that still require exact hardware-model confirmation.