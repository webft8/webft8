import * as webft8_ft8_decode  from "../webft8/webft8_ft8_decode.js";

var webft8_worker = null;

await webft8_ft8_decode.init();

export const decode_ft8_audio_frame_async = (config_json, int8arr) => new Promise((res, rej) => {
	const channel = new MessageChannel(); 
	channel.port1.onmessage = ({data}) => {
		channel.port1.close();
		if (data.error) {
			rej(data.error);
		}else {
			res(data.result);
		}
	};
	webft8_worker.postMessage([config_json, int8arr], [channel.port2]);
});




var intervalID = null; 


export function stop() {
    console.log("Stopping.");
    if(intervalID) {
        clearInterval(intervalID);
        intervalID = null;
    }
}


// { 
//    recorder_start_fn: (){}, 
//    recorder_is_running_fn: (){}, // returns bool true if is still running
// }
export function start(config) {
    console.log("WebFT8::start()!");
    if(config.recorder_start_fn == null) {
        throw("Missing recorder_start_fn!");
    }
    if(config.recorder_is_running_fn == null) {
        throw("Missing recorder_is_running_fn!");
    }
    if (window.Worker) {
        //const worker = new Worker("data:application/x-javascript;base64,b25tW...", { type: "module" });
        webft8_worker = new Worker("./webft8/webft8_worker.js", { type: "module" });
    } else {
        console.error('Your browser doesn\'t support web workers.');
        throw 'Your browser doesn\'t support web workers.'
    }
    

    intervalID = setInterval((a) => {
        const d = new Date();
        let s = d.getSeconds();
        if(s == 0  || s == 1  ||
           s == 15 || s == 16 ||
           s == 30 || s == 31 ||
           s == 45 || s == 46) {
            //console.log("seconds == " + s);
            if(config.recorder_is_running_fn()) {
                //console.error("Unable to start, recorder still running!");
            } else {
                const offset = s % 5;
                const recording_time = 15 - offset;
                if(offset > 0) {
                    console.log("We're late... trying anyway.");
                }  
                //console.log(`calculated offset: ${offset}s, recording time: ${recording_time} s`)
                config.recorder_start_fn(recording_time);
            }
            
        }
    }, 250, {});




    //if(!config.recorder_is_running_fn()) {
    //    config.recorder_start_fn(recording_time);
    //} else {
    //    console.log("Already running!");
    //}
    console.log("WebFT8::start() - complete.");
    
}
