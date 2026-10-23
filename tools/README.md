# UART flow control and power tools

Small helpers used next to mbt to see and control the RTS/CTS lines of the
Bluetooth UART, and the GPIO that powers the controller. The UART tools use
the standard modem line ioctls (TIOCMGET, TIOCMBIC, TIOCMBIS). What they read
is the line state the UART driver reports, not the voltage on the pin, and the
time resolution is the polling interval. gpio_line uses the GPIO character
device (/dev/gpiochipN).

Build for Android with `ANDROID_NDK_ROOT=<NDK directory> make tools`; the
binaries go to Release/android/.

| Tool | What it does |
|---|---|
| `cts_watch <tty> <seconds>` | Logs every change of CTS (and the RTS bit) with the time since start, polling every 20 ms. Opens the port without changing its settings, so it can run while mbt or the Bluetooth HAL uses it. |
| `rts_ctl <tty> <run_s> <pulse_s> <hold_s>` | Like cts_watch (5 ms), and also: at `pulse_s` deasserts RTS for 0.5 s (to check that RTS control works: with the controller's flow control on, its replies pause); then, when CTS stays low for more than 100 ms (for example after Launch RAM), deasserts RTS for `hold_s` seconds. Use with flow control off, otherwise the UART drives RTS itself. |
| `uart_watch <tty> <rfkill state file> <off_ms> <listen_ms> [baud]` | Power-cycles the controller through rfkill (BT_REG_ON), optionally with RTS held deasserted (`RTSOFF=1`), logs what the controller sends, then sends HCI commands (`ORDER=rnv`: Reset, Read Local Name, Read Local Version) and logs the replies. |
| `gpio_line <chip> <line> info \| set <0\|1> \| pulse <0\|1> <ms>` | Shows or drives one GPIO line through the GPIO character device (v2 uapi), for hosts where BT_REG_ON or the module supply is a plain GPIO without an rfkill driver. `pulse` drives the line to the value for the given time and back, then releases it. The line is requested as an output that already has the final value, so a line that is high is not glitched by requesting it. |

On the MT8786 host, software control of RTS was confirmed at 115200 and
921600; at 3 Mbps it had no effect in an earlier test.

On the MediaTek Genio 1200 EVK (MT8395), the M.2 Key E slot has no GPIO
for W_DISABLE#2 (BT_REG_ON on our M.2 test board) and no UART; the slot's
3.3 V supply is enabled by GPIO_135. `gpio_line /dev/gpiochip0 135 pulse 0
500` powers the whole module off for 0.5 s. Remove the PCIe device first
(`echo 1 > /sys/bus/pci/devices/0000:01:00.0/remove`) and rescan
afterwards (`echo 1 > /sys/bus/pci/rescan`); the module comes back.
