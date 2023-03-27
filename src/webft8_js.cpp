#include <emscripten/emscripten.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "unistd.h"

#include "webft8_encode.h"
#include "webft8_decode.h"

#define WEBFT8_EXPORT extern "C"  EMSCRIPTEN_KEEPALIVE

WEBFT8_EXPORT char* webft8_ft8_decode_js(char* json_config_string, uint8_t* data, int size) {
    printf("NATIVE_ENTER: webft8_ft8_decode_js(ptr=%p, size=%i): json_config_string=%s\n", data, size, json_config_string); fflush(stdout);
    #ifdef WEBFT8_DEBUG
        printf("webft8_ft8_decode_js input bytes %i:\n", (int)size);    
        for(int i=0; i<size; i++) {
            printf(" [%i]=%i, ", i, (uint8_t)data[i]); fflush(stdout);
        }
        printf("\n---\n");   fflush(stdout);
    #endif
    char* ret = webft8_ft8_decode(data, size);
    #ifdef WEBFT8_FAKE_LONG_OPERATION
        emscripten_sleep(5);
    #endif
    return ret;
}

struct webft8_buffer {
    size_t size;
    size_t pos;
    uint8_t* data;
};

WEBFT8_EXPORT webft8_buffer* webft8_buffer_create(int size) {
    //printf("webft8_buffer_create(%i)\n", size);
    webft8_buffer* buffer = (webft8_buffer*)malloc(sizeof(webft8_buffer));
    buffer->size = size;
    buffer->pos = 0;
    buffer->data = (uint8_t*)malloc(size);
    #if 0
    printf("webft8_buffer_create(%i) - malloc ok (pointer=%p), accessing memory... \n", size, buffer->data);
    for(int i=0; i<size; i++) {
        buffer->data[i] = (uint8_t)0;
        printf("%i=%i, ", i, (int) buffer->data[i]);
    }
    printf("\n");
    #endif
    //printf("webft8_buffer_create(%i) => %p\n", size, buffer);
    return buffer;
}

WEBFT8_EXPORT int webft8_buffer_size(webft8_buffer* buffer) {
    //printf("webft8_buffer_size(%p) -> %i\n", buffer, (int)buffer->size);
    return (int)buffer->size;
}

WEBFT8_EXPORT int webft8_buffer_pos(webft8_buffer* buffer) {
    //printf("webft8_buffer_pos(%p) -> %i\n", buffer, (int)buffer->pos);
    return (int)buffer->pos;
}

WEBFT8_EXPORT uint8_t* webft8_buffer_data(webft8_buffer* buffer) {
    //printf("webft8_buffer_data(%p) -> %p\n", buffer, buffer->data);
    return buffer->data;
}

WEBFT8_EXPORT void webft8_buffer_dump(webft8_buffer* buffer) {
    printf("webft8_buffer_dump(%p) -> size: %i = [ ", buffer, (int) buffer->size);
    for(int i=0; i<buffer->size; i++) {
        printf(" [%i]=%i, ", i, (uint8_t)buffer->data[i]);
    }
    printf("\n");
    printf("webft8_buffer_pos(%p) -> %i\n", buffer, (int)buffer->pos);
    printf("webft8_buffer_size(%p) -> %i\n", buffer, (int)buffer->size);
    printf("webft8_buffer_data(%p) -> %p\n", buffer, buffer->data);
}

WEBFT8_EXPORT int webft8_buffer_write(webft8_buffer* buffer, uint8_t* data, int size) {
    #ifdef WEBFT8_DEBUG
    printf("webft8_buffer_write(buf=%p, data=%p, size=%i); buffer->pos=%i buffer->size=%i\n", buffer, data, size, (int)buffer->pos, (int)buffer->size);
    #endif
    if(buffer->pos + size > buffer->size) {
        printf("webft8_buffer_write(buf=%p, data=%p, size=%i) - buf->pos > buffer->size now: buffer->pos=%i buffer->size=%i\n", buffer, data, size, (int)buffer->pos, (int)buffer->size);
        fflush(stdout);
    }
    memcpy(buffer->data+buffer->pos, data, size);
    buffer->pos += size;
    #ifdef WEBFT8_DEBUG
    printf("webft8_buffer_write(buf=%p, data=%p, size=%i) - DONE! now: buffer->pos=%i buffer->size=%i\n", buffer, data, size, (int)buffer->pos, (int)buffer->size);
    #endif
    return (int)buffer->pos;
}

WEBFT8_EXPORT void webft8_buffer_destroy(webft8_buffer* buffer) {
    //printf("webft8_buffer_destroy(%p)\n", buffer);
    free(buffer->data);
    free(buffer);
    //printf("webft8_buffer_destroy(%p) - done!\n", buffer);
}