#include <iostream>
#include <string>
#include <string_view>
#include <fmt/format.h>
#include <chrono>
#include <variant>


#include <mln/io/imread.hpp>
#include <mln/io/imsave.hpp>
#include <mln/core/image/ndimage.hpp>


#include <spdlog/spdlog.h>
#include <uWebSockets/App.h>
#include <uWebSockets/ClientApp.h>
#include <cpprest/http_client.h>
#include <CLI/CLI.hpp>

#include "scribo.hpp"
#include "process.hpp"
#include <sstream>



std::string storage_uri;
std::string storage_auth_token;
std::unique_ptr<web::http::client::http_client> storage_client;



namespace proxy 
{

    // Create a connection to the storage server to get the image using cpprestsdk and returns the raw buffer content
    // @exception std::runtime_error if the request fails
    mln::image2d<uint8_t>
    get_image(std::string_view directory, int view)
    {
        std::string endpoint = fmt::format("directories/{}/{}/image", directory, view);

        auto request = web::http::http_request(web::http::methods::GET);
        request.set_request_uri(endpoint);
        request.headers().add("Authorization", storage_auth_token);


        auto data = storage_client->request(request)
            .then([](web::http::http_response response) -> std::vector<uint8_t> {
                if (response.status_code() == web::http::status_codes::OK) {
                    spdlog::info("Image retrieved from storage server.");
                } else {
                    spdlog::error("Image retrieval failed with status code: {}", response.status_code());
                    throw std::runtime_error("Request to storage server failed.");
                }
                return response.extract_vector().get();

        }).get();

        auto sp = std::span{data.data(), data.size()};

        mln::image2d<uint8_t> out;
        mln::io::imread_from_bytes(std::as_bytes(sp), out);
        return out;
    }
}


void process(std::string_view directory, std::variant<std::string_view, int> viewStr, uWS::HttpResponse<false> *res) {
    mln::image2d<uint8_t> image;
    scribo::cleaning_parameters params;
    std::vector<std::byte> buffer;
    int view;
    try {
        if (auto str = std::get_if<std::string_view>(&viewStr))
            view = std::stoi(std::string(*str));
        else
            view = std::get<int>(viewStr);
    } catch (const std::exception& e) {
        spdlog::error("Invalid <view> parameter: {}", e.what());
        res->writeStatus("400 Bad Request")
           ->writeHeader("Content-Type", "text/plain")
           ->end("Invalid view parameter");
        return;
    }
    try {
        spdlog::info("Processing view {} from {}", view, directory);
        auto start = std::chrono::high_resolution_clock::now();
        image = proxy::get_image(directory, view);
        image = scribo::clean_document(image, params);
        mln::io::imsave_to_bytes(image, "jpg", buffer);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        spdlog::info("Processing view {} from {} took {}ms", view, directory, duration.count());
    }
    catch (const std::exception& e) {
        spdlog::error("Internal error: {}", e.what());
        res->writeStatus("500 Internal Server Error")
           ->writeHeader("Content-Type", "text/plain")
           ->end(fmt::format("Internal Server Error ({})", e.what()));
        return;
    }

    auto sp = std::string_view{reinterpret_cast<const char*>(buffer.data()), buffer.size()};

    res->writeStatus("200 OK")
       ->writeHeader("Content-Type", "text/json")
       ->end(sp);      
}





/// @brief  Get the layout of the document
/// The payload of the request should be an image (webp or jpeg) in the body of the request
/// @param res 
/// @param req 
void get_layout(uWS::HttpResponse<false> *res, uWS::HttpRequest *req) {
    std::vector<std::byte> _buffer;

    // Get the json payload from the request and parse it
    res->onData([buffer = &_buffer, res](std::string_view data, bool last) {
        auto tmp = (const std::byte*) data.data();
        buffer->insert(buffer->end(), tmp, tmp + data.size());
        if (last) {
            try {
                mln::image2d<uint8_t> image;
                std::ostringstream ss;
                auto p = params {
                    .display_opts = 0,
                    .denoising = -1,
                    .deskew = true,
                    .bg_suppression = true,
                    .debug = 0,
                    .xheight = -1,
                    .output_path = "",
                    .output_layout_file = "",
                    .json = &ss
                };



                spdlog::info("Extracting layout from image.");
                auto start = std::chrono::high_resolution_clock::now();
                mln::io::imread_from_bytes(*buffer, image);
        
                process(image, p);

                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                spdlog::info("Layout extraction took {}ms", duration.count());
                res->writeStatus("200 OK")
                   ->writeHeader("Content-Type", "text/json")
                   ->end(ss.view());
            }
            catch (const std::exception& e) {
                spdlog::error("Error: {}", e.what());
                res->writeStatus("400 Bad Request")
                   ->writeHeader("Content-Type", "text/plain")
                   ->end(fmt::format("Bad Request: {}", e.what()));
            }
        }
    });

    res->onAborted([]() {
        spdlog::warn("Request aborted.");
    });
}




static const char* usage = R"(Document processing API
    POST /imgproc/layout : Get the layout of the document (Pass the image in the body of the request, returns a json with the layout)
    GET  /imgproc/deskew?directory=<directory>&view=<view> : Process the image from the given directory and view
    POST /imgproc/deskew : Process the image from the given directory and view
    {
        "document": "<directory>",
        "view": <view>
    }
    GET /health_check : Check if the server is running
    GET /imgproc/health_check : Check if the server is running
    GET /imgproc/deskew/health_check : Check if the server is running
    )";

int main(int argc, char* argv[]) {
    uWS::App hub;
    int listen_port;
    std::string prefix;

    // Parse the arguments of the command line
    auto app = CLI::App(usage);
    app.add_option("-s,--storage-uri", storage_uri, "Storage server URI")->default_val("http://localhost:3000");
    app.add_option("-t,--storage-auth-token", storage_auth_token, "Storage server authentication token")->default_val("00000000");
    app.add_option("-p,--port", listen_port, "Port to listen on")->default_val(6969);
    app.add_option("-P,--prefix", prefix, "Prefix for each route");


    CLI11_PARSE(app, argc, argv);


    // Create the storage client
    storage_client = std::make_unique<web::http::client::http_client>(storage_uri);

    auto health_check = [](uWS::HttpResponse<false> *res, uWS::HttpRequest* req) {
        spdlog::info("Request {}:{}.", req->getMethod(), req->getUrl());

        res->writeStatus("200 OK")
           ->writeHeader("Content-Type", "text/plain")
           ->end("OK");
    };


    hub.get(prefix + "/imgproc/deskew", [](uWS::HttpResponse<false> *res, uWS::HttpRequest *req) {
        process(req->getQuery("directory"), req->getQuery("view"), res);
    })
    .post(prefix + "/imgproc/deskew", [](uWS::HttpResponse<false> *res, uWS::HttpRequest*) {
        auto ss = std::make_unique<std::stringstream>();

        // Get the json payload from the request and parse it
        res->onData([res, ss = std::move(ss)](std::string_view data, bool last) {
            *ss << data;
            if (last) {
                try {
                    auto params = web::json::value::parse(*ss);
                    auto directory = params.at("document").as_string();
                    auto view = params.at("view").as_integer();
                    process(directory, view, res);
                }
                catch (const std::exception& e) {
                    spdlog::error("Error: {}", e.what());
                    res->writeStatus("400 Bad Request")
                       ->writeHeader("Content-Type", "text/plain")
                       ->end(fmt::format("Bad Request: {}", e.what()));
                }
            }
        });
        res->onAborted([]() {
            spdlog::warn("Request aborted.");
        });
    })
    .post(prefix + "/imgproc/layout", get_layout)
    .get(prefix + "/health_check", health_check)
    .get(prefix + "/imgproc/health_check", health_check)
    .get(prefix + "/imgproc/deskew/health_check", health_check)
    .any("/*", [](uWS::HttpResponse<false> *res, uWS::HttpRequest* req) {
        spdlog::info("Request to unknown route {}:{}.", req->getMethod(), req->getUrl());
        res->writeStatus("404 Not Found")
           ->writeHeader("Content-Type", "text/plain")
           ->end("Not Found");
    })
    .listen(listen_port, [listen_port](auto *token) {
        if (token) {
            spdlog::info("Listening on port {}", listen_port);
        } else {
            spdlog::error("Failed to listen on port {}", listen_port);
        }
    })
    .run()
    ;
}
