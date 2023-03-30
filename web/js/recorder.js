// Copyright (c) 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

import createBlobFromAudioBuffer from './exporter.mjs';
import * as webft8 from '../webft8/webft8.js'
import * as webft8_table from './webft8_table.mjs'

const context = new AudioContext();
let isRecording = false;
let visualizationEnabled = true;
let total_decoded = 0;

function isRecordingFn() {
  //console.log("isRecordingFn " + isRecording)
  return isRecording;
}

function setIsRecordingTrueFn() {
  //console.log("set recording to true, previous = " + isRecording)
  isRecording = true;
}

function setIsRecordingFalseFn() {
  //console.log("set recording to false, previous = " + isRecording)
  isRecording = false;
}

document.querySelector('#loading_placeholder').remove();
document.querySelector('#click-to-start').style.display = 'block';
document.querySelector('#await_click_placeholder').style.display = 'block';

// Wait for user interaction to initialize audio, as per specification.
document.addEventListener('click', (element) => {
  init();
  document.querySelector('#click-to-start').remove();
  document.querySelector('#await_click_placeholder').remove();
  document.querySelector('#app').style.display = 'block';
}, {once: true});

/**
 * Defines overall audio chain and initializes all functionality.
 */
async function init() {
  if (context.state === 'suspended') {
    await context.resume();
  }

  document.querySelector('#state').innerHTML = 'WAITING';

  // Get user's microphone and connect it to the AudioContext.
  const micStream = await navigator.mediaDevices.getUserMedia({
    audio: {
      echoCancellation: false,
      autoGainControl: false,
      noiseSuppression: false,
      latency: 0,
    },
  });
  const micSourceNode = context.createMediaStreamSource(micStream);
  const recordingProperties = {
    numberOfChannels: 1, // micSourceNode.channelCount, // FT8
    sampleRate: context.sampleRate,
    maxFrameCount: context.sampleRate*14, // 15, // FT8
  };
  const recordingNode = await setupRecordingWorkletNode(recordingProperties);

  // We can pass this port across the app
  // and let components handle their relevant messages
  const visualizerCallback = setupVisualizers(recordingNode);
  const recordingCallback = handleRecording(
      recordingNode.port, recordingProperties);

  recordingNode.port.onmessage = (event) => {
    if (event.data.message === 'UPDATE_VISUALIZERS') {
      visualizerCallback(event);
    } else {
      recordingCallback(event);
    }
  };
  micSourceNode
      .connect(recordingNode);
      //.connect(context.destination);


  webft8.start({
    recorder_is_running_fn: () => { return isRecordingFn(); },
    recorder_start_fn: (recording_time) => {
      console.log('recorder_start_fn');
      document.querySelector('#state').innerHTML = isRecording ? 'RECORDING' : 'WAITING';
      if(isRecordingFn()) {
        console.error("Attempted to start recording while system was recording!");
        return;
      }
      setIsRecordingTrueFn();
      recordingNode.port.postMessage({
        message: 'RESTART_RECORDER',
        setRecording: true,
        recordingTime: recording_time,
      });
      document.querySelector('#state').innerHTML = isRecording ? 'RECORDING' : 'WAITING';
    },
  });
  init_website();
}

/**
 * Creates ScriptProcessor to record and track microphone audio.
 * @param {object} recordingProperties
 * @param {number} properties.numberOfChannels
 * @param {number} properties.sampleRate
 * @param {number} properties.maxFrameCount
 * @return {AudioWorkletNode} Recording node related components for the app.
 */
async function setupRecordingWorkletNode(recordingProperties) {
  await context.audioWorklet.addModule('./js/recording-processor.js');
  const WorkletRecordingNode = new AudioWorkletNode(
      context,
      'recording-processor',
      {
        processorOptions: recordingProperties,
      },
  );
  return WorkletRecordingNode;
}

/**
 * Set events and define callbacks for recording start/stop events.
 * @param {MessagePort} processorPort
 *     Processor port to send recording state events to
 * @param {object} recordingProperties Microphone channel count,
 *     for accurate recording length calculations.
 * @param {number} properties.numberOfChannels
 * @param {number} properties.sampleRate
 * @param {number} properties.maxFrameCount
 * @return {function} Callback for recording-related events.
 */
function handleRecording(processorPort, recordingProperties) {
  let recordingLength = 0;

  // If the max length is reached, we can no longer record.
  const recordingEventCallback = async (event) => {
    if (event.data.message === 'MAX_RECORDING_LENGTH_REACHED') {
    }
    if (event.data.message === 'UPDATE_RECORDING_LENGTH') {
      recordingLength = event.data.recordingLength;

      document.querySelector('#data-len').innerHTML =
          Math.round(recordingLength / context.sampleRate * 100)/100;
    }
    if (event.data.message === 'SHARE_RECORDING_BUFFER') {
      console.log("SHARE_RECORDING_BUFFER");
      setIsRecordingFalseFn();
      document.querySelector('#state').innerHTML = isRecording ? 'RECORDING' : 'PROCESSING';
      const recordingBuffer = context.createBuffer(
          recordingProperties.numberOfChannels,
          recordingLength,
          context.sampleRate);

      for (let i = 0; i < recordingProperties.numberOfChannels; i++) {
        recordingBuffer.copyToChannel(event.data.buffer[i], i, 0);
      }

      const blob = createBlobFromAudioBuffer(recordingBuffer, true);
      var arrayBuffer = await blob.arrayBuffer();
      console.log("Sending data to Web Worker...");
      const date_processing_start = new Date();
      var webft8_decode_result = await webft8.decode_ft8_audio_frame_async({ format: "wav", }, new Uint8Array(arrayBuffer));
      console.log("Data received from Web Worker");
      console.log(JSON.stringify(webft8_decode_result, null, 2));
      for(var i = 0; i < webft8_decode_result.decoded.length; i++) {
        webft8_decode_result.decoded[i].ts = date_processing_start;
        webft8_table.add_msg(webft8_decode_result.decoded[i]);
      }
      total_decoded += webft8_decode_result.decoded.length;
      document.querySelector('#decoded_messages_count').textContent =  webft8_decode_result.decoded.length;
      document.querySelector('#decoded_messages_total_count').textContent = total_decoded;
      document.querySelector('#decoded_messages_processing_time').textContent =  webft8_decode_result.stats.processing_time_ms;
    }
  };
  //recordButton.addEventListener('click', (e) => {
    //startRecording();
  //});

  return recordingEventCallback;
}


/**
 * Sets up and handles calculations and rendering for all visualizers.
 * @return {function} Callback for visualizer events from the processor.
 */
function setupVisualizers() {
  const drawRecordingGain = setupRecordingGainVis();
  let initialized = false;
  let gain = 0;
  // Wait for processor to start sending messages before beginning to render.
  const visualizerEventCallback = async (event) => {
    if (event.data.message === 'UPDATE_VISUALIZERS') {
      gain = event.data.gain;
      if (!initialized) {
        initialized = true;
        draw();
      }
    }
  };

  function draw() {
    if (isRecording) {
      drawRecordingGain(gain);
    }

    // Request render frame regardless.
    // If visualizers are disabled, function can still wait for enable.
    requestAnimationFrame(draw);
  }

  return visualizerEventCallback;
}


/**
 * Prepares and defines render function for the recording gain visualizer.
 * @return {function} Draw function to render incoming recorded audio.
 */
function setupRecordingGainVis() {
  const canvas = document.querySelector('#recording-canvas');
  const canvasContext = canvas.getContext('2d');

  const width = canvas.width;
  const height = canvas.height;

  canvasContext.fillStyle = 'red';
  canvasContext.fillRect(0, 0, 1, 1);

  let currentX = 0;

  function draw(currentSampleGain) {
    const compensatedCurrentSampleGain = currentSampleGain * 160;
    const centerY = ((1 - compensatedCurrentSampleGain) * height) / 2;
    const gainHeight = compensatedCurrentSampleGain * height;

    // Clear current Y-axis.
    canvasContext.clearRect(currentX, 0, 1, height);

    // Draw recording bar 1 ahead.
    canvasContext.fillStyle = 'red';
    canvasContext.fillRect(currentX+1, 0, 1, height);

    // Draw current gain.
    canvasContext.fillStyle = 'black';
    canvasContext.fillRect(currentX, centerY, 1, gainHeight);

    if (currentX < width - 2) {
      // Keep drawing new waveforms rightwards until canvas is full.
      currentX++;
    } else {
      // If the waveform fills the canvas,
      // move it by one pixel to the left to make room.
      canvasContext.globalCompositeOperation = 'copy';
      canvasContext.drawImage(canvas, -1, 0);

      // Return to original state, where new visuals
      // are drawn without clearing the canvas.
      canvasContext.globalCompositeOperation = 'source-over';
    }
  }
  return draw;
}



function init_website() {
  var wallClockIntervalID = setInterval((a) => {
    const date = new Date();
    const d_str = date.toLocaleTimeString();
    document.querySelector('#wall_clock').innerHTML = d_str;
}, 250, {});


}