# lv2_simple_volume - a minimal LV2 plugin for MOD / pi-Stomp

A one-knob, one-footswitch volume pedal, written to explain how an LV2 plugin
is built and added to MOD. From the californiasonics YouTube channel.

## Files

| File | What it does |
|------|--------------|
| `lv2_simple_volume.lv2/manifest.ttl` | Tells MOD the plugin exists and where its parts are |
| `lv2_simple_volume.lv2/lv2_simple_volume.ttl` | Describes the pedal: name, knob, footswitch, audio ports |
| `lv2_simple_volume.c` | The audio processing code |
| `Makefile` | Compiles the code and installs the plugin |
| `lv2_simple_volume.lv2/modgui.ttl` | Tells MOD the plugin has a custom pedal graphic |
| `lv2_simple_volume.lv2/modgui/icon-lv2_simple_volume.html` | The layers of the pedal: body, knob, LED, footswitch, jacks |
| `lv2_simple_volume.lv2/modgui/stylesheet-lv2_simple_volume.css` | Where each layer sits and what it looks like |
| `lv2_simple_volume.lv2/modgui/*.png` | The artwork |
| `artwork/pedal_original.jpg` | The original pedal graphic |
| `artwork/pedal_body_dB.png` | The pedal body with the dial marked in dB (−40 to +6) |
| `artwork/knob_cutout.png` | The knob cut out of it, pointer at 12 o'clock (input for the knob tool) |
| `tools/make_knob_strip.py` | Turns one knob picture into the film strip MOD needs |

## Install on a pi-Stomp

Log in to the pi-Stomp (e.g. with PuTTY), then:

```bash
sudo apt install -y git build-essential lv2-dev
git clone https://github.com/californiasonics/lv2_simple_volume.git
cd lv2_simple_volume
make
make install
sudo systemctl restart mod-host mod-ui
```

Open the MOD web interface, add **Simple Volume** to the board, and use the
gear icon on each control to assign the knob and footswitch.

## Update after changes

```bash
cd ~/lv2_simple_volume
git pull
make install
sudo systemctl restart mod-host mod-ui
```
