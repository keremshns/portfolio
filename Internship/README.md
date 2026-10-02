# ADS1148 16-bit ADC driver for STM32F103 (bare-metal C)

Core firmware that configures a Texas Instruments **ADS1148** 16-bit delta-sigma ADC over **SPI** from an
**STM32F103RC** (ARM Cortex-M3) MCU, reads conversion results on the ADC's data-ready interrupt, and
converts the raw two's-complement samples into the measured supply voltage, streamed over UART.

Developed during my summer internship at **Pavotek A.Ş.** (Jul–Sep 2019) for a precision analog
measurement board. Only the core source file (`main.c`) is shared here. The rest of the project
(board configuration, HAL setup, headers) belongs to the company and is not public.
Project poster: [internship-ads1148-poster.pdf](internship-ads1148-poster.pdf).

## What it does

```
analog input (0–10 V) ──► INA159 / op-amp front end ──► ADS1148 (×2, SPI) ──► STM32F103 ──► UART "Value: x.xxx"
```

- Register-level driver for the ADS1148: channel multiplexer, voltage reference, PGA gain, data rate,
  bias, IDAC, burn-out sources, GPIO, self/system offset and gain calibration.
- Drives **two ADCs** on a shared SPI bus with separate chip-select lines.
- Uses the **DRDY falling edge (EXTI interrupt)** to read each conversion as soon as it is ready.
- Converts each sample to a voltage:

  ```
  raw (two's complement) + 2^15   → unsigned code
  code × step size − 2.5 V        → ADC differential input
  −5 × input + 1 V (+ offset)     → power-supply input voltage
  ```

## Where to look in `main.c`

| Function | Role |
|---|---|
| `main()` | Peripheral init, then the conversion loop: read sample, convert to voltage, print over UART |
| `InitConfig()` | ADC reset and start-up sequence, following the pseudocode on p. 65 of the ADS1148 datasheet |
| `ADS1148ReadDout()`, `ADS1148ReadRegister()`, `ADS1148WriteCommand()` | SPI transactions with the ADC |
| `ADS1148Set*()` | Register configuration helpers (channel, reference, gain, data rate, …) |
| `HAL_GPIO_EXTI_Callback()` | Flags a new sample on each DRDY falling edge |

## Result

The board correctly digitised **1.2–10 V** analog inputs (target: 0–10 V). Getting there took
hardware/software co-debugging:

1. **All-zero readings.** The datasheet showed the input range is ±V<sub>REF</sub>/gain, and the ADC only
   takes its reference from the dedicated REFP/REFN pins. These were unused, so V<sub>REF</sub> was 0 V.
   Routing 2.5 V to the reference pins fixed it.
2. **Only 5–10 V readable.** Traced to a mis-connected pin on the INA159 front-end amplifier. Tying it
   to 5 V extended the range to 1.2–10 V.
3. **SPI data shifted one bit right.** Worked around in software with `shiftl()` after each read.

A higher reference voltage would allow the full 0–10 V range. The internship ended before that was done.

## Notes

The file was generated with STM32CubeMX and does not build on its own without the company's project files.
Comments in the source are in Turkish.

## My contribution

The work built on an existing in-house ADS1148 driver. I debugged the hardware/software chain (the
reference-voltage and amplifier issues above), implemented the conversion from raw samples to supply
voltage and the UART output, and validated the measurements on the board.
