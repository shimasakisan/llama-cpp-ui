#include "third-party/cpp-httplib/httplib.h"
#include "third-party/json/single_include/nlohmann/json.hpp"
#include "llamalib.h"
#include "webutils.h"
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <common.h>
#include "webapi.h"
#include "../frontend/build/index_html.hpp"


using json = nlohmann::json;

LlamaSession* currentSession = NULL;
bool generation_in_progress = false;
bool cancel_generation_requested = false;

void set_cors_headers(httplib::Response& res)
{
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET,POST");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

//void handle_chat(const httplib::Request& req, httplib::Response& res) {
//    set_cors_headers(res);
//
//    if (generation_in_progress) {
//        res.status = 403;
//        res.body = "Generation already in progress, discarding.";
//        fprintf(stderr, "\n[!] Generation in progress, discarded prompt: %s\n", req.body.c_str());
//        return;
//    }
//
//    // Stream response
//    res.set_chunked_content_provider(
//        "text/plain",
//        [req](size_t offset, httplib::DataSink& sink) {
//            generation_in_progress = true;
//
//            auto prompt = req.body;
//            fprintf(stderr, "[+] Processing prompt: '%s'\n", prompt.c_str());
//            currentSession->process_prompt(prompt);
//
//            fprintf(stderr, "[+] Generating response:\n");
//            const char* token;
//            while (true) {
//                if (cancel_generation_requested) {
//                    cancel_generation_requested = false;
//                    fprintf(stderr, "\n[!] Generation cancelled\n");
//                    sink.write("[Generation cancelled]", 23);
//                    break;
//                }
//                token = currentSession->predict_next_token();
//                if (token == NULL) {
//                    break;
//                }
//
//                sink.write(token, strlen(token));
//            }
//
//            sink.done();
//
//            generation_in_progress = false;
//            return true;
//        }
//    );
//}
//
//void handle_test(const httplib::Request& req, httplib::Response& res) {
//    
//    set_cors_headers(res);
//
//    // Stream response
//    res.set_chunked_content_provider(
//        "text/plain",
//        [req](size_t offset, httplib::DataSink& sink) {
//            if (generation_in_progress) return true;
//            generation_in_progress = true;
//
//            for (int i = 1; i < 200; i++) {
//                if (cancel_generation_requested) {
//                    cancel_generation_requested = false;
//                    fprintf(stderr, "\n[!] Generation cancelled\n");
//                    sink.write("[Generation cancelled]", 23);
//                    break;
//                }
//                std::string b("Base ");
//                sink.write(b.c_str(), b.size());
//                std::this_thread::sleep_for(std::chrono::milliseconds(30));
//            }
//
//            sink.done();
//
//            generation_in_progress = false;
//            return true;
//        }
//    );
//}


void handle_completion_json(const httplib::Request& req, httplib::Response& res) {

}

void handle_models_json(const httplib::Request& req, httplib::Response& res) {
    set_cors_headers(res);

    // OpenAI models are required for some frontends to work.
    json response = {
        { "data", json::array({
            {
                { "id", "gpt-35-turbo" },
                { "object", "model" },
                { "owned_by", "organization_owner" },
                //{ "permission", "model_1" },
            },
            {
                { "id", "gpt-4" },
                { "object", "model" },
                { "owned_by", "organization_owner" },
                //{ "permission", "model_1" },
            },
        })}
    };

    auto r = response.dump(2);

    res.set_content(r, "application/json");

    /*
    Response:
    {
      "data": [
        {
          "id": "model-id-0",
          "object": "model",
          "owned_by": "organization-owner",
          "permission": [...]
        },
        {
          "id": "model-id-1",
          "object": "model",
          "owned_by": "organization-owner",
          "permission": [...]
        },
        {
          "id": "model-id-2",
          "object": "model",
          "owned_by": "openai",
          "permission": [...]
        },
      ],
      "object": "list"
    }
    */
}

void handle_chat_json(const httplib::Request& req, httplib::Response& res) {
    json j = json::parse(req.body);

    std::cout << "[+] REQUEST\n";

    auto temperature = j.contains("temperature") ? j["temperature"].get<float>() : 0.2;
    auto stream = j.contains("stream") ? j["stream"].get<bool>() : false;
    if (j.contains("messages")) {
        auto messages = j["messages"];
        if (messages.is_array()) {
            for (auto& msg : messages) {
                auto role = msg["role"].get<std::string>();
                auto content = msg["content"].get<std::string>();
                std::cout << "   " << role << ": " << content << "\n";
            }
        }
    }

    /* Event stream format: 
    https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events#event_stream_format
    */
    set_cors_headers(res);

    if (stream) {
        json response = {
            {"choices", json::array({
                {
                    //{"finish_reason", "length"}
                    {"delta",
                        {
                            {"content", "hey "}
                        }
                    }
                }
            })}
        };

        int i = 0;

        res.set_chunked_content_provider("text/event-stream",
            [i, response](size_t offset, httplib::DataSink& sink) mutable {
                // Evaluate the prompt and stream the result
                bool continues = true;
                auto& choice = response["choices"][0];
                choice["delta"]["content"] = "Sample outputs " + std::to_string(i+1) + " ";
                if (++i == 10) {
                    choice["finish_reason"] = "stop";
                    continues = false;
                }

                auto payload = "data:" + response.dump() + "\n\n";
                std::cerr << "send event:\n" << payload;
                sink.write(payload.c_str(), payload.size());

                if (!continues) sink.done();

                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                
                return continues;
            });
    }
    else {
        // The non-streaming thingy
        //res.set_content(response.dump(2), "application/json");
    }
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

void handle_index_html(const httplib::Request& req, httplib::Response& res) {
    set_cors_headers(res);
    //res.set_header("Content-type", "gzip");
    res.set_content(INDEX_HTML_CONTENT, "text/html");
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
    //svr.Post("/chat",   handle_chat);
    //svr.Post("/test",   handle_test);
    svr.Get("/status",  handle_status);
    svr.Get("/cancel",  handle_cancel);
    svr.Get("/v1/completions", handle_completion_json);
    svr.Post("/v1/chat/completions", handle_chat_json);
    svr.Get("/v1/models", handle_models_json);
    svr.Get("/", handle_index_html);
}

int main(int argc, char** argv) {

    webapi_params webparams;
    parse_webapi_params(argc, argv, webparams);

    gpt_params params;
    gpt_params_parse(argc, argv, params);

    LlamaSession session(&params);

    /*int res = session.load_model();
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
    currentSession = &session;*/

    session.m_prompt_prefix = webparams.prompt_prefix;
    session.m_prompt_suffix = webparams.prompt_suffix;

    // HTTP server

    httplib::Server svr;

    setup_http_server(svr, webparams);

    std::cout << "[+] Listening on http://" << webparams.host << ":" << webparams.port << "\n\n\n";
    svr.listen(webparams.host, webparams.port);

    session.release_model();

    return 0;
}
