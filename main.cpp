#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <string>
#include <cstdlib>
#include "httplib.h"
namespace fs = std::filesystem;
struct Config {
 std::string host = "localhost";
 int port = 4312;
 std::string path = ".";
};
bool loadConfig(Config& c) {
 std::ifstream f("httpconfig.txt");
 if (!f) return false;
 getline(f, c.host);
 f >> c.port;
 f.ignore();
 getline(f, c.path);
 return true;
}
void saveConfig(const Config& c) {
 std::ofstream f("httpconfig.txt");
 f << c.host << "\n";
 f << c.port << "\n";
 f << c.path << "\n";
}
std::string get_mime_type(const fs::path& p) {
 auto ext = p.extension().string();
 if (ext == ".html" || ext == ".htm") return "text/html; charset=utf-8";
 if (ext == ".css") return "text/css";
 if (ext == ".js") return "application/javascript";
 if (ext == ".png") return "image/png";
 if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
 if (ext == ".gif") return "image/gif";
 if (ext == ".svg") return "image/svg+xml";
 if (ext == ".txt") return "text/plain; charset=utf-8";
 if (ext == ".json") return "application/json";
 return "application/octet-stream";
}
std::string generate_directory_html(const fs::path& dir, const std::string& url) {
 std::ostringstream html;
 html << R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Index of )" << url << R"(</title>
<link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.1/css/all.min.css" rel="stylesheet">
<style>
body{font-family:Segoe UI,Arial,sans-serif;max-width:1000px;margin:40px auto}ul{list-style:none;padding:0}li{padding:6px 10px}li:hover{background:#f3f3f3}a{text-decoration:none;color:#06c}.icon{width:20px;display:inline-block;margin-right:8px}
</style>
</head>
<body>
<h1>Index of )" << url << R"(</h1>
<ul>)";
 if (url != "/") {
 html << R"(
<li>
<a href="../">
<span class="icon"><i class="fa-solid fa-arrow-left"></i></span>
.. (Parent directory)
</a>
</li>)";
 }
 for (const auto& e : fs::directory_iterator(dir)) {
 std::string name = e.path().filename().u8string();
 bool is_dir = e.is_directory();
 html << "<li><a href=\"" << name;
 if (is_dir) html << "/";
 html << "\"><span class=\"icon\"><i class=\"fa-solid "
 << (is_dir ? "fa-folder" : "fa-file")
 << "\"></i></span>"
 << name;
 if (is_dir) html << "/";
 html << "</a></li>";
 }
 html << "</ul></body></html>";
 return html.str();
}
int main() {
 system("title SimpleHTTP Server");
 std::cout << "SimpleHTTP Server v1.0 - Made by Hruritiz Team\n\n";
 Config cfg;
 if (!loadConfig(cfg)) {
 std::string input;
 std::cout << "Enter host:port [localhost:4312]: ";
 getline(std::cin, input);
 if (!input.empty()) {
 auto p = input.find(':');
 if (p != std::string::npos) {
 cfg.host = input.substr(0, p);
 cfg.port = std::stoi(input.substr(p + 1));
 }
 }
 std::cout << "Enter folder path [current directory]: ";
 getline(std::cin, input);
 if (!input.empty()) cfg.path = input;
 saveConfig(cfg);
 }
 fs::path root = fs::canonical(cfg.path);
 if (!fs::exists(root) || !fs::is_directory(root)) {
 std::cerr << "Directory does not exist\n";
 system("pause");
 return 1;
 }
 httplib::Server server;
 server.Get(R"(^/.*$)", [&](const httplib::Request& req, httplib::Response& res) {
 fs::path rel = fs::u8path(req.path).lexically_normal();
 if (rel.empty() || rel == "/") rel = ".";
 fs::path requested = fs::weakly_canonical(root / rel.relative_path());
 if (requested.string().find(root.string()) != 0) {
 res.status = 403;
 return;
 }
 if (fs::is_directory(requested)) {
 fs::path index = requested / "index.html";
 if (fs::exists(index)) {
 requested = index;
 } else {
 res.set_content(
 generate_directory_html(requested, req.path),
 "text/html; charset=utf-8"
 );
 return;
 }
 }
 if (fs::exists(requested) && fs::is_regular_file(requested)) {
 std::ifstream f(requested, std::ios::binary);
 std::ostringstream ss;
 ss << f.rdbuf();
 res.set_content(ss.str(), get_mime_type(requested));
 return;
 }
 res.status = 404;
 });
 system("title SimpleHTTP Server - Server started");
 std::string url = "http://" + cfg.host + ":" + std::to_string(cfg.port);
 std::cout << "Server started: " << url << "\n";
 std::cout << "Root: " << root << "\n";
 std::cout << "Press Ctrl+C to stop\n\n";
 system(("start \"\" \"" + url + "\"").c_str());
 if (!server.listen(cfg.host.c_str(), cfg.port)) {
 std::cerr << "Could not bind to " << cfg.host << ":" << cfg.port << "\n";
 system("pause");
 return 1;
 }
 return 0;
}
