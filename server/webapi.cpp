#include "third-party/cpp-httplib/httplib.h"
#include "llamalib.h"
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <common.h>
#include "webapi.h"

LlamaSession* currentSession = NULL;

void set_cors_headers(httplib::Response& res)
{
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET,POST");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

void handle_chat(const httplib::Request& req, httplib::Response& res) {
    
    set_cors_headers(res);

    // Stream response
    res.set_chunked_content_provider(
        "text/plain",
        [req](size_t offset, httplib::DataSink& sink) {

            //auto prompt = "<|prompter|>" + req.body + "<|assistant|>";
            auto prompt = req.body;

            fprintf(stderr, "[+] Processing prompt: '%s'\n", prompt.c_str());
            currentSession->process_prompt(prompt, false);

            fprintf(stderr, "[+] Generating response:\n");
            const char* token;
            while (true) {
                token = currentSession->predict_next_token();
                if (token == NULL) {
                    break;
                }

                sink.write(token, strlen(token));
            }

            sink.done();
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

            for (int i = 1; i < 200; i++) {
                std::string b("Base ");
                sink.write(b.c_str(), b.size());
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }

            sink.done();
            return true;
        }
    );
}


void handle_status(const httplib::Request& req, httplib::Response& res) {
    set_cors_headers(res);
    res.body = "{ \"status\": \"ok\" }";
}

void setup_http_server(httplib::Server& svr) {

    // CORS preflight OPTIONS handler
    svr.Options("/*", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.set_header("Access-Control-Max-Age", "86400");
        res.status = 204;
        });

    // Endpoints
    svr.Post("/chat", handle_chat);
    svr.Get("/status", handle_status);
    svr.Post("/test", handle_test);
}

int main(int argc, char** argv) {

    gpt_params params;
    if (gpt_params_parse(argc, argv, params) == false) return 11;

    LlamaSession session(&params);

    int res = session.load_model();
    if (res > 0) {
        fprintf(stderr, "Load model returned error");
        return 12;
    }

    res = session.process_prompt(params.prompt, false);
    if (res > 0) {
        fprintf(stderr, "Process initial prompt returned error");
        return 13;
    }

    fprintf(stderr, "\nInitial prompt processed. Ready to accept requests.\n");
    currentSession = &session;


    // HTTP server

    httplib::Server svr;

    setup_http_server(svr);

    std::cout << "Listening on port 8080\n";
    svr.listen("127.0.0.1", 8080);

    session.release_model();

    return 0;
}
