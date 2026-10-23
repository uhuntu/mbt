# Jia Hu MBT 2.11 source snapshot

Received through Infineon case IFX-260818-2262707:

- `mbt.c` and `mbt.h`: 2026-10-22, per the session record.
- `mbt_common.c`: 2026-10-23, completing the three-file set.

All three files are byte-for-byte copies of the supplied attachments,
including original line endings and license notices. Version 2.11 is the
version reported by the supplied source, not a claim of a public release tag.
This branch starts from `main` commit `6f82e08`; the custom Android branch's
per-record status checks, flow-control environment variable and GPIO tools
are not included. Existing tracked binaries and scripts are inherited from
main and have **not** been rebuilt for 2.11. Build from source before use.

## Source SHA-256

```
90c1fa6069278b600d8a3e4f603871c76336c621caace81488dff15bcbf920d2  mbt.c
5a9865e65c09b9a42cbb106caafd94ed98349c95f87fe90886183f2b24caf747  mbt.h
461eb90840b16dd4d4ef2ec76f0669b995c5a191108f5e0e3e35df3934d77868  mbt_common.c
```

## Build and use

The existing Makefile supports `make x86` and `make arm64`; run `make clean`
before changing architecture. ARM64 here means Linux ARM64, not Android.
The original source also compiles with Android NDK r26b targeting API 30;
this branch does not add an Android Makefile target.

Set `MBT_TRANSPORT` to the controller UART. For full byte logging, put
`--message` at the end of the download command, for example:

```sh
export MBT_TRANSPORT=/dev/ttyS6
./Release/x86/mbt download firmware.hcd --autobaud --message
```

Use the binary for the host architecture. This command assumes the board
has been prepared for autobaud download. The supplied source enables
CRTSCTS unconditionally; it does not support `MBT_FLOW_CONTROL=0`.

## ROCK 5B test, 2026-10-23

Using these unmodified sources with `--autobaud --message` at 921600:

- Public 0225: 463 HCD records acknowledged with status 0x00; patched name
  `CYW55560A2 UART WLBGA iPA sLNA ANT0` and FC79 payload
  `00 97 FE E1 00 00 00` confirmed afterwards.
- Test 0415: 615 records acknowledged with status 0x00 and download command
  reported success; subsequent independent queries returned `CYW55560A1`
  and `00 97 FE FF FF 00 00`, matching the earlier ROM baseline.

The adapter has crossed RTS/CTS wiring. The host overlay used UART6 TX/RX
only, with GPIO control of the flow pins. An external Python helper disabled
host CRTSCTS without editing these sources. Identity queries used the
existing no-flow MBT branch. This does not validate the stock hardware-flow
control path. The 3 Mbps attempts did not receive a download-entry response
for either image. No external LPO was connected or measured, and no new
4.5-second timing conclusion is drawn from this run.
