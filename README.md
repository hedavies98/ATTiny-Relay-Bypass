# ATTiny-Relay-Bypass
Latching and momentary relay control with indicator LED using an ATTiny 
___

## Use
Control a relay (or three!) using a single ATTiny. Connect a switch between switch in and ground and then connect your relay to one of the three relay pins. 

No need for an external pull up resistor since the ATTiny is configured to use the internal ones, the switch is also software debounced. Aside from the switch and relay no external hardware is necessary. 

Pressing the switch will toggle the active high and low pins and send a 3ms pulse to the latching relay pin.

Holding the switch for more than 400ms will engage momentary mode, only toggling the relays on while the switch is held and turning them off when it's released. This also works with the latching relay, a pulse is sent on hold and on release

## Modifications
If you want to alter the timings you can edit the below constants at the top of the source

`F_CPU 1000000UL` -> Tells the code the ATTiny clock speed, set this to whatever your Tiny is running at

`MOMENTARY_DELAY 400` -> If the switch is held for this long it will enter momentary mode, turning the relays off once it's released. Set this to something very small to always have the switch operate in momentary mode. Currently there is no way to disable momentary operation after a certain length of time has passed, a workaround is to set this variable to something large like 8000000UL (~2.5 hours)

`LATCHING_TIME 3` -> Pulse length in ms for the latching relay, 3ms should work with most relays but can be changed if necessary

### Note!

The active low relay pin uses the reset pin on the ATTiny, to enable this you'll have to disable that pin using the fuses, this will also disable ISP until you reset the fuses with HVSP


# Pinout
```
                   _______
Active Low  Relay-|o      |-Vcc
Active High Relay-|       |-Switch In
   Latching Relay-|       |-Static LED
              Gnd-|_______|-Blinky LED
```
