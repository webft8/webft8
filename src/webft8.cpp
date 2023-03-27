#include "webft8_decode.h"
#include "webft8_encode.h"

#include <cstdio>
#include <cstdlib>


#include <fstream>
#include <cstdio>
#include <streambuf>
#include <vector>
#include <string>

#include "CLI11.hpp"

#include <unistd.h>



std::string read_whole_file(const std::string& filename) {
    std::ifstream t(filename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    std::string str;
    str.reserve(t.tellg());
    t.seekg(0, std::ios::beg);
    str.assign((std::istreambuf_iterator<char>(t)),
               std::istreambuf_iterator<char>());
    return str;
}

std::vector<std::string> all_test_files() {
    std::vector<std::string> ret = {
        "extra_test_data/2023-03-23T18_14_26.815Z.wav",
        "extra_test_data/telemetry_7FFFFFFFFFFFFFFFFF.wav",
        "extra_test_data/telemetry_711111111111111111.wav"
    };
    const int file_count = 20;
    for(int i=1; i<file_count; i++) {
        std::string fpath = "./ft8_lib/test/wav/websdr_test" + std::to_string(i) + ".wav";
        ret.push_back(fpath);
    }
    return ret;
}

bool process_file(std::string fpath) {
    printf("Reading %s\n", fpath.c_str());
    std::string fdata = read_whole_file(fpath);
    printf(" -> read size: %i\n", (int)fdata.length());
    uint8_t* data = (uint8_t*)fdata.data();
    int len = fdata.length();
    char* ret = webft8_ft8_decode(data, len);
    printf("process_file() => %s\n", ret);
    if(!ret) {
        return false;
    }
    free(ret);
    return true;
}

int main(int argc, char** argv)  {
    CLI::App app{"webft8"};

    std::string filename = "";
    app.add_option("-f,--file", filename, "Input file path");

    std::string input = "";
    app.add_option("-i,--input", input, "Input as string");

    bool test_all = false;
    app.add_flag("-t,--test", test_all, "Test all");

    bool decode_single = false;
    app.add_flag("-d,--decode", decode_single, "decode single file");

    bool encode_single = false;
    app.add_flag("-e,--encode", encode_single, "encode single file");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    if(test_all) {
        for(std::string fname : all_test_files()) {
            bool ok = process_file(fname); 
            if(!ok) {
                return 1;
            }
        }
    } else if(decode_single) {
        bool ok = process_file(filename);
        return ok ? 0 : 1;
    } else if(encode_single) {
        printf("%s\n", pack2json(input.c_str()).c_str());
    } else {
        fprintf(stderr, "%s\n", app.help().c_str());
        return 1;
    }

    return 0;
}