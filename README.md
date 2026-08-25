# Network Toolbox

ESPHome firmware for handheld network test gear. Plug a device into an unknown
network and read back what that network actually gives you: link speed and
duplex, whether DHCP answered, the address, netmask, gateway and DNS it handed
out, both IPv6 addresses, and round trip time and loss to the gateway and to the
internet.

These are deliberately not Home Assistant devices. There is no `api:` and mDNS is
off, so they stay quiet on the network under test. Firmware updates come from an
HTTP manifest rather than from an ESPHome dashboard.

## Devices

One YAML per device at the top level, each self-contained: no packages, no
includes, no `secrets.yaml`. Shared code lives in `components/`.

| Config                            | Hardware                                              |
| --------------------------------- | ----------------------------------------------------- |
| `core-s3-poe-base.factory.yaml`   | M5Stack CoreS3 with Base LAN PoE v1.2 (SKU K012-C-V12) |

```bash
esphome run core-s3-poe-base.factory.yaml
```

Flashing is over USB. There is no ESPHome OTA platform configured, only
`ota: platform: http_request` pointed at a manifest URL that is deliberately
unresolvable until there is a real one to point at.

The YAML files are heavily commented, and those comments carry the reasoning
behind the non-obvious choices. Read them before changing anything in a boot
sequence.

## Shared components

Both are local rather than pulled from GitHub:

- `components/ping` wraps ESP-IDF's own `esp_ping` and exposes a `ping.ping`
  action. ESPHome has no ping component of its own. Wrapping rather than
  vendoring keeps it in step with whatever lwIP the framework ships.
- `components/aw9523` is a fixed-up fork of the AW9523 I/O expander driver. The
  public copy no longer builds against current ESPHome, and its `pin_mode` tests
  the output flag twice, so every pin ends up an input.

## Pending upstream

Unmerged ESPHome pull requests are pinned via `github://pr#`. Drop them from
`external_components:` once they reach a release.

| PR                                                      | Why it is needed                                                  |
| ------------------------------------------------------- | ------------------------------------------------------------------ |
| [#18530](https://github.com/esphome/esphome/pull/18530) | Adds `spi_id:` to `ethernet:`, so a W5500 can join an existing bus |

Because that one is still open, the builds track ESPHome **dev** rather than a
release. Pin `esphome-version` in `.github/workflows/build.yml` back to a release
once it lands.

The companion fix for GPIO35 being both bus MISO and the LCD D/C line
([#18529](https://github.com/esphome/esphome/pull/18529), `mipi_spi` toggling D/C
inside the SPI bus lock) is already merged and in dev, so it no longer needs
pinning.

## M5Stack CoreS3 with Base LAN PoE

The W5500 shares the CoreS3's SPI bus with the LCD rather than claiming a host of
its own. The CoreS3 bus is multi-drop by design: the LCD, the microSD slot and the
M-Bus base all sit on the same clock, MOSI and MISO nets with their own chip
selects, and the board carries 10K pull-ups on all three. GPIO35 is both bus MISO
and the LCD D/C line, which is what #18529 above is for.

Power can cross the M-Bus in either direction. On USB the CoreS3 feeds the base;
on PoE the base feeds itself. There is no sense line that says which, so the
firmware works it out at boot and records the answer in flash.

### The UI

Four tabs, selected from the tab bar. Swipe is disabled on purpose: the content
container snaps back on a short drag, which reads as the swipe being ignored.

| Tab     | Shows                                                                  |
| ------- | ---------------------------------------------------------------------- |
| Network | Link speed and duplex, and an IPv4/IPv6 selector over the addressing    |
| Ping    | Latency and loss to the internet target and to the gateway             |
| Power   | Supply source, base power direction, VBUS, system voltage, battery     |
| System  | Uptime, MAC, firmware version, update checking                         |

`Link` distinguishes an unplugged cable from a live link that never got a lease,
which is most of the point of the device. Every field refreshes on a link change
rather than waiting for its next poll.

The Power tab's `Hold to power off` button fades the backlight over two seconds
before the PMU is told to shut down. Releasing early cancels it, so the fade is
the countdown.
