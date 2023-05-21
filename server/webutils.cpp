#include "webutils.h"

void parse_webapi_params(int& argc, char* argv[], webapi_params& params) {
    int num_parsed_args = 0;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--system_prompt") == 0 && i + 1 < argc) {
            params.system_prompt = argv[i + 1];
            num_parsed_args += 2;
            i++;
        }
        else if (std::strcmp(argv[i], "--prompt_prefix") == 0 && i + 1 < argc) {
            params.prompt_prefix = argv[i + 1];
            num_parsed_args += 2;
            i++;
        }
        else if (std::strcmp(argv[i], "--prompt_suffix") == 0 && i + 1 < argc) {
            params.prompt_suffix = argv[i + 1];
            num_parsed_args += 2;
            i++;
        }
        else if (std::strcmp(argv[i], "--public_directory") == 0 && i + 1 < argc) {
            params.public_directory = argv[i + 1];
            num_parsed_args += 2;
            i++;
        }
        else if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            params.port = std::atoi(argv[i + 1]);
            num_parsed_args += 2;
            i++;
        }
        else if (std::strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
            params.host = argv[i + 1];
            num_parsed_args += 2;
            i++;
        }
        else {
            // move the unknown argument to the front of argv
            argv[i - num_parsed_args] = argv[i];
        }
    }

    // update argc to exclude the parsed arguments
    argc -= num_parsed_args;
}
