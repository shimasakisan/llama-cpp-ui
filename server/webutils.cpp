#include "webutils.h"


void parse_webapi_params(int argc, char** argv, webapi_params& params) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--system-prompt") {
            if (i + 1 < argc) {
                params.system_prompt = argv[i + 1];
                i++;
            }
        }
        else if (arg == "--prompt-prefix") {
            if (i + 1 < argc) {
                params.prompt_prefix = argv[i + 1];
                i++;
            }
        }
        else if (arg == "--prompt-suffix") {
            if (i + 1 < argc) {
                params.prompt_suffix = argv[i + 1];
                i++;
            }
        }
        else if (arg == "--public-directory") {
            if (i + 1 < argc) {
                params.public_directory = argv[i + 1];
                i++;
            }
        }
    }
}
