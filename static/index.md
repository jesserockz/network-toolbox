# Network Toolbox

ESPHome firmware for handheld network test gear. Plug a device into an unknown
network and read back what that network gives you: link speed and duplex,
whether DHCP answered, the address, netmask, gateway, DNS and IPv6 it handed
out, and live latency and loss to the gateway and the internet.

# Screens

<div class="carousel" id="carousel">
  <div class="carousel-frame">
    <img class="carousel-shot" src="network.png" width="320" height="240" alt="Network tab: 100M full duplex link, IPv4 selected, showing address, netmask, gateway and DNS" data-caption="Network: the lease IPv4 handed out">
    <img class="carousel-shot" src="network-ipv6.png" width="320" height="240" alt="Network tab: IPv6 selected, showing global and link-local addresses" data-caption="Network: the same tab switched to IPv6">
    <img class="carousel-shot" src="ping.png" width="320" height="240" alt="Ping tab before a run, latency and loss both blank" data-caption="Ping: idle, before either target is probed">
    <img class="carousel-shot" src="ping-done.png" width="320" height="240" alt="Ping tab after a run, 4 ms to the internet and 1 ms to the gateway, no loss" data-caption="Ping: four probes to each target, latency and loss">
    <img class="carousel-shot" src="power.png" width="320" height="240" alt="Power tab on USB, VBUS 4.88 V, system 3.72 V, no battery fitted" data-caption="Power: running from USB, feeding the base">
    <img class="carousel-shot" src="power-poe.png" width="320" height="240" alt="Power tab on PoE, base on its own supply, VBUS 4.83 V" data-caption="Power: running from PoE, base on its own supply">
    <img class="carousel-shot" src="system.png" width="320" height="240" alt="System tab showing device name, uptime, update toggle, MAC and firmware version" data-caption="System: uptime, MAC, firmware and update checking">
  </div>
  <div class="carousel-bar">
    <button type="button" class="carousel-btn" data-step="-1" aria-controls="carousel" aria-label="Previous screenshot">&#8592;</button>
    <p class="carousel-caption" aria-live="polite"></p>
    <button type="button" class="carousel-btn" data-step="1" aria-controls="carousel" aria-label="Next screenshot">&#8594;</button>
  </div>
</div>

<style>
  .carousel {
    /* The captures are 320x240. Cap at that so they are never upscaled, and
       let them shrink only on viewports narrower than the image itself. */
    max-width: 320px;
    margin: 1.5rem 0;
  }
  .carousel-frame {
    background: #0d1117;
    /* box-shadow rather than border: a border is laid out inside the 320px
       box and would shrink the image to 318, scaling it back off native. */
    box-shadow: 0 0 0 1px #30363d;
    border-radius: 6px;
    line-height: 0;
  }
  .carousel-shot {
    display: none;
    width: 100%;
    height: auto;
    /* The slate theme puts padding and a light border on every img, which
       renders as a pale inset inside the dark frame and would push the capture
       12px past native. Undo both so it draws at exactly 320x240. */
    padding: 0;
    border: 0;
    /* At 1:1 this is a no-op. It only bites on a HiDPI screen, where the
       browser draws one CSS pixel as several device pixels: nearest-neighbour
       keeps the LCD pixels crisp instead of smearing them. */
    image-rendering: pixelated;
  }
  .carousel-shot.is-current {
    display: block;
  }
  .carousel-bar {
    display: flex;
    align-items: center;
    gap: 0.75rem;
    margin-top: 0.5rem;
  }
  .carousel-caption {
    flex: 1;
    margin: 0;
    text-align: center;
    font-size: 0.9rem;
    /* Captions differ in length and wrap to two lines at this width. Reserve
       the space so stepping through does not shift the layout. */
    min-height: 2.6em;
  }
  .carousel-btn {
    flex: none;
    min-width: 2.5rem;
    padding: 0.35rem 0.6rem;
    font-size: 1.1rem;
    line-height: 1.2;
    cursor: pointer;
    background: #21262d;
    color: #e6edf3;
    border: 1px solid #30363d;
    border-radius: 6px;
  }
  .carousel-btn:hover {
    background: #30363d;
  }
</style>

<script>
  (function () {
    var carousel = document.getElementById("carousel");
    var shots = carousel.querySelectorAll(".carousel-shot");
    var caption = carousel.querySelector(".carousel-caption");
    var current = 0;

    function show(index) {
      // Wraps in both directions, so neither button is ever a dead end.
      current = (index + shots.length) % shots.length;
      for (var i = 0; i < shots.length; i++) {
        shots[i].classList.toggle("is-current", i === current);
      }
      caption.textContent =
        shots[current].dataset.caption +
        " (" + (current + 1) + " of " + shots.length + ")";
    }

    var buttons = carousel.querySelectorAll(".carousel-btn");
    for (var b = 0; b < buttons.length; b++) {
      buttons[b].addEventListener("click", function () {
        show(current + Number(this.dataset.step));
      });
    }

    show(0);
  })();
</script>

<noscript>
  <style>
    #carousel .carousel-shot { display: block; }
    #carousel .carousel-bar { display: none; }
  </style>
</noscript>

# Installation

You can use the button below to install the pre-built firmware directly to your
device via USB from the browser.

<esp-web-install-button manifest="firmware/network-toolbox.manifest.json"></esp-web-install-button>

<script type="module" src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module"></script>
