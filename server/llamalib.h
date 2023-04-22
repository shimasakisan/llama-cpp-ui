#pragma once

#include "llama.h"
#include "common.h"
#include <functional>


class LlamaSession {

public:
	LlamaSession(gpt_params* params) : mParams(params) {};

	int load_model();
	int process_initial_prompt(std::string& input);
	const char* predict_next_token();
	void release_model();

private:
	llama_context* mCtx = NULL;
	gpt_params* mParams = NULL;
	std::vector<llama_token>* mLastTokens = NULL;
	int mNPast = 0;
	/*int n_past = 0;
	int n_keep = mParams->n_keep ? mParams->n_keep : input_tokens.size();
	int n_remain = mParams->n_predict;
	int n_consumed = 0;*/
};

