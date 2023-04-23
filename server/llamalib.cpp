#include "llamalib.h"

int LlamaSession::load_model() {

    // load the model
    auto llama_params = llama_context_default_params();

    llama_params.n_ctx = m_params->n_ctx;
    llama_params.n_parts = m_params->n_parts;
    llama_params.seed = m_params->seed;
    llama_params.f16_kv = m_params->memory_f16;
    llama_params.use_mmap = m_params->use_mmap;
    llama_params.use_mlock = m_params->use_mlock;

    m_ctx = llama_init_from_file(m_params->model.c_str(), llama_params);
        
    if (m_ctx == NULL) {
        fprintf(stderr, "%s: error: failed to load model '%s'\n", __func__, m_params->model.c_str());
        return 10;
    }

    if (!m_params->lora_adapter.empty()) {
        int err = llama_apply_lora_from_file(m_ctx,
            m_params->lora_adapter.c_str(),
            m_params->lora_base.empty() ? NULL : m_params->lora_base.c_str(),
            m_params->n_threads);
        if (err != 0) {
            fprintf(stderr, "%s: error: failed to apply lora adapter\n", __func__);
            return 11;
        }
    }

    // print system information
    {
        fprintf(stderr, "\n");
        fprintf(stderr, "system_info: n_threads = %d / %d | %s\n",
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

int LlamaSession::process_prompt(const std::string& input, bool include_pre_suffix) {

    if (input.empty()) {
        return 0;
    }

    std::vector<llama_token> input_tokens = ::llama_tokenize(m_ctx, input, true);

    // TODO: partition in batches of mParams.batch_size
    check_context();
    if (llama_eval(m_ctx, input_tokens.data(), input_tokens.size(), m_num_past_tokens, m_params->n_threads)) {
        fprintf(stderr, "%s : failed to eval\n", __func__);
        return 1;
    }

    m_num_past_tokens += input_tokens.size();

    return 0;
}

const char *LlamaSession::predict_next_token() {

    // TODO: Lock on some mutex or maybe just return in case another request is being processed.
    check_context();
    if (llama_eval(m_ctx, &(m_last_tokens->back()), 1, m_num_past_tokens, m_params->n_threads)) {
        fprintf(stderr, "%s : failed to eval\n", __func__);
        return NULL;
    }

    llama_token predicted_token = 0;

    // llama_sample_top_p_top_k to select the token given the specified temperature, topK, topP and repeat params.
    // TODO: receive those as params
    {
        predicted_token = llama_sample_top_p_top_k(m_ctx,
            m_last_tokens->data() + llama_n_ctx(m_ctx) - m_params->repeat_last_n,
            m_params->repeat_last_n, m_params->top_k, m_params->top_p,
            m_params->temp, m_params->repeat_penalty);

        m_last_tokens->erase(m_last_tokens->begin());
        m_last_tokens->push_back(predicted_token);        
        m_num_past_tokens++;
    }

    // Check for EOS or report the new token
    if (predicted_token == llama_token_eos()) {
        printf("\n[+] END OF TEXT\n");
        return NULL;
    }
    else {
        auto predicted_text = llama_token_to_str(m_ctx, predicted_token);
        printf("%s", predicted_text);
            
        if (is_reverse_prompt()) {
            printf("\n[+] REVERSE PROMPT\n");
            return NULL;
        }
        
        return predicted_text;

        // ring the embeddings if context size reached, otherwise just add
        //current_batch.push_back(predicted_token);
        /*if (input_tokens.size() > context_size) {
            fprintf(stderr, "Reached end of context size");
            break;
        }*/

    }

    return 0;
}

bool LlamaSession::is_reverse_prompt() {
    // Check lastTokens if they contain the reverse prompt.
    return false;
}

void LlamaSession::check_context() {
    int context_size = llama_n_ctx(m_ctx);
    if (m_num_past_tokens >= context_size) {
        m_num_past_tokens = context_size - 1;
    }
}