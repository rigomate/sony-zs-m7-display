# M5StickC PLUS live-input investigation

The replay decoder and web server pass their tests, but the first Sony-connected
M5Stick test did not recover usable numeric or title data. This is an active
hardware/capture investigation; do not treat the earlier web-only deployment
check as verification of wired reception.

## Observations

- Device is still the M5StickC PLUS (ESP32-PICO-D4), not the ESP32-C3.
- Initial firmware exposed ~7.5 million application queue drops and zero-byte
  numeric transactions. A bounded capture drain, suppression of zero-clock
  transactions, and rearming only while CS is inactive stopped the flood.
- With diagnostic firmware, select and data on the numeric bus change, but G0
  has no sampled low levels and SPI-1 reports zero received bits per select.
- Independent ESP32 PCNT hardware counters confirm **0 rising edges on G0**.
  At uptime 79.193 s, the text-clock counter recorded 298,875 edges while G0
  recorded none. `captures/m5-debug/edge-counter-api.json` retains the snapshot.
- SPI-1's clock-input route register is 128 (GPIO0, GPIO matrix input selected),
  and the pad mux enables its digital input. Thus the requested input routing
  is configured; this is not merely a millisecond-polling alias.
- SPI-2 receives mostly 120-bit frames, but also unexpected lengths and corrupt
  text. No guessed transform has been applied to make it look correct.
- Saleae baseline `before.sal`/CSV contains only high levels because the probes
  were disconnected. It is not evidence of the actual Sony bus state. The user
  is making a header to permit parallel analyzer access.

## Schematic evidence and hypothesis

The official M5StickC PLUS schematic shows R12 = **1 kOhm from GPIO0 to 3.3 V**,
plus the USB programmer's GPIO0 control circuit, the microphone clock connection,
and a 22-ohm series resistor to the external pad. This is a physical pull-up;
disabling the ESP32's internal pulls cannot remove it.

The Sony interface image in this workspace shows R448 = **1 kOhm in the SCK1
path to IC401**. Whether it is between the Sony source and the M5Stick depends
on the actual tap location. If it is, a source attempting to drive 0 V through
1 kOhm against the M5Stick's 1 kOhm pull-up would leave the pad near 1.65 V,
ignoring output and protection-device effects. That is above the ESP32's
specified low-level maximum of 0.25*VDD (about 0.825 V at 3.3 V).

This is a plausible explanation, not yet an established voltage measurement.
An open/misplaced connection or USB programming-circuit interaction remain
possible. No resistor has been removed and no player command was sent.

## Further evidence being collected

Firmware includes hardware clock-edge counters, pin high/low sample counts,
raw bit lengths, and SPI routing/bit-order registers under `receiver` in the
API. A short startup ADC2 measurement of GPIO0 is taken before Wi-Fi starts;
GPIO0 is returned to digital-input mode before SPI initialization. Approximate
minimum/maximum readings and saturation flags are exposed in
`gpio0_voltage_at_boot`. Saturated readings do not establish the true voltage.

Next compare the clock at the Sony tap and at the M5Stick input with the new
header, and establish signal voltages. Buffering/level conversion may be needed;
a decoder change cannot correct a clock that never crosses the input threshold.

Sources:
- M5Stack schematic: https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/669/k016-p-StickC-Plus-sche.pdf
- Sony schematic excerpt: `images/display-intreface.png`
- ESP32-PICO DC input specifications: https://www.espressif.com/sites/default/files/documentation/esp32-pico-d4_datasheet_en.pdf

## Startup analog result

`captures/m5-debug/voltage-at-boot-api.json` records 4,168 ADC2 samples during
120 ms before Wi-Fi starts. Minimum raw code 2295 maps to approximately 1,935 mV
using the SDK's default 1100 mV reference (calibration type 0); maximum raw code
4095 is saturated (3,942 samples). The true high voltage is therefore unknown.
This is an approximate ADC observation, not an oscilloscope waveform or a precise
calibrated voltage measurement. At the time of sampling, the minimum was well
above the specified valid-low limit (~825 mV at a 3.3 V I/O supply).

This supports the strong-pull-up/loading explanation and shows why changing SPI
mode cannot restore a valid G0 clock. Confirm the loaded/unloaded waveform and
voltage once the parallel header is available. A high-input-impedance,
3.3-V-output buffer with suitable input voltage tolerance can isolate the Sony
signal from the M5Stick's strong pull-up; the existing resistor is physical and
cannot be disabled by firmware. No board modification has been made.

Of those 4,168 samples, 226 (5.42%) were below ADC full scale. The measured
numeric clock has approximately 128 low half-periods of ~12.5 us per ~31.9 ms
frame (~5% low duty over time). The ADC proportion is consistent with clock
pulses whose low voltage has been raised, but a scope/parallel trace is still
needed to establish that waveform directly.

A possible isolating interface is an SN74LVC245A powered at 3.3 V, with signal
inputs on the Sony side and outputs on the ESP32 side. Its inputs accept up to
5.5 V and the output drive can overcome a 1 kOhm pull-up. This is a candidate
component, not an installed or tested fix. Datasheet:
https://www.ti.com/product/SN74LVC245A
