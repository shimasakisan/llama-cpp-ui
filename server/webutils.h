#pragma once

#include <string>

struct webapi_params {
	std::string system_prompt = "";		// The system prompt to start the chatbot
	std::string prompt_prefix = "";	    // The prompt prefix
	std::string prompt_suffix = "";     // 
	std::string public_directory = "";  // The directory to serve files from
};

void parse_webapi_params(int& argc, char** argv, webapi_params& params);
