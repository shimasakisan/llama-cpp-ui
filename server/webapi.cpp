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
}

enum finish_reason {
    fr_cancelled = 10,
    fr_finish = 11,
    fr_length = 12
};

void generate_response_for_prompt(
        std::string& prompt,
        std::function<bool(const std::string&)> onNewToken,
        std::function<void(finish_reason)> onFinish) {
    
    generation_in_progress = true;

    fprintf(stderr, "[+] Processing prompt: '%s'\n", prompt.c_str());
    currentSession->process_prompt(prompt);

    fprintf(stderr, "[+] Generating response:\n");
    while (true) {
        if (cancel_generation_requested) {
            cancel_generation_requested = false;
            fprintf(stderr, "\n[!] Generation cancelled\n");
            onFinish(fr_cancelled);
            break;
        }
        auto token = currentSession->predict_next_token();
        if (token == std::string{}) {
            onFinish(fr_finish);
            break;
        }
        if (!onNewToken(token)) {
            onFinish(fr_cancelled);
            break;
        }
    }

    generation_in_progress = false;
}

void handle_chat_json(const httplib::Request& req, httplib::Response& res) {
    json j = json::parse(req.body);

    std::cout << "[+] REQUEST\n";

    auto temperature = j.contains("temperature") ? j["temperature"].get<float>() : 0.2;
    auto stream = j.contains("stream") ? j["stream"].get<bool>() : false;

    // We only use the latest message as the prompt.
    std::string prompt;
    if (j.contains("messages")) {
        auto& messages = j["messages"];
        if (messages.is_array()) {
            auto& last = messages.back();
            std::cout << "last: " << last << std::endl;
            prompt = last["content"].get<std::string>();  // TODO: Check "role" is "user"
        }
    }

    /* Event stream format: 
    https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events#event_stream_format
    */
    set_cors_headers(res);

    if (stream) {
        res.set_chunked_content_provider("text/event-stream",
            [prompt](size_t offset, httplib::DataSink& sink) mutable {
                
                generate_response_for_prompt(prompt, [&sink](std::string token) {
                        
					    json response = {
				            {"choices", json::array({
					            {
						            {"delta",
							            {
								            {"content", token}
							            }
						            }
					            }
				            })}
					    };
                        auto payload = "data:" + response.dump() + "\n\n";
                        sink.write(payload.c_str(), payload.size());
                        return true;

                    }, [&sink](finish_reason reason) {

                        auto msg = (reason == fr_finish) ? "stop" : "";

                        json response = {
                            {"choices", json::array({
                                {
                                    {"finish_reason", msg}
                                }
                            })}
                        };
                        auto payload = "data:" + response.dump() + "\n\n";
                        sink.write(payload.c_str(), payload.size());

                    });

                sink.done();
                return true;
            });
    }
    else {
        // The non-streaming thingy
        std::stringstream result;
        generate_response_for_prompt(prompt, [&result](std::string token) {
                result << token;
                return true;
            }, [&result](finish_reason reason) {
                // Build the proper response json from result.str()
                //res.set_content(response.dump(2), "application/json");
            });
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

    std::cout << "[+] Listening on http://" << webparams.host << ":" << webparams.port << "\n\n\n";
    svr.listen(webparams.host, webparams.port);

    session.release_model();

    return 0;
}
