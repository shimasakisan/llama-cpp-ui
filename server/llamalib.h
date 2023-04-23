#pragma once

#include "llama.h"
#include "common.h"
#include <functional>


class LlamaSession {

public:
	LlamaSession(gpt_params* params) : mParams(params) {};

	int load_model();
	int process_prompt(const std::string& input, bool include_pre_suffix);
	const char* predict_next_token();
	void release_model();

	std::string m_prompt_prefix = "";
	std::string m_prompt_suffix = "";

private:
	bool is_reverse_prompt();
	void check_context();

	llama_context* mCtx = NULL;
	gpt_params* mParams = NULL;
	std::vector<llama_token>* mLastTokens = NULL;
	int mNPast = 0;
	/*int n_past = 0;
	int n_keep = mParams->n_keep ? mParams->n_keep : input_tokens.size();
	int n_remain = mParams->n_predict;
	int n_consumed = 0;*/
};

