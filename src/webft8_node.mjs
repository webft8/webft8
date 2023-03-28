import * as webft8_ft8_decode from './webft8_ft8_decode.js'
import * as test_data from './test_data.js'
import * as fs from 'fs'

//console.log(webft8_ft8_decode);

try {
    //var byteArray = test_data.waveUint8Arr; //new Uint8Array([ 1, 2, 3, 4, 5, 6, 7 ,8 ,  9, 10, 0, 11])
    const fdata = fs.readFileSync('../../extra_test_data/2023-03-23T18_14_26.815Z.wav', { flag:'r'});
    const byteArray = new Uint8Array(fdata);
    var conf = {
        format: "wav",
    };
    var ret = webft8_ft8_decode.webft8_ft8_decode(conf, byteArray);
    console.log("RET: " + JSON.stringify(ret, null, 3));
} catch(error) {
    console.log("failed: " + error + ", stack:\n" + error.stack);
    throw error;
}
