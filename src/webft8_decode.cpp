#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>

#include <ft8/decode.h>
#include <ft8/encode.h>
#include <ft8/message.h>

#include <common/common.h>
#include <common/wave.h>
#include <common/monitor.h>
#include <common/audio.h>

// LOG_DEBUG
#define LOG_LEVEL LOG_WARN
#include <ft8/debug.h>

#include <string>

const int kMin_score = 10; // Minimum sync score threshold for candidates
const int kMax_candidates = 140;
const int kLDPC_iterations = 25;

const int kMax_decoded_messages = 50;

const int kFreq_osr = 2; // Frequency oversampling rate (bin subdivision)
const int kTime_osr = 2; // Time oversampling rate (symbol subdivision)

void usage(const char* error_msg)
{
    if (error_msg != NULL)
    {
        fprintf(stderr, "ERROR: %s\n", error_msg);
    }
    fprintf(stderr, "Usage: decode_ft8 [-list|([-ft4] [INPUT|-dev DEVICE])]\n\n");
    fprintf(stderr, "Decode a 15-second (or slighly shorter) WAV file.\n");
}
#define WEBFT8_MIN(a,b) (((a)<(b))?(a):(b))

int webft8_load_wav_from_buffer(float* signal, int* num_samples, int* sample_rate, const uint8_t* wav_data, int wav_data_size)
{
    //printf("DUMPING NON-ZERO WAV DATA, bytes %i:\n", wav_data_size);
    //for(int i=0; i<wav_data_size; i++) {
    printf("DUMPING min(wav_data_size, 50) NON-ZERO WAV DATA, bytes %i:\n", wav_data_size);    
    for(int i=0; i<WEBFT8_MIN(wav_data_size, 50); i++) {
        if(wav_data[i] != 0) {
            printf(" %d, ", wav_data[i]);
        }
    }
    printf("\n");
    
    char subChunk1ID[4];    // = {'f', 'm', 't', ' '};
    uint32_t subChunk1Size; // = 16;    // 16 for PCM
    uint16_t audioFormat;   // = 1;     // PCM = 1
    uint16_t numChannels;   // = 1;
    uint16_t bitsPerSample; // = 16;
    uint32_t sampleRate;
    uint16_t blockAlign; // = numChannels * bitsPerSample / 8;
    uint32_t byteRate;   // = sampleRate * blockAlign;

    char subChunk2ID[4];    // = {'d', 'a', 't', 'a'};
    uint32_t subChunk2Size; // = num_samples * blockAlign;

    char chunkID[4];    // = {'R', 'I', 'F', 'F'};
    uint32_t chunkSize; // = 4 + (8 + subChunk1Size) + (8 + subChunk2Size);
    char format[4];     // = {'W', 'A', 'V', 'E'};

    //FILE* f = fopen(path, "rb");
    //if (f == NULL)
    //    return -1;

    // NOTE: works only on little-endian architecture

    //fread((void*)chunkID, sizeof(chunkID), 1, f);
    int pos = 0;
    memcpy((void*)&chunkID, wav_data, sizeof(chunkID));
    pos += sizeof(chunkID);
    //fread((void*)&chunkSize, sizeof(chunkSize), 1, f);
    memcpy((void*)&chunkSize, wav_data+pos, sizeof(chunkSize));
    pos += sizeof(chunkSize);

    //fread((void*)format, sizeof(format), 1, f);
    memcpy((void*)format, wav_data+pos, sizeof(format));
    pos += sizeof(format);

    //fread((void*)subChunk1ID, sizeof(subChunk1ID), 1, f);
    memcpy((void*)subChunk1ID, wav_data+pos, sizeof(subChunk1ID));
    pos += sizeof(subChunk1ID);

    //fread((void*)&subChunk1Size, sizeof(subChunk1Size), 1, f);
    memcpy((void*)&subChunk1Size, wav_data+pos, sizeof(subChunk1Size));
    pos += sizeof(subChunk1Size);

    if (subChunk1Size != 16){
        printf("INVALID AUDIO FORMAT:subChunk1Size != 16 \n  ");
        return -2;
    }

    //fread((void*)&audioFormat, sizeof(audioFormat), 1, f);
    memcpy((void*)&audioFormat, wav_data+pos, sizeof(audioFormat));
    pos += sizeof(audioFormat); // 1 = pcm, int16_t; 3 = IEEE float
    if(audioFormat == 1) {
        printf("AUDIO FORMAT: %i int16!\n  ", (int)audioFormat);
    } else if(audioFormat == 3) {
        printf("AUDIO FORMAT: %i - float32\n  ", (int)audioFormat);
    } else {
        printf("INVALID AUDIO FORMAT: %i - not int16 nor float32!\n  ", (int)audioFormat);
        return -2;
    }

    //fread((void*)&numChannels, sizeof(numChannels), 1, f);
    memcpy((void*)&numChannels, wav_data+pos, sizeof(numChannels));
    pos += sizeof(numChannels);

    //fread((void*)&sampleRate, sizeof(sampleRate), 1, f);
    memcpy((void*)&sampleRate, wav_data+pos, sizeof(sampleRate));
    pos += sizeof(sampleRate);

    //fread((void*)&byteRate, sizeof(byteRate), 1, f);
    memcpy((void*)&byteRate, wav_data+pos, sizeof(byteRate));
    pos += sizeof(byteRate);

    //fread((void*)&blockAlign, sizeof(blockAlign), 1, f);
    memcpy((void*)&blockAlign, wav_data+pos, sizeof(blockAlign));
    pos += sizeof(blockAlign);

    //fread((void*)&bitsPerSample, sizeof(bitsPerSample), 1, f);
    memcpy((void*)&bitsPerSample, wav_data+pos, sizeof(bitsPerSample));
    pos += sizeof(bitsPerSample);

    if(audioFormat == 1) {
        if ( numChannels != 1 || bitsPerSample != 16) {
            fprintf(stderr, "INVALID AUDIO FORMAT for int16: numChannels != 1 || bitsPerSample != 16 \n  ");
            return -3;
        }
    } else if(audioFormat == 3) {
        if ( numChannels != 1 || bitsPerSample != 32) {
            fprintf(stderr, "INVALID AUDIO FORMAT: for float: numChannels != 1 || bitsPerSample != 32 \n  ");
            return -3;
        }
    } else {
        fprintf(stderr, "INVALID AUDIO FORMAT: %i\n", audioFormat);
        return -3;        
    }


    //fread((void*)subChunk2ID, sizeof(subChunk2ID), 1, f);
    memcpy((void*)subChunk2ID, wav_data+pos, sizeof(subChunk2ID));
    pos += sizeof(subChunk2ID);
    //fread((void*)&subChunk2Size, sizeof(subChunk2Size), 1, f);
    memcpy((void*)&subChunk2Size, wav_data+pos, sizeof(subChunk2Size));
    pos += sizeof(subChunk2Size);

    if (subChunk2Size / blockAlign > *num_samples) {
        printf("INVALID AUDIO FORMAT: (subChunk2Size / blockAlign > *num_samples) \n  ");
        return -4;
    }
        
    if(audioFormat == 1) {
        *num_samples = subChunk2Size / blockAlign;
        *sample_rate = sampleRate;
        int16_t* raw_data = (int16_t*)malloc(*num_samples * blockAlign);
        //fread((void*)raw_data, blockAlign, *num_samples, f);
        memcpy((void*)raw_data, wav_data+pos, *num_samples * blockAlign);
        for (int i = 0; i < *num_samples; i++) {
            signal[i] = raw_data[i] / 32768.0f;
        }
        free(raw_data);
    } else if(audioFormat == 3) {
        *num_samples = subChunk2Size / blockAlign;
        *sample_rate = sampleRate;
        float* raw_data = (float*)malloc(*num_samples * blockAlign);
        memcpy((void*)raw_data, wav_data+pos, *num_samples * blockAlign);
        for (int i = 0; i < *num_samples; i++) {
            signal[i] = raw_data[i];
        }
        free(raw_data);
    }

    printf("load_wav complete. format=%i\n", audioFormat);
    return 0;
    
}



#define CALLSIGN_HASHTABLE_SIZE 1024

static struct
{
    char callsign[12]; ///> Up to 11 symbols of callsign + trailing zeros (always filled)
    uint32_t hash;     ///> 8 MSBs contain the age of callsign; 22 LSBs contain hash value
} callsign_hashtable[CALLSIGN_HASHTABLE_SIZE];

static int callsign_hashtable_size;

void hashtable_init(void)
{
    callsign_hashtable_size = 0;
    memset(callsign_hashtable, 0, sizeof(callsign_hashtable));
}

void hashtable_cleanup(uint8_t max_age)
{
    for (int idx_hash = 0; idx_hash < CALLSIGN_HASHTABLE_SIZE; ++idx_hash)
    {
        if (callsign_hashtable[idx_hash].callsign[0] != '\0')
        {
            uint8_t age = (uint8_t)(callsign_hashtable[idx_hash].hash >> 24);
            if (age > max_age)
            {
                LOG(LOG_INFO, "Removing [%s] from hash table, age = %d\n", callsign_hashtable[idx_hash].callsign, age);
                // free the hash entry
                callsign_hashtable[idx_hash].callsign[0] = '\0';
                callsign_hashtable[idx_hash].hash = 0;
                callsign_hashtable_size--;
            }
            else
            {
                // increase callsign age
                callsign_hashtable[idx_hash].hash = (((uint32_t)age + 1u) << 24) | (callsign_hashtable[idx_hash].hash & 0x3FFFFFu);
            }
        }
    }
}

void hashtable_add(const char* callsign, uint32_t hash)
{
    uint16_t hash10 = (hash >> 12) & 0x3FFu;
    int idx_hash = (hash10 * 23) % CALLSIGN_HASHTABLE_SIZE;
    while (callsign_hashtable[idx_hash].callsign[0] != '\0')
    {
        if (((callsign_hashtable[idx_hash].hash & 0x3FFFFFu) == hash) && (0 == strcmp(callsign_hashtable[idx_hash].callsign, callsign)))
        {
            // reset age
            callsign_hashtable[idx_hash].hash &= 0x3FFFFFu;
            LOG(LOG_DEBUG, "Found a duplicate [%s]\n", callsign);
            return;
        }
        else
        {
            LOG(LOG_DEBUG, "Hash table clash!\n");
            // Move on to check the next entry in hash table
            idx_hash = (idx_hash + 1) % CALLSIGN_HASHTABLE_SIZE;
        }
    }
    callsign_hashtable_size++;
    strncpy(callsign_hashtable[idx_hash].callsign, callsign, 11);
    callsign_hashtable[idx_hash].callsign[11] = '\0';
    callsign_hashtable[idx_hash].hash = hash;
}

bool hashtable_lookup(ftx_callsign_hash_type_t hash_type, uint32_t hash, char* callsign)
{
    uint8_t hash_shift = (hash_type == FTX_CALLSIGN_HASH_10_BITS) ? 12 : (hash_type == FTX_CALLSIGN_HASH_12_BITS ? 10 : 0);
    uint16_t hash10 = (hash >> (12 - hash_shift)) & 0x3FFu;
    int idx_hash = (hash10 * 23) % CALLSIGN_HASHTABLE_SIZE;
    while (callsign_hashtable[idx_hash].callsign[0] != '\0')
    {
        if (((callsign_hashtable[idx_hash].hash & 0x3FFFFFu) >> hash_shift) == hash)
        {
            strcpy(callsign, callsign_hashtable[idx_hash].callsign);
            return true;
        }
        // Move on to check the next entry in hash table
        idx_hash = (idx_hash + 1) % CALLSIGN_HASHTABLE_SIZE;
    }
    callsign[0] = '\0';
    return false;
}

ftx_callsign_hash_interface_t hash_if = {
    .lookup_hash = hashtable_lookup,
    .save_hash = hashtable_add
};

std::string decode(const monitor_t* mon, struct tm* tm_slot_start)
{
    std::string ret = "{ \"decoded\": [ ";
    bool first = true;
    const ftx_waterfall_t* wf = &mon->wf;
    // Find top candidates by Costas sync score and localize them in time and frequency
    ftx_candidate_t candidate_list[kMax_candidates];
    int num_candidates = ftx_find_candidates(wf, kMax_candidates, candidate_list, kMin_score);

    // Hash table for decoded messages (to check for duplicates)
    int num_decoded = 0;
    ftx_message_t decoded[kMax_decoded_messages];
    ftx_message_t* decoded_hashtable[kMax_decoded_messages];

    // Initialize hash table pointers
    for (int i = 0; i < kMax_decoded_messages; ++i)
    {
        decoded_hashtable[i] = NULL;
    }

    // Go over candidates and attempt to decode messages
    for (int idx = 0; idx < num_candidates; ++idx)
    {
        const ftx_candidate_t* cand = &candidate_list[idx];

        float freq_hz = (mon->min_bin + cand->freq_offset + (float)cand->freq_sub / wf->freq_osr) / mon->symbol_period;
        float time_sec = (cand->time_offset + (float)cand->time_sub / wf->time_osr) * mon->symbol_period;

#ifdef WATERFALL_USE_PHASE
        // int resynth_len = 12000 * 16;
        // float resynth_signal[resynth_len];
        // for (int pos = 0; pos < resynth_len; ++pos)
        // {
        //     resynth_signal[pos] = 0;
        // }
        // monitor_resynth(mon, cand, resynth_signal);
        // char resynth_path[80];
        // sprintf(resynth_path, "resynth_%04f_%02.1f.wav", freq_hz, time_sec);
        // save_wav(resynth_signal, resynth_len, 12000, resynth_path);
#endif

        ftx_message_t message;
        ftx_decode_status_t status;
        if (!ftx_decode_candidate(wf, cand, kLDPC_iterations, &message, &status))
        {
            if (status.ldpc_errors > 0)
            {
                LOG(LOG_DEBUG, "LDPC decode: %d errors\n", status.ldpc_errors);
            }
            else if (status.crc_calculated != status.crc_extracted)
            {
                LOG(LOG_DEBUG, "CRC mismatch!\n");
            }
            continue;
        }

        LOG(LOG_DEBUG, "Checking hash table for %4.1fs / %4.1fHz [%d]...\n", time_sec, freq_hz, cand->score);
        int idx_hash = message.hash % kMax_decoded_messages;
        bool found_empty_slot = false;
        bool found_duplicate = false;
        do
        {
            if (decoded_hashtable[idx_hash] == NULL)
            {
                LOG(LOG_DEBUG, "Found an empty slot\n");
                found_empty_slot = true;
            }
            else if ((decoded_hashtable[idx_hash]->hash == message.hash) && (0 == memcmp(decoded_hashtable[idx_hash]->payload, message.payload, sizeof(message.payload))))
            {
                LOG(LOG_DEBUG, "Found a duplicate!\n");
                found_duplicate = true;
            }
            else
            {
                LOG(LOG_DEBUG, "Hash table clash!\n");
                // Move on to check the next entry in hash table
                idx_hash = (idx_hash + 1) % kMax_decoded_messages;
            }
        } while (!found_empty_slot && !found_duplicate);

        if (found_empty_slot)
        {
            // Fill the empty hashtable slot
            memcpy(&decoded[idx_hash], &message, sizeof(message));
            decoded_hashtable[idx_hash] = &decoded[idx_hash];
            ++num_decoded;

            char text[FTX_MAX_MESSAGE_LENGTH];
            ftx_message_rc_t unpack_status = ftx_message_decode(&message, &hash_if, text);
            if (unpack_status != FTX_MESSAGE_RC_OK)
            {
                snprintf(text, sizeof(text), "Error [%d] while unpacking!", (int)unpack_status);
            }
            
            // Fake WSJT-X-like output for now
            float snr = cand->score * 0.5f; // TODO: compute better approximation of SNR
            //printf("%02d%02d%02d %+05.1f %+4.2f %4.0f ~  %s\n",
            //    tm_slot_start->tm_hour, tm_slot_start->tm_min, tm_slot_start->tm_sec,
            //    snr, time_sec, freq_hz, text);
            if(first) {
                first = false;
            } else {
                ret += ", ";
            }
            ret += "{ \"text\": \"" + std::string(text) + \
                    "\", \"snr\": " + std::to_string(snr)  + \
                    ", \"freq_hz\": " + std::to_string(freq_hz) + \
                    ", \"time_sec\": " + std::to_string(time_sec) + " }";
            //printf(" RETURN: %s", ret.c_str());
        }
    }
    LOG(LOG_INFO, "Decoded %d messages, callsign hashtable size %d\n", num_decoded, callsign_hashtable_size);
    hashtable_cleanup(10);
    ret += " ], \"error\": null }";
    printf(" RETURN: %s\n", ret.c_str());
    return ret;
}

// hard-coding max 17 secs of 48kilosamples/s for static array allocation.
#define MAX_PROTO_SECONDS 17
#define MAX_SAMPLE_RATE 48000
#define MAX_STATIC_NUM_SAMPLES (MAX_PROTO_SECONDS * MAX_SAMPLE_RATE)

char* webft8_ft8_decode(uint8_t* wav_data, int size) {
    printf("Will load wave now %p, size=%i!\n", wav_data, (int)size); fflush(stdout);
    LOG(LOG_DEBUG, "webft8_ft8_decode\n");
    //monitor_process(NULL, NULL);
    ftx_protocol_t protocol = FTX_PROTOCOL_FT8;
    int sample_rate = MAX_SAMPLE_RATE;
    int num_samples = MAX_STATIC_NUM_SAMPLES; //slot_period * sample_rate;
    printf("Allocating %i bytes space for float32 samples... \n", (int)(MAX_STATIC_NUM_SAMPLES * sizeof(float))); fflush(stdout);
    float* signal = (float*)malloc(MAX_STATIC_NUM_SAMPLES * sizeof(float));
    bool is_live = false;
    printf("Done, Will load wave now!\n"); fflush(stdout);
    int rc = webft8_load_wav_from_buffer(signal, &num_samples, &sample_rate, wav_data, size);
    if (rc < 0){
        LOG(LOG_ERROR, "ERROR: cannot load wave data of size %i\n", size); fflush(stdout);
        free(signal);
        return NULL;
    }
    LOG(LOG_INFO, "Sample rate %d Hz, %d samples, %.3f seconds\n", sample_rate, num_samples, (double)num_samples / sample_rate); fflush(stdout);
    // Compute FFT over the whole signal and store it
    monitor_t mon;
    monitor_config_t mon_cfg = {
        .f_min = 200,
        .f_max = 3000,
        .sample_rate = sample_rate,
        .time_osr = kTime_osr,
        .freq_osr = kFreq_osr,
        .protocol = protocol
    };

    hashtable_init();

    monitor_init(&mon, &mon_cfg);
    LOG(LOG_DEBUG, "Waterfall allocated %d symbols\n", mon.wf.max_blocks);
    struct tm tm_slot_start = { 0 };
    // Process and accumulate audio data in a monitor/waterfall instance
    for (int frame_pos = 0; frame_pos + mon.block_size <= num_samples; frame_pos += mon.block_size){
        LOG(LOG_DEBUG, "Frame pos: %.3fs\n", (float)(frame_pos + mon.block_size) / sample_rate);
        //fprintf(stderr, "#\n"); fflush(stderr);
        // Process the waveform data frame by frame - you could have a live loop here with data from an audio device
        monitor_process(&mon, signal + frame_pos);
    }
    //fprintf(stderr, "\n");
    LOG(LOG_DEBUG, "Waterfall accumulated %d symbols\n", mon.wf.num_blocks);
    LOG(LOG_INFO, "Max magnitude: %.1f dB\n", mon.max_mag);

    // Decode accumulated data (containing slightly less than a full time slot)
    std::string json_ret = decode(&mon, &tm_slot_start);

    // Reset internal variables for the next time slot
    monitor_reset(&mon);
    monitor_free(&mon);
    char* response = (char*) malloc(json_ret.length()+1);
    response[json_ret.length()] = '\0';
    strcpy(response, json_ret.c_str());
    printf("Free(signal %p)\n", signal); fflush(stdout);
    free(signal);
    return response;
}

