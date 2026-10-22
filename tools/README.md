# UART flow control tools

Small helpers used next to mbt to see and control the RTS/CTS lines of the
Bluetooth UART. They use the standard modem line ioctls (TIOCMGET, TIOCMBIC,
TIOCMBIS). What they read is the line state the UART driver reports, not the
voltage on the pin, and the time resolution is the polling interval.

Build for Android with `ANDROID_NDK_ROOT=<NDK directory> make tools`; the
binaries go to Release/android/.

| Tool | What it does |
|---|---|
| `cts_watch <tty> <seconds>` | Logs every change of CTS (and the RTS bit) with the time since start, polling every 20 ms. Opens the port without changing its settings, so it can run while mbt or the Bluetooth HAL uses it. |
| `rts_ctl <tty> <run_s> <pulse_s> <hold_s>` | Like cts_watch (5 ms), and also: at `pulse_s` deasserts RTS for 0.5 s (to check that RTS control works: with the controller's flow control on, its replies pause); then, when CTS stays low for more than 100 ms (for example after Launch RAM), deasserts RTS for `hold_s` seconds. Use with flow control off, otherwise the UART drives RTS itself. |
| `uart_watch <tty> <rfkill state file> <off_ms> <listen_ms> [baud]` | Power-cycles the controller through rfkill (BT_REG_ON), optionally with RTS held deasserted (`RTSOFF=1`), logs what the controller sends, then sends HCI commands (`ORDER=rnv`: Reset, Read Local Name, Read Local Version) and logs the replies. |

On the MT8786 host, software control of RTS was confirmed at 115200 and
921600; at 3 Mbps it had no effect in an earlier test.
