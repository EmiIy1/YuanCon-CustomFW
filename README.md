## Controller Modes
Currently four controller modes are implemented:
1. Mouse + keyboard, gamepad variant 1
2. Mouse + keyboard, no gamepad
3. Gamepad variant 1, joystick position = knob position
4. Gamepad variant 2, joystick position = knob direaction

The mode can be selected by holding EX1, FXL and FXR, then either tapping the corresponding BT, or using FX2 to cycle through.

The default mode is mode 1, which will work both in anything using keybinds or anything using a gamepad (e.g. EAC) out of the box. When binding in spice it will preferentially use the gamepad buttons over the keyboard. If you don't want this, you can switch to mode 2 during binding.

## Lighting Modes
Lighting is seperated into four individually configurable zones:
1. The wing lights
2. The top lights, referred to as the "start" lights
3. The button lights (including lights for the EX buttons, unpopupated on a stock controller)
4. Laser effects, when turning a knob or pressing a button (the latter only when reactive lights are enabled)

When holding EX-1, the mode for a zone can be cycled by tapping the BT corresponding to that zone.

Solid colour lighting is divided into three zones, the start LEDs, the top half of the wing LEDs, and the lower half, respectively. There is currently no way to configure these zones independently on the controller itself, and as such all three zones are updated in unison. The colours in solid colour modes can be changed by holding EX-1 then rotating the corresponding knob for the left or right side.

Reactive lights are enabled by default, and can be toggled by holding EX-1 and tapping FX-L. Reactive lights will show lighting effects when any of the buttons on the controller are pressed.

Auto-HID is enabled by default, and can be toggled by holding EX-1 and tapping FX-R. In Auto-HID mode, the controller will switch to using HID lighting if it detects a game running using HID lighting, then will return to normal after the game closes.

### Wing lights options
1. Rainbow puke
2. Solid colour
3. HID-reactive
4. Disabled

### Start lights options
1. Rainbow puke
2. Solid colour
3. HID-reactive
4. Disabled

### Button lights options
1. Buttons light when pressed
2. HID-reactive
3. HID-reactive, but also lighting when pressed
4. Disabled

### Laser light effect options
1. White
2. Use colour of choice
3. Disabled
