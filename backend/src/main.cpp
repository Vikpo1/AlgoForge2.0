#include <iostream>
#include <cstdlib>
#include <exception>
#include <string>

#include "httplib.h"
#include "http/Routes.h"

namespace {

int loadPortFromEnv() {
    const char* portText = std::getenv("ALGOFORGE_BACKEND_PORT");
    if (portText == nullptr || std::string(portText).empty()) {
        return 8080;
    }

    try {
        return std::stoi(portText);
    } catch (const std::exception&) {
        return 8080;
    }
}

} // anonymous namespace

int main() {
    const int port = loadPortFromEnv();
    httplib::Server server;

    algoforge::http::registerRoutes(server);

    if (!server.bind_to_port("0.0.0.0", port)) {
        std::cerr
            << "Failed to bind AlgoForge backend on port " << port
            << ". Another backend process may already be using this port."
            << std::endl;
        return 1;
    }

    std::cout << "AlgoForge backend is running at http://localhost:" << port << std::endl;
    std::cout << "Health check: http://localhost:" << port << "/api/health" << std::endl;

    server.listen_after_bind();

    return 0;
}
