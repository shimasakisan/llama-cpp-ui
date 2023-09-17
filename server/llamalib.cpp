#include "llamalib.h"
#include <tuple>

int LlamaSession::load_model() {

    llama_backend_init(m_params->numa);
    auto res = llama_init_from_gpt_params(*m_params);
    m_model = std::get<0>(res);
    m_ctx = std::get<1>(res);

    if (m_params->cfg_scale > 1.f) {
        struct llama_context_params lparams = llama_context_params_from_gpt_params(*m_params);
        m_ctx_guidance = llama_new_context_with_model(m_model, lparams);
    }

    if (m_model == NULL) {
        fprintf(stderr, "%s : failed to eval\n", __func__);
        return 1;
    }

    if (!m_params->grammar.empty()) {
        m_parsed_grammar = &(std::move(grammar_parser::parse(m_params->grammar.c_str())));
        // will be empty (default) if there are parse errors
        if (m_parsed_grammar->rules.empty()) {
            return 1;
        }
        LOG_TEE("%s: grammar:\n", __func__);
        grammar_parser::print_grammar(stderr, *m_parsed_grammar);
        LOG_TEE("\n");

        {
            auto it = m_params->logit_bias.find(llama_token_eos(m_ctx));
            if (it != m_params->logit_bias.end() && it->second == -INFINITY) {
                LOG_TEE("%s: warning: EOS token is disabled, which will cause most grammars to fail\n", __func__);
            }
        }

        std::vector<const llama_grammar_element*> grammar_rules(m_parsed_grammar->c_rules());
        m_grammar = llama_grammar_init(
            grammar_rules.data(), grammar_rules.size(), m_parsed_grammar->symbol_ids.at("root"));
    }

    m_params->prompt.insert(0, 1, ' ');

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
    llama_free_model(m_model);
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

const std::string LlamaSession::predict_next_token() {
    check_past_tokens();
    if (llama_eval(m_ctx, &(m_last_tokens->back()), 1, m_num_past_tokens, m_params->n_threads)) {
        fprintf(stderr, "[!] Failed to eval\n");
        return std::string{};
    }

    llama_token predicted_token = 0;

    {
        auto logits = llama_get_logits(m_ctx);
        auto n_vocab = llama_n_vocab(m_ctx);
        std::vector<llama_token_data> candidates;
        candidates.reserve(n_vocab);
        for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
            candidates.emplace_back(llama_token_data{ token_id, logits[token_id], 0.0f });
        }
        predicted_token = llama_sample_token(m_ctx, m_ctx_guidance, m_grammar, *m_params, *m_last_tokens, candidates);
        
        m_last_tokens->erase(m_last_tokens->begin());
        m_last_tokens->push_back(predicted_token);        
        m_num_past_tokens++;
    }

    // Check for EOS or report the new token
    if (predicted_token == llama_token_eos(m_ctx)) {
        printf("\n[+] END OF TEXT\n");
        llama_print_timings(m_ctx);
        return std::string{};
    }

    printf("\n[-] token_id: %d: ", predicted_token);

    auto predicted_text = llama_token_to_piece(m_ctx, predicted_token);
    printf("%s", predicted_text.c_str());
            
    if (is_reverse_prompt()) {
        printf("\n[+] REVERSE PROMPT\n");
        return std::string{};
    }
        
    return std::move(predicted_text);
}


void LlamaSession::serialize_state(std::ostream output_stream) {
    
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
