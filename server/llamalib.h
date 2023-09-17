#pragma once

#include "llama.h"
#include "common.h"
#include <functional>
#include <fstream>

class LlamaSession {

public:
	LlamaSession(gpt_params* params) : m_params(params) {};

	int load_model();
	int process_prompt(const std::string& input);
	const char* predict_next_token();
	void release_model();

	void deserialize_state(std::istream input_stream);
	void serialize_state(std::ostream output_stream);

	std::string m_prompt_prefix = "";
	std::string m_prompt_suffix = "";

private:
	bool is_reverse_prompt();
	void check_past_tokens();

	llama_context* m_ctx = NULL;
	llama_model* m_model = NULL;
	gpt_params* m_params = NULL;
	std::vector<llama_token>* m_last_tokens = NULL;
	int m_num_past_tokens = 0;
	/*int n_past = 0;
	int n_keep = mParams->n_keep ? mParams->n_keep : input_tokens.size();
	int n_remain = mParams->n_predict;
	int n_consumed = 0;*/
};

