import * as webft8_ft8_decode  from "../webft8/webft8_ft8_decode.js";

addEventListener("message", (event) => {
    try{
      //if (typeof event.data[0] !== "number" || typeof event.data[1] !== "number") {
      // / throw new Error("both arguments must be numbers");
      //}
      const config_json = event.data[0];
      const int8arr = event.data[1];
      const decoded = webft8_ft8_decode.webft8_ft8_decode(config_json, int8arr)
      event.ports[0].postMessage({result: decoded});
    }catch(e) {
      event.ports[0].postMessage({error: e});
    }
}, false);
