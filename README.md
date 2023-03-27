# webft8 - FT8 decoder in browser

Live demo: https://webft8.github.io/

Rafal Rozestwinski (callsign: SO2A, https://rozestwinski.com), project home: https://github.com/webft8/webft8

Based on an excellent: https://github.com/kgoba/ft8_lib

Tested with Google Chrome and on iPhone.

![Screenshot](/web/webft8.github.io.jpeg?raw=true "Screenshot")

## Instructions

1. Open web browser https://webft8.github.io/
2. Click on webpage
3. Allow microphone access
4. Play some FT8 audio frequency near your computer/phone microphone.
5. Watch decoded FT8 messages.

## Building

### Dependencies

- Install emscripten, https://emscripten.org
- Remember to pull submodules.
- Website has to be hosted with https protocol, as required by browser audio plugin.

### Quickly make wasm + website

`make`

### Run full non-browser test set

`cd src && make`




