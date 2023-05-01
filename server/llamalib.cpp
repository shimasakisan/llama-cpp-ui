#include "llamalib.h"

int LlamaSession::load_model() {
    auto llama_params = llama_context_default_params();

    llama_params.n_ctx = m_params->n_ctx;
    llama_params.n_parts = m_params->n_parts;
    llama_params.seed = m_params->seed;
    llama_params.f16_kv = m_params->memory_f16;
    llama_params.use_mmap = m_params->use_mmap;
    llama_params.use_mlock = m_params->use_mlock;

    m_ctx = llama_init_from_file(m_params->model.c_str(), llama_params);
        
    if (m_ctx == NULL) {
        fprintf(stderr, "[!] Failed to load model '%s'\n", m_params->model.c_str());
        return 10;
    }

    if (!m_params->lora_adapter.empty()) {
        int err = llama_apply_lora_from_file(m_ctx,
            m_params->lora_adapter.c_str(),
            m_params->lora_base.empty() ? NULL : m_params->lora_base.c_str(),
            m_params->n_threads);
        if (err != 0) {
            fprintf(stderr, "[!] Failed to apply lora adapter\n");
            return 11;
        }
    }

    // print system information
    {
        fprintf(stderr, "\n");
        fprintf(stderr, "[+] System info: n_threads = %d / %d | %s\n",
            m_params->n_threads, std::thread::hardware_concurrency(), llama_print_system_info());
    }

    if (m_last_tokens != NULL) delete(m_last_tokens);
    m_last_tokens = new std::vector<llama_token>(llama_n_ctx(m_ctx));
    std::fill(m_last_tokens->begin(), m_last_tokens->end(), 0);
    
    return 0;
}

void LlamaSession::release_model() {
    llama_free(m_ctx);
    if (m_last_tokens != NULL) delete(m_last_tokens);
}

int LlamaSession::process_prompt(const std::string& input) {
    if (input.empty()) {
        fprintf(stderr, "[-] Prompt is empty, continue generation with \n");
        return 0;
    }

    int context_size = llama_n_ctx(m_ctx);
    std::string full_input = m_prompt_prefix + input + m_prompt_suffix;

    std::vector<llama_token> input_tokens = ::llama_tokenize(m_ctx, full_input, true);
    if (input_tokens.size() > context_size) {
        fprintf(stderr, "[!] Prompt is too long (%d > %d)\n", (int)input_tokens.size(), context_size);
        return 2;
    }

    // TODO: partition in batches of mParams.batch_size
    check_past_tokens();
    if (llama_eval(m_ctx, input_tokens.data(), input_tokens.size(), m_num_past_tokens, m_params->n_threads)) {
        fprintf(stderr, "[!] Failed to eval\n");
        return 1;
    }

    m_num_past_tokens += input_tokens.size();

    return 0;
}

const char *LlamaSession::predict_next_token() {
    check_past_tokens();
    if (llama_eval(m_ctx, &(m_last_tokens->back()), 1, m_num_past_tokens, m_params->n_threads)) {
        fprintf(stderr, "[!] Failed to eval\n");
        return NULL;
    }

    llama_token predicted_token = 0;

    {
        // TODO: This is the most basic sampling possible: use at least temp, top_k, top_p
        auto logits = llama_get_logits(m_ctx);
        auto n_vocab = llama_n_vocab(m_ctx);
        std::vector<llama_token_data> candidates;
        candidates.reserve(n_vocab);
        for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
            candidates.emplace_back(llama_token_data{ token_id, logits[token_id], 0.0f });
        }
        llama_token_data_array candidates_p = { candidates.data(), candidates.size(), false };
        predicted_token = llama_sample_token(m_ctx, &candidates_p);
        
        //predicted_token = llama_sample_top_p_top_k(m_ctx,
        //    m_last_tokens->data() + llama_n_ctx(m_ctx) - m_params->repeat_last_n,
        //    m_params->repeat_last_n, m_params->top_k, m_params->top_p,
        //    m_params->temp, m_params->repeat_penalty);

        m_last_tokens->erase(m_last_tokens->begin());
        m_last_tokens->push_back(predicted_token);        
        m_num_past_tokens++;
    }

    // Check for EOS or report the new token
    if (predicted_token == llama_token_eos()) {
        printf("\n[+] END OF TEXT\n");
        return NULL;
    }

    auto predicted_text = llama_token_to_str(m_ctx, predicted_token);
    printf("%s", predicted_text);
            
    if (is_reverse_prompt()) {
        printf("\n[+] REVERSE PROMPT\n");
        return NULL;
    }
        
    return predicted_text;
}


void LlamaSession::serialize_state(std::ostream output_stream) {
    // Save the current llama_state, model_name, m_last_tokens, m_num_past_tokens
    // Should be enough to quickly load a previous conversation in context.

    // Could save all the conversation under the same filename (not here) for logging purposes.
}

void LlamaSession::deserialize_state(std::istream input_stream) {

}

bool LlamaSession::is_reverse_prompt() {
    // Check latest tokens if they contain the reverse prompt.
    return false;
}

void LlamaSession::check_past_tokens() {
    int max_past_tokens = llama_n_ctx(m_ctx) - 4;
    if (m_num_past_tokens > max_past_tokens) {
        m_num_past_tokens = max_past_tokens;
    }
}
