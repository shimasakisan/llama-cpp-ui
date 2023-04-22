#include "llamalib.h"

int LlamaSession::load_model() {

    // load the model
    auto llama_params = llama_context_default_params();

    llama_params.n_ctx = mParams->n_ctx;
    llama_params.n_parts = mParams->n_parts;
    llama_params.seed = mParams->seed;
    llama_params.f16_kv = mParams->memory_f16;
    llama_params.use_mmap = mParams->use_mmap;
    llama_params.use_mlock = mParams->use_mlock;

    mCtx = llama_init_from_file(mParams->model.c_str(), llama_params);
        
    if (mCtx == NULL) {
        fprintf(stderr, "%s: error: failed to load model '%s'\n", __func__, mParams->model.c_str());
        return 10;
    }

    if (!mParams->lora_adapter.empty()) {
        int err = llama_apply_lora_from_file(mCtx,
            mParams->lora_adapter.c_str(),
            mParams->lora_base.empty() ? NULL : mParams->lora_base.c_str(),
            mParams->n_threads);
        if (err != 0) {
            fprintf(stderr, "%s: error: failed to apply lora adapter\n", __func__);
            return 11;
        }
    }

    // print system information
    {
        fprintf(stderr, "\n");
        fprintf(stderr, "system_info: n_threads = %d / %d | %s\n",
            mParams->n_threads, std::thread::hardware_concurrency(), llama_print_system_info());
    }

    if (mLastTokens != NULL) delete(mLastTokens);
    mLastTokens = new std::vector<llama_token>(llama_n_ctx(mCtx));
    std::fill(mLastTokens->begin(), mLastTokens->end(), 0);

    return 0;
}

void LlamaSession::release_model() {
    llama_free(mCtx);
    if (mLastTokens != NULL) delete(mLastTokens);
}

int LlamaSession::process_prompt(const std::string& input, bool include_pre_suffix) {

    if (input.empty()) {
        return 0;
    }

    std::vector<llama_token> input_tokens = ::llama_tokenize(mCtx, input, true);

    // TODO: partition in batches of mParams.batch_size
    check_context();
    if (llama_eval(mCtx, input_tokens.data(), input_tokens.size(), mNPast, mParams->n_threads)) {
        fprintf(stderr, "%s : failed to eval\n", __func__);
        return 1;
    }

    mNPast += input_tokens.size();

    return 0;
}

const char *LlamaSession::predict_next_token() {

    // TODO: Lock on some mutex or maybe just return in case another request is being processed.
    check_context();
    if (llama_eval(mCtx, &(mLastTokens->back()), 1, mNPast, mParams->n_threads)) {
        fprintf(stderr, "%s : failed to eval\n", __func__);
        return NULL;
    }

    llama_token predicted_token = 0;

    // llama_sample_top_p_top_k to select the token given the specified temperature, topK, topP and repeat params.
    // TODO: receive those as params
    {
        predicted_token = llama_sample_top_p_top_k(mCtx,
            mLastTokens->data() + llama_n_ctx(mCtx) - mParams->repeat_last_n,
            mParams->repeat_last_n, mParams->top_k, mParams->top_p,
            mParams->temp, mParams->repeat_penalty);

        mLastTokens->erase(mLastTokens->begin());
        mLastTokens->push_back(predicted_token);        
        mNPast++;
    }

    // Check for EOS or report the new token
    if (predicted_token == llama_token_eos()) {
        printf("\n[+] END OF TEXT\n");
        return NULL;
    }
    else {
        auto predicted_text = llama_token_to_str(mCtx, predicted_token);
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
    int context_size = llama_n_ctx(mCtx);
    if (mNPast >= context_size) {
        mNPast = context_size - 1;
    }
}