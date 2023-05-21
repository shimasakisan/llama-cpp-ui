#pragma once

#include <string>

struct webapi_params {
	std::string system_prompt = "";		// The system prompt to start the chatbot
	std::string prompt_prefix = "";	    // The prompt prefix
	std::string prompt_suffix = "";     // 
	std::string public_directory = "";  // The directory to serve files from
	std::string host = "127.0.0.1";		// The interface to listen on
	int port         = 8080;			// Ths port to listen on
};

void parse_webapi_params(int& argc, char** argv, webapi_params& params);
