#include "webft8_encode.h"
#include <stdlib.h>
#include <ft8/message.h>
#include <ft8/encode.h>
#include <ft8/constants.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdexcept>

WebFt8Tones pack2tones(const char* message) {
    static_assert(WEBFT8_FT8_TONES_NN == FT8_NN);
    WebFt8Tones ret;
    ret.ok = false;
    ftx_message_t msg;
    ftx_message_rc_t rc = ftx_message_encode(&msg, NULL, message);
    if(rc != FTX_MESSAGE_RC_OK) {
        ret.ok = false;
        return ret;
    }
    ft8_encode(msg.payload, ret.tones);
    ret.ok = true;
    return ret;
}

std::string pack2json(const char* message) {
    WebFt8Tones tones = pack2tones(message);
    if(!tones.ok) {
        throw std::runtime_error("Errors packing ft8 message");
    }
    std::string ret = "{ \"type\": \"ft8\", \"tones\": [ ";
    for (int j = 0; j < WEBFT8_FT8_TONES_NN; ++j){
        ret += std::to_string(tones.tones[j]);
        if(j < (WEBFT8_FT8_TONES_NN-1)) {
            ret += ", ";
        } else {
            ret += " ";
        }
    }
    ret += "], \"message\": \"";
    ret += message;
    ret += "\" }";
    return ret;
}