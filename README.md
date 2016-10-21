# SimpleLink Host Interface for ARM mbed 5

This repository contains a TI SimpleLink Host Interface driver for mbed-os 5.

It allows you to talk to a CC3100 daughterboard via a SPI interface.

It uses the mbed HAL, thus it works on all targets supported by mbed.
You only have to pass the pin numbers for the SPI and control signals.

The SimpleLink Host Interface is documented very clearly at http://processors.wiki.ti.com/index.php/CC31xx_Host_Interface

TL;DR:

* 6 wires, of which:
** 4 SPI wires (`SPI_MOSI`, `SPI_MISO`, `SPI_SCK`, `SPI_CS`)
** 1 hibernate wire (basically a reset line)
** 1 IRQ line

```cpp
SimpleLinkInterface wifi(PG_10, PG_11);
```

You can pass a custom frequency:

```cpp
SimpleLinkInterface wifi(PG_10, PG_11, 20000000);
```

Or use another SPI port:

```cpp
SimpleLinkInterface wifi(PG_10, PG_11, SPI_MOSI, SPI_MISO, SPI_SCK, SPI_CS);
```

## Limitations

* no support for disconnect

* no support for wifi scan

* no support for mbed Socket interface; you can either call the native SimpleLink API or use Mongoose (https://github.com/cesanta/mongoose)
