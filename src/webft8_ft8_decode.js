import webft8_ft8_decode_js from './webft8_ft8_decode_js.js';

var _debug = true;

export function enable_console_stdio() {
        _debug = true;
}

export function init(debug) {
        
}

var webft8_ft8_decode_wasm_module = await webft8_ft8_decode_js({
        preRun: [],
        postRun: [],
        print:  (function() {
          return function(text) {
            if(_debug) {
                console.log(text);
            }
            //stdout += text;
          };
        })(),
        printErr: (function() {
          return function(text) {
            if(_debug) {
                //console.error(text);
                console.log(text);
            }
            //stderr += text;
          };
        })()
 });
console.log(webft8_ft8_decode_wasm_module);

var _webft8_ft8_decode_js = webft8_ft8_decode_wasm_module.cwrap('webft8_ft8_decode_js', 'number', ['string', 'number', 'number']);
//                                                                                      json_str*   json_config* buffer, buffer_size

var _webft8_buffer_create = webft8_ft8_decode_wasm_module.cwrap('webft8_buffer_create', 'number', ['number']);
//                                                                                       buf*        size

var _webft8_buffer_size = webft8_ft8_decode_wasm_module.cwrap('webft8_buffer_size', 'number', ['number']);
//                                                                                       size      buf*

var _webft8_buffer_pos = webft8_ft8_decode_wasm_module.cwrap('webft8_buffer_pos', 'number', ['number']);
//                                                                                       pos      buf*

var _webft8_buffer_data = webft8_ft8_decode_wasm_module.cwrap('webft8_buffer_data', 'number', ['number']);
//                                                                                    data*      buf*

var _webft8_buffer_write = webft8_ft8_decode_wasm_module.cwrap('webft8_buffer_write', 'number', ['number', 'array', 'number']);
//                                                                                       size      buf*       input_arr  size

var _webft8_buffer_destroy = webft8_ft8_decode_wasm_module.cwrap('webft8_buffer_destroy', 'void', ['number']);
//                                                                                                   buf* 

var _webft8_buffer_dump = webft8_ft8_decode_wasm_module.cwrap('webft8_buffer_dump', 'void', ['number']);
//                                                                                                   buf* 

function _on_error(msg) {
        console.error(msg);
        throw msg; 
}

/// input:
///    config_json: {
///        format: "wav",
///    };
///    int8arr:
///        new Uint8Array([ 1, 2, 3, 4, 5, 6, 7 ,8 ,  9, 10, 0, 11, ...]);
/// output:
///     JSON: {
///          "decoded": [{
///                      "text": "CQ DL8ALH JN58",
///                      "snr": 18,
///                      "freq_hz": 271.875,
///                      "time_sec": 1.52
///                   },
///                   ...
///          ],
///          "error": null,
///          "stats": {}
///         }
///      }
/// on error: throws exception.
export function webft8_ft8_decode(config_json, int8arr) {
        console.log(`webft8_ft8_decode(config=${JSON.stringify(config_json)} dataLen=${int8arr.length})`);
        var tmp_buffer = _webft8_buffer_create(int8arr.length);
        if(tmp_buffer == null) {
                _on_error("webft8_ft8_decode(): tmp_buffer null!");
        }
        var pos = 0;
        var wrote = 0;
        try {
                while(true) {
                        var remaining = int8arr.length - pos;
                        if(remaining <= 0) {
                                if(remaining != -1) {
                                        _on_error(`remaining = ${remaining} (expected=-1), wrote everything?`)
                                }
                                break;
                        }
                        var to_write = Math.min(remaining, 4096);
                        var a = int8arr.subarray(pos, pos+to_write+1);
                        wrote += a.length;
                        _webft8_buffer_write(tmp_buffer, a, a.length);
                        pos += to_write + 1;
                }
                var buffer_data = _webft8_buffer_data(tmp_buffer);
                //_webft8_buffer_dump(tmp_buffer);
                console.log("Calling FT8 decoder");
                var time_begin_call = Date.now();
                var ptr = _webft8_ft8_decode_js(JSON.stringify(config_json), buffer_data, int8arr.length);
                var delta = Date.now() - time_begin_call;
                console.log(`Time in WASM: ${delta}ms`);
                if(ptr == 0 || ptr == null) {
                        _on_error("_webft8_ft8_decode_js failed!")
                }
                var out_s = webft8_ft8_decode_wasm_module.UTF8ToString(ptr);
                webft8_ft8_decode_wasm_module._free(ptr);
                _webft8_buffer_destroy(tmp_buffer);
                tmp_buffer = null;
                //console.log(out_s);
                var ret = JSON.parse(out_s)
                ret.stats = {}
                ret.stats.processing_time_ms = delta;
                ret.stats.input_data_size_bytes = int8arr.length;
                return ret;
        } catch(error) {
                console.error("failed, will perform cleanup, exception: " + error + ", stack:\n" + error.stack + "\n");
                try {
                        if(tmp_buffer) {
                                console.log(tmp_buffer);
                                _webft8_buffer_destroy(tmp_buffer);
                        }
                } catch(error2) {
                        console.error("Error while deallocating memory. Ignore it then... " + error)
                }
                throw error;
        }

}

