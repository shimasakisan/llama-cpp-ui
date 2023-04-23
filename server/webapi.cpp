#include "third-party/cpp-httplib/httplib.h"
#include "llamalib.h"
#include "webutils.h"
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <common.h>
#include "webapi.h"

LlamaSession* currentSession = NULL;
bool generation_in_progress = false;
bool cancel_generation_requested = false;

void set_cors_headers(httplib::Response& res)
{
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET,POST");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

void handle_chat(const httplib::Request& req, httplib::Response& res) {
    set_cors_headers(res);

    if (generation_in_progress) {
        res.status = 403;
        res.body = "Generation already in progress, discarding.";
        fprintf(stderr, "\n[!] Generation in progress, discarded prompt: %s\n", req.body.c_str());
        return;
    }

    // Stream response
    res.set_chunked_content_provider(
        "text/plain",
        [req](size_t offset, httplib::DataSink& sink) {
            generation_in_progress = true;

            auto prompt = req.body;
            fprintf(stderr, "[+] Processing prompt: '%s'\n", prompt.c_str());
            currentSession->process_prompt(prompt);

            fprintf(stderr, "[+] Generating response:\n");
            const char* token;
            while (true) {
                if (cancel_generation_requested) {
                    cancel_generation_requested = false;
                    fprintf(stderr, "\n[!] Generation cancelled\n");
                    sink.write("[Generation cancelled]", 23);
                    break;
                }
                token = currentSession->predict_next_token();
                if (token == NULL) {
                    break;
                }

                sink.write(token, strlen(token));
            }

            sink.done();

            generation_in_progress = false;
            return true;
        }
    );
}

void handle_test(const httplib::Request& req, httplib::Response& res) {
    
    set_cors_headers(res);

    // Stream response
    res.set_chunked_content_provider(
        "text/plain",
        [req](size_t offset, httplib::DataSink& sink) {
            if (generation_in_progress) return true;
            generation_in_progress = true;

            for (int i = 1; i < 200; i++) {
                if (cancel_generation_requested) {
                    cancel_generation_requested = false;
                    fprintf(stderr, "\n[!] Generation cancelled\n");
                    sink.write("[Generation cancelled]", 23);
                    break;
                }
                std::string b("Base ");
                sink.write(b.c_str(), b.size());
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }

            sink.done();

            generation_in_progress = false;
            return true;
        }
    );
}

void handle_cancel(const httplib::Request& req, httplib::Response& res) {
    set_cors_headers(res);
    cancel_generation_requested = true;
    res.set_content("{ \"status\": \"ok\" }", "application/json");
}

void handle_status(const httplib::Request& req, httplib::Response& res) {
    set_cors_headers(res);
    res.set_content("{ \"status\": \"ok\" }", "application/json");
}

void setup_http_server(httplib::Server& svr, webapi_params& webparams) {

    // CORS preflight OPTIONS handler
    svr.Options("/*", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.set_header("Access-Control-Max-Age", "86400");
        res.status = 204;
        });

    // Endpoints
    svr.Post("/chat",   handle_chat);
    svr.Post("/test",   handle_test);
    svr.Get("/status",  handle_status);
    svr.Get("/cancel",  handle_cancel);

    // File server
    if (!webparams.public_directory.empty()) {
        svr.set_mount_point("/", webparams.public_directory);
    }
}

int main(int argc, char** argv) {

    webapi_params webparams;
    parse_webapi_params(argc, argv, webparams);

    gpt_params params;
    gpt_params_parse(argc, argv, params);

    LlamaSession session(&params);

    int res = session.load_model();
    if (res) {
        fprintf(stderr, "[!] Load model failed");
        return 12;
    }

    std::string initial_prompt = 
        webparams.system_prompt.empty() ? 
            params.prompt : webparams.system_prompt;

    fprintf(stderr, "[+] Processing initial prompt: %s", initial_prompt.c_str());

    res = session.process_prompt(initial_prompt);
    if (res) {
        return 13;
    }

    fprintf(stderr, "\n[+] Initial prompt processed. Ready to accept requests.\n");
    currentSession = &session;

    session.m_prompt_prefix = webparams.prompt_prefix;
    session.m_prompt_suffix = webparams.prompt_suffix;

    // HTTP server

    httplib::Server svr;

    setup_http_server(svr, webparams);

    std::cout << "[+] Listening on port 8080\n";
    svr.listen("127.0.0.1", 8080);

    session.release_model();

    return 0;
}
