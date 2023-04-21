#include "third-party/cpp-httplib/httplib.h"
#include <iostream>
#include <string>
#include <sstream>
#include <thread>

void handle_streaming_chat(const httplib::Request& req, httplib::Response& res) {
    // Get request message


    // Stream response
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");

    res.set_chunked_content_provider(
        "text/plain",
        [](size_t offset, httplib::DataSink& sink) {
            std::string input = "split this string by spaces split this string by spaces split this string by spaces split this string by spaces";
            std::stringstream ss(input);

            std::string word;
            while (ss >> word) {
                sink.write(word.c_str(), word.length());
                std::this_thread::sleep_for(std::chrono::milliseconds(80));
            }
            sink.done();
            return true; // return 'false' if you want to cancel the process.
        }
    );
}

int main() {
    // Create an HTTP server
    httplib::Server svr;

    // Add the streaming endpoint
    svr.Get("/stream", handle_streaming_chat);

    // Add a CORS preflight OPTIONS handler
    svr.Options("/*", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.set_header("Access-Control-Max-Age", "86400");
        res.status = 204;
    });

    std::cout << "Listening on port 8080";

    // Start the server
    svr.listen("127.0.0.1", 8080);

    return 0;
}
