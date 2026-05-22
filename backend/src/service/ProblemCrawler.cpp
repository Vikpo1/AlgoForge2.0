#include "service/ProblemCrawler.h"

#include "httplib.h"
#include "json.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace algoforge {
namespace service {

namespace {

using json = nlohmann::json;

struct ParsedUrl {
    std::string scheme;
    std::string host;
    std::string pathAndQuery;
};

struct HttpFetchResult {
    int status = 0;
    std::string body;
};

struct ProxyConfig {
    bool enabled = false;
    std::string host;
    int port = 0;
};

struct PythonCrawlerResult {
    bool attempted = false;
    bool success = false;
    domain::Problem problem;
    std::string errorMessage;
};

std::string getEnv(const char* name) {
    const char* value = std::getenv(name);
    return value == nullptr ? "" : std::string(value);
}

std::string trim(const std::string& value) {
    std::size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
        ++first;
    }

    std::size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
        --last;
    }

    return value.substr(first, last - first);
}

bool fileExists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

std::string hexEncode(const std::string& value) {
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (unsigned char ch : value) {
        output << std::setw(2) << static_cast<int>(ch);
    }
    return output.str();
}

std::string quoteCommandArg(const std::string& value) {
#ifdef _WIN32
    std::string quoted = "\"";
    for (char ch : value) {
        if (ch == '"') {
            quoted += "\\\"";
        } else {
            quoted += ch;
        }
    }
    quoted += "\"";
    return quoted;
#else
    std::string quoted = "'";
    for (char ch : value) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted += ch;
        }
    }
    quoted += "'";
    return quoted;
#endif
}

std::optional<std::string> runCommand(const std::string& command) {
    std::array<char, 4096> buffer{};
    std::string output;

#ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif
    if (pipe == nullptr) {
        return std::nullopt;
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }

#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    return output;
}

std::vector<std::string> pythonCommands() {
    std::vector<std::string> commands;
    auto configured = getEnv("ALGOFORGE_CRAWLER_PYTHON");
    if (!configured.empty()) {
        commands.push_back(configured);
    }
    commands.push_back("python3");
    commands.push_back("python");
    return commands;
}

std::vector<std::string> crawlerScriptCandidates() {
    std::vector<std::string> scripts;
    auto configured = getEnv("ALGOFORGE_CRAWLER_SCRIPT");
    if (!configured.empty()) {
        scripts.push_back(configured);
    }
    scripts.push_back("scripts/problem_crawler.py");
    scripts.push_back("backend/scripts/problem_crawler.py");
    scripts.push_back("../scripts/problem_crawler.py");
    return scripts;
}

std::optional<ProxyConfig> loadProxyConfig() {
    std::string raw = getEnv("ALGOFORGE_HTTP_PROXY");
    if (raw.empty()) {
        raw = getEnv("HTTPS_PROXY");
    }
    if (raw.empty()) {
        raw = getEnv("HTTP_PROXY");
    }
    if (raw.empty()) {
        raw = getEnv("https_proxy");
    }
    if (raw.empty()) {
        raw = getEnv("http_proxy");
    }
    if (raw.empty()) {
        return std::nullopt;
    }

    raw = trim(raw);
    raw = std::regex_replace(raw, std::regex(R"(^https?://)", std::regex::icase), "");
    auto at = raw.find('@');
    if (at != std::string::npos) {
        raw = raw.substr(at + 1);
    }
    auto slash = raw.find('/');
    if (slash != std::string::npos) {
        raw = raw.substr(0, slash);
    }

    auto colon = raw.rfind(':');
    if (colon == std::string::npos || colon + 1 >= raw.size()) {
        return std::nullopt;
    }

    try {
        ProxyConfig config;
        config.enabled = true;
        config.host = raw.substr(0, colon);
        config.port = std::stoi(raw.substr(colon + 1));
        return config;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

template <typename Client>
void configureHttpClient(Client& client) {
    client.set_follow_location(true);
    client.set_connection_timeout(8, 0);
    client.set_read_timeout(12, 0);
    client.set_write_timeout(12, 0);

    auto proxy = loadProxyConfig();
    if (proxy.has_value() && proxy->enabled) {
        client.set_proxy(proxy->host, proxy->port);
    }
}

std::string requestErrorMessage(const httplib::Result& response) {
    return "request failed: " + httplib::to_string(response.error());
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool containsIgnoreCase(const std::string& text, const std::string& needle) {
    return toLower(text).find(toLower(needle)) != std::string::npos;
}

void replaceAll(std::string& value, const std::string& from, const std::string& to) {
    std::size_t position = 0;
    while ((position = value.find(from, position)) != std::string::npos) {
        value.replace(position, from.size(), to);
        position += to.size();
    }
}

std::string htmlDecode(std::string value) {
    replaceAll(value, "&nbsp;", " ");
    replaceAll(value, "&quot;", "\"");
    replaceAll(value, "&#34;", "\"");
    replaceAll(value, "&#39;", "'");
    replaceAll(value, "&apos;", "'");
    replaceAll(value, "&lt;", "<");
    replaceAll(value, "&gt;", ">");
    replaceAll(value, "&amp;", "&");
    return value;
}

std::string jsonDecode(std::string value) {
    replaceAll(value, "\\n", "\n");
    replaceAll(value, "\\r", "");
    replaceAll(value, "\\t", "\t");
    replaceAll(value, "\\/", "/");
    replaceAll(value, "\\\"", "\"");
    replaceAll(value, "\\\\", "\\");
    return htmlDecode(value);
}

std::string urlDecode(const std::string& value) {
    std::string result;
    result.reserve(value.size());

    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            const std::string hex = value.substr(i + 1, 2);
            char* end = nullptr;
            long decoded = std::strtol(hex.c_str(), &end, 16);
            if (end != nullptr && *end == '\0') {
                result.push_back(static_cast<char>(decoded));
                i += 2;
                continue;
            }
        }
        if (value[i] == '+') {
            result.push_back(' ');
        } else {
            result.push_back(value[i]);
        }
    }

    return result;
}

std::string stripTags(std::string html) {
    html = std::regex_replace(html, std::regex(R"(<script[\s\S]*?</script>)", std::regex::icase), " ");
    html = std::regex_replace(html, std::regex(R"(<style[\s\S]*?</style>)", std::regex::icase), " ");
    html = std::regex_replace(html, std::regex(R"(<br\s*/?>)", std::regex::icase), "\n");
    html = std::regex_replace(html, std::regex(R"(</(p|div|section|article|h1|h2|h3|h4|li|pre|tr)>)", std::regex::icase), "\n");
    html = std::regex_replace(html, std::regex(R"(<[^>]+>)"), " ");
    html = htmlDecode(html);
    html = std::regex_replace(html, std::regex(R"([ \t\f\v]+)"), " ");
    html = std::regex_replace(html, std::regex(R"(\n\s*\n\s*\n+)"), "\n\n");
    return trim(html);
}

std::optional<std::string> firstMatch(const std::string& text, const std::regex& pattern, int group = 1) {
    std::smatch match;
    if (!std::regex_search(text, match, pattern) || static_cast<int>(match.size()) <= group) {
        return std::nullopt;
    }
    return match[group].str();
}

std::optional<std::string> extractBalancedDivByClass(const std::string& html, const std::string& className) {
    std::regex startPattern(
        "<div[^>]*class=[\"'][^\"']*" + className + "[^\"']*[\"'][^>]*>",
        std::regex::icase
    );
    std::smatch match;
    if (!std::regex_search(html, match, startPattern)) {
        return std::nullopt;
    }

    const std::size_t start = static_cast<std::size_t>(match.position());
    const std::string lowered = toLower(html);
    std::size_t cursor = start;
    int depth = 0;

    while (cursor < html.size()) {
        const std::size_t nextOpen = lowered.find("<div", cursor);
        const std::size_t nextClose = lowered.find("</div", cursor);
        if (nextOpen == std::string::npos && nextClose == std::string::npos) {
            break;
        }

        if (nextOpen != std::string::npos && (nextClose == std::string::npos || nextOpen < nextClose)) {
            ++depth;
            cursor = nextOpen + 4;
        } else {
            --depth;
            const std::size_t closeEnd = lowered.find('>', nextClose);
            if (closeEnd == std::string::npos) {
                break;
            }
            cursor = closeEnd + 1;
            if (depth == 0) {
                return html.substr(start, cursor - start);
            }
        }
    }

    return std::nullopt;
}

std::optional<std::string> extractBalancedDivById(const std::string& html, const std::string& id) {
    std::regex startPattern(
        "<div[^>]*id=[\"']" + id + "[\"'][^>]*>",
        std::regex::icase
    );
    std::smatch match;
    if (!std::regex_search(html, match, startPattern)) {
        return std::nullopt;
    }

    const std::size_t start = static_cast<std::size_t>(match.position());
    const std::string lowered = toLower(html);
    std::size_t cursor = start;
    int depth = 0;

    while (cursor < html.size()) {
        const std::size_t nextOpen = lowered.find("<div", cursor);
        const std::size_t nextClose = lowered.find("</div", cursor);
        if (nextOpen == std::string::npos && nextClose == std::string::npos) {
            break;
        }

        if (nextOpen != std::string::npos && (nextClose == std::string::npos || nextOpen < nextClose)) {
            ++depth;
            cursor = nextOpen + 4;
        } else {
            --depth;
            const std::size_t closeEnd = lowered.find('>', nextClose);
            if (closeEnd == std::string::npos) {
                break;
            }
            cursor = closeEnd + 1;
            if (depth == 0) {
                return html.substr(start, cursor - start);
            }
        }
    }

    return std::nullopt;
}

std::optional<ParsedUrl> parseUrl(const std::string& rawUrl) {
    std::smatch match;
    std::regex pattern(R"(^(https?)://([^/:?#]+)(?::\d+)?([^#]*))", std::regex::icase);
    if (!std::regex_search(rawUrl, match, pattern) || match.size() < 4) {
        return std::nullopt;
    }

    ParsedUrl parsed;
    parsed.scheme = toLower(match[1].str());
    parsed.host = toLower(match[2].str());
    parsed.pathAndQuery = match[3].str().empty() ? "/" : match[3].str();
    return parsed;
}

std::string inferOj(const std::string& host) {
    if (host.find("atcoder.jp") != std::string::npos) {
        return "AtCoder";
    }
    if (host.find("codeforces.com") != std::string::npos) {
        return "Codeforces";
    }
    if (host.find("luogu.com.cn") != std::string::npos || host.find("luogu.com") != std::string::npos) {
        return "Luogu";
    }
    if (host.find("qoj.ac") != std::string::npos || host.find("qoj") != std::string::npos) {
        return "QOJ";
    }
    return "Imported";
}

std::string inferTitleFromUrl(const std::string& url, const std::string& oj) {
    auto slash = url.find_last_of('/');
    std::string id = slash == std::string::npos ? url : url.substr(slash + 1);
    auto query = id.find_first_of("?#");
    if (query != std::string::npos) {
        id = id.substr(0, query);
    }
    id = trim(id);
    if (id.empty()) {
        return "导入题目";
    }
    return oj + " " + id;
}

domain::Problem fallbackProblem(int problemId, const std::string& url, const std::string& oj, const std::string& errorMessage) {
    domain::Problem problem(problemId, inferTitleFromUrl(url, oj), oj, url, "Medium");
    problem.setTags({oj});

    std::ostringstream statement;
    statement << "## 题面\n\n";
    statement << "题面爬取失败，请跳转原题查看。\n\n";
    statement << "- 原题链接：`" << url << "`\n";
    if (!errorMessage.empty()) {
        statement << "- 失败原因：" << errorMessage << "\n";
    }
    problem.setStatementMarkdown(statement.str());
    problem.setInputDescription("题面爬取失败，请跳转原题查看。");
    problem.setOutputDescription("题面爬取失败，请跳转原题查看。");
    problem.setSampleInput("");
    problem.setSampleOutput("");
    return problem;
}

std::optional<HttpFetchResult> fetchUrl(const ParsedUrl& parsed, std::string& errorMessage) {
    httplib::Headers headers = {
        {"User-Agent", "Mozilla/5.0 AlgoForge/0.4 problem crawler"},
        {"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"},
        {"Accept-Language", "zh-CN,zh;q=0.9,en;q=0.8"}
    };

    if (parsed.scheme == "https") {
#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
        errorMessage = "backend was built without HTTPS/OpenSSL support";
        return std::nullopt;
#else
        httplib::SSLClient client(parsed.host, 443);
        configureHttpClient(client);

        auto response = client.Get(parsed.pathAndQuery, headers);
        if (!response) {
            errorMessage = requestErrorMessage(response);
            return std::nullopt;
        }
        if (response->status >= 400) {
            errorMessage = "remote site returned HTTP " + std::to_string(response->status);
            return std::nullopt;
        }
        return HttpFetchResult{response->status, response->body};
#endif
    }

    httplib::Client client(parsed.host, 80);
    configureHttpClient(client);

    auto response = client.Get(parsed.pathAndQuery, headers);
    if (!response) {
        errorMessage = requestErrorMessage(response);
        return std::nullopt;
    }
    if (response->status >= 400) {
        errorMessage = "remote site returned HTTP " + std::to_string(response->status);
        return std::nullopt;
    }

    return HttpFetchResult{response->status, response->body};
}

std::string extractTitleTag(const std::string& html) {
    auto title = firstMatch(html, std::regex(R"(<title[^>]*>([\s\S]*?)</title>)", std::regex::icase)).value_or("");
    title = stripTags(title);
    title = std::regex_replace(title, std::regex(R"(\s*-\s*AtCoder.*$)", std::regex::icase), "");
    title = std::regex_replace(title, std::regex(R"(\s*-\s*Codeforces.*$)", std::regex::icase), "");
    title = std::regex_replace(title, std::regex(R"(\s*-\s*洛谷.*$)", std::regex::icase), "");
    return trim(title);
}

std::string sectionAfterHeading(const std::string& html, const std::string& heading) {
    std::regex pattern(
        "<h[1-4][^>]*>\\s*" + heading + "\\s*</h[1-4]>([\\s\\S]*?)(?=<h[1-4][^>]*>|$)",
        std::regex::icase
    );
    return stripTags(firstMatch(html, pattern).value_or(""));
}

std::string sectionAfterAnyHeading(const std::string& html, const std::vector<std::string>& headings) {
    for (const auto& heading : headings) {
        auto section = sectionAfterHeading(html, heading);
        if (!section.empty()) {
            return section;
        }
    }
    return "";
}

std::vector<std::string> extractPreBlocks(const std::string& html) {
    std::vector<std::string> blocks;
    std::regex pattern(R"(<pre[^>]*>([\s\S]*?)</pre>)", std::regex::icase);
    auto begin = std::sregex_iterator(html.begin(), html.end(), pattern);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        blocks.push_back(stripTags((*it)[1].str()));
    }
    return blocks;
}

domain::Problem parseAtCoder(int problemId, const std::string& url, const std::string& html) {
    auto taskHtml = extractBalancedDivById(html, "task-statement");

    auto title = firstMatch(
        html,
        std::regex(R"(<span[^>]*class=["']h2["'][^>]*>([\s\S]*?)</span>)", std::regex::icase)
    ).value_or(extractTitleTag(html));
    title = stripTags(title);

    domain::Problem problem(problemId, title.empty() ? inferTitleFromUrl(url, "AtCoder") : title, "AtCoder", url, "Medium");
    problem.setTags({"AtCoder"});
    if (!taskHtml.has_value()) {
        return problem;
    }

    std::string task = taskHtml.value();
    problem.setStatementMarkdown("## 题面\n\n" + stripTags(task));
    problem.setInputDescription(sectionAfterHeading(task, "Input"));
    problem.setOutputDescription(sectionAfterHeading(task, "Output"));

    auto samples = extractPreBlocks(task);
    if (samples.size() >= 2) {
        problem.setSampleInput(samples[samples.size() - 2]);
        problem.setSampleOutput(samples[samples.size() - 1]);
    }
    return problem;
}

domain::Problem parseCodeforces(int problemId, const std::string& url, const std::string& html) {
    auto statementHtml = extractBalancedDivByClass(html, "problem-statement");

    auto title = firstMatch(
        statementHtml.value_or(html),
        std::regex(R"(<div[^>]*class=["'][^"']*title[^"']*["'][^>]*>([\s\S]*?)</div>)", std::regex::icase)
    ).value_or(extractTitleTag(html));
    title = stripTags(title);

    domain::Problem problem(problemId, title.empty() ? inferTitleFromUrl(url, "Codeforces") : title, "Codeforces", url, "Medium");
    problem.setTags({"Codeforces"});
    if (!statementHtml.has_value() || containsIgnoreCase(html, "cf-mitigated")) {
        return problem;
    }

    std::string statement = statementHtml.value();
    problem.setStatementMarkdown("## 题面\n\n" + stripTags(statement));

    auto inputBlock = firstMatch(statement, std::regex(R"(<div[^>]*class=["'][^"']*input-specification[^"']*["'][^>]*>([\s\S]*?)</div>)", std::regex::icase));
    auto outputBlock = firstMatch(statement, std::regex(R"(<div[^>]*class=["'][^"']*output-specification[^"']*["'][^>]*>([\s\S]*?)</div>)", std::regex::icase));
    problem.setInputDescription(stripTags(inputBlock.value_or("")));
    problem.setOutputDescription(stripTags(outputBlock.value_or("")));

    auto samples = extractPreBlocks(statement);
    if (samples.size() >= 2) {
        problem.setSampleInput(samples[0]);
        problem.setSampleOutput(samples[1]);
    }
    return problem;
}

std::string jsonStringField(const json& object, const char* key) {
    if (!object.is_object() || !object.contains(key) || !object.at(key).is_string()) {
        return "";
    }
    return object.at(key).get<std::string>();
}

const json* jsonObjectField(const json& object, const char* key) {
    if (!object.is_object() || !object.contains(key) || !object.at(key).is_object()) {
        return nullptr;
    }
    return &object.at(key);
}

std::vector<std::string> jsonStringArrayField(const json& object, const char* key) {
    std::vector<std::string> values;
    if (!object.is_object() || !object.contains(key) || !object.at(key).is_array()) {
        return values;
    }
    for (const auto& item : object.at(key)) {
        if (item.is_string()) {
            values.push_back(item.get<std::string>());
        }
    }
    return values;
}

std::optional<json> parseCrawlerOutput(const std::string& output) {
    auto start = output.find('{');
    auto end = output.rfind('}');
    if (start == std::string::npos || end == std::string::npos || end < start) {
        return std::nullopt;
    }

    try {
        return json::parse(output.substr(start, end - start + 1));
    } catch (const json::exception&) {
        return std::nullopt;
    }
}

domain::Problem problemFromPythonCrawlerJson(int problemId, const std::string& url, const std::string& fallbackOj, const json& payload) {
    auto platform = jsonStringField(payload, "platform");
    if (platform.empty()) {
        platform = fallbackOj;
    }

    auto title = jsonStringField(payload, "title");
    if (title.empty()) {
        title = inferTitleFromUrl(url, platform);
    }

    domain::Problem problem(problemId, title, platform, url, "Medium");

    auto tags = jsonStringArrayField(payload, "tags");
    if (tags.empty()) {
        tags.push_back(platform);
    }
    problem.setTags(tags);

    auto content = jsonStringField(payload, "content");
    auto contentFormat = toLower(jsonStringField(payload, "content_format"));
    if (contentFormat == "html") {
        problem.setStatementMarkdown("## 题面\n\n" + stripTags(content));
    } else {
        problem.setStatementMarkdown(content);
    }

    problem.setInputDescription(jsonStringField(payload, "input_description"));
    problem.setOutputDescription(jsonStringField(payload, "output_description"));
    problem.setSampleInput(jsonStringField(payload, "sample_input"));
    problem.setSampleOutput(jsonStringField(payload, "sample_output"));

    auto statementUrl = jsonStringField(payload, "statement_url");
    if (!statementUrl.empty() && problem.getStatementMarkdown().find("ALGOFORGE_PDF_STATEMENT:") == std::string::npos) {
        problem.setStatementMarkdown("ALGOFORGE_PDF_STATEMENT: " + statementUrl + "\n\n" + problem.getStatementMarkdown());
    }
    return problem;
}

PythonCrawlerResult runPythonCrawler(int problemId, const std::string& url, const std::string& oj) {
    PythonCrawlerResult result;
    if (oj != "Codeforces" && oj != "AtCoder" && oj != "Luogu" && oj != "QOJ") {
        return result;
    }

    std::string scriptPath;
    for (const auto& candidate : crawlerScriptCandidates()) {
        if (fileExists(candidate)) {
            scriptPath = candidate;
            break;
        }
    }
    if (scriptPath.empty()) {
        return result;
    }

    result.attempted = true;
    std::string lastError = "Python crawler did not return JSON";
    const auto urlHex = hexEncode(url);

    for (const auto& python : pythonCommands()) {
        const auto command = quoteCommandArg(python) + " " + quoteCommandArg(scriptPath) + " " + urlHex + " 2>&1";
        auto output = runCommand(command);
        if (!output.has_value()) {
            lastError = "failed to start Python crawler";
            continue;
        }

        auto parsed = parseCrawlerOutput(output.value());
        if (!parsed.has_value()) {
            const auto text = trim(output.value());
            if (!text.empty()) {
                lastError = text;
            }
            continue;
        }

        if (!parsed->value("success", false)) {
            result.errorMessage = parsed->value("error", "Python crawler failed");
            return result;
        }

        result.problem = problemFromPythonCrawlerJson(problemId, url, oj, parsed.value());
        result.success = true;
        return result;
    }

    result.errorMessage = lastError;
    return result;
}

std::optional<domain::Problem> parseLuoguLentilleContext(int problemId, const std::string& url, const std::string& html) {
    auto contextText = firstMatch(
        html,
        std::regex(R"REGEX(<script[^>]*id=["']lentille-context["'][^>]*>([\s\S]*?)</script>)REGEX", std::regex::icase)
    );
    if (!contextText.has_value()) {
        return std::nullopt;
    }

    json root;
    try {
        root = json::parse(contextText.value());
    } catch (const json::exception&) {
        try {
            root = json::parse(htmlDecode(contextText.value()));
        } catch (const json::exception&) {
            return std::nullopt;
        }
    }

    const json* data = jsonObjectField(root, "data");
    const json* payload = data == nullptr ? nullptr : jsonObjectField(*data, "problem");
    if (payload == nullptr) {
        return std::nullopt;
    }

    const json* content = jsonObjectField(*payload, "content");
    if (content == nullptr) {
        content = jsonObjectField(*payload, "contenu");
    }
    if (content == nullptr) {
        return std::nullopt;
    }

    auto pid = jsonStringField(*payload, "pid");
    auto name = jsonStringField(*payload, "name");
    if (name.empty()) {
        name = jsonStringField(*content, "name");
    }

    std::string title;
    if (!pid.empty() && !name.empty()) {
        title = pid + " " + name;
    } else if (!name.empty()) {
        title = name;
    } else if (!pid.empty()) {
        title = pid;
    } else {
        title = extractTitleTag(html);
    }

    domain::Problem problem(problemId, title.empty() ? inferTitleFromUrl(url, "Luogu") : title, "Luogu", url, "Medium");
    problem.setTags({"Luogu"});

    const auto background = jsonStringField(*content, "background");
    const auto description = jsonStringField(*content, "description");
    const auto hint = jsonStringField(*content, "hint");
    const auto input = jsonStringField(*content, "formatI");
    const auto output = jsonStringField(*content, "formatO");

    std::ostringstream statement;
    if (!background.empty()) {
        statement << "## 题目背景\n\n" << background << "\n\n";
    }
    if (!description.empty()) {
        statement << "## 题目描述\n\n" << description << "\n\n";
    }
    if (!hint.empty()) {
        statement << "## 说明/提示\n\n" << hint;
    }

    problem.setStatementMarkdown(trim(statement.str()));
    problem.setInputDescription(input);
    problem.setOutputDescription(output);

    if (payload->contains("samples") && payload->at("samples").is_array() && !payload->at("samples").empty()) {
        const auto& firstSample = payload->at("samples").front();
        if (firstSample.is_array() && firstSample.size() >= 2) {
            if (firstSample.at(0).is_string()) {
                problem.setSampleInput(firstSample.at(0).get<std::string>());
            }
            if (firstSample.at(1).is_string()) {
                problem.setSampleOutput(firstSample.at(1).get<std::string>());
            }
        }
    }

    return problem;
}

domain::Problem parseLuoguArticle(int problemId, const std::string& url, const std::string& html) {
    auto article = firstMatch(
        html,
        std::regex(R"REGEX(<article[^>]*>([\s\S]*?)</article>)REGEX", std::regex::icase)
    );

    auto title = extractTitleTag(html);
    title = std::regex_replace(title, std::regex(R"(\s*-\s*洛谷.*$)", std::regex::icase), "");

    domain::Problem problem(problemId, title.empty() ? inferTitleFromUrl(url, "Luogu") : title, "Luogu", url, "Medium");
    problem.setTags({"Luogu"});
    if (!article.has_value()) {
        return problem;
    }

    const auto articleHtml = article.value();
    const auto background = sectionAfterAnyHeading(articleHtml, {"题目背景"});
    const auto description = sectionAfterAnyHeading(articleHtml, {"题目描述", "Description"});
    const auto input = sectionAfterAnyHeading(articleHtml, {"输入格式", "Input"});
    const auto output = sectionAfterAnyHeading(articleHtml, {"输出格式", "Output"});
    const auto hint = sectionAfterAnyHeading(articleHtml, {"说明/提示", "Hint"});

    std::ostringstream statement;
    if (!background.empty()) {
        statement << "## 题目背景\n\n" << background << "\n\n";
    }
    if (!description.empty()) {
        statement << "## 题目描述\n\n" << description << "\n\n";
    }
    if (!hint.empty()) {
        statement << "## 说明/提示\n\n" << hint;
    }

    const auto statementText = trim(statement.str());
    problem.setStatementMarkdown(statementText.empty() ? "## 题面\n\n" + stripTags(articleHtml) : statementText);
    problem.setInputDescription(input);
    problem.setOutputDescription(output);

    auto samples = extractPreBlocks(articleHtml);
    if (samples.size() >= 2) {
        problem.setSampleInput(samples[0]);
        problem.setSampleOutput(samples[1]);
    }

    return problem;
}

domain::Problem parseLuogu(int problemId, const std::string& url, const std::string& html) {
    auto parsed = parseLuoguLentilleContext(problemId, url, html);
    if (parsed.has_value() && parsed->getStatementMarkdown().size() >= 40) {
        return parsed.value();
    }

    return parseLuoguArticle(problemId, url, html);

}

domain::Problem parseQoj(int problemId, const std::string& url, const std::string& html) {
    auto contentHtml = firstMatch(
        html,
        std::regex(R"(<main[^>]*>([\s\S]*?)</main>)", std::regex::icase)
    );
    if (!contentHtml.has_value()) {
        contentHtml = extractBalancedDivByClass(html, "problem-content");
    }
    if (!contentHtml.has_value()) {
        contentHtml = extractBalancedDivByClass(html, "statement");
    }
    if (!contentHtml.has_value()) {
        contentHtml = firstMatch(
        html,
        std::regex(R"(<div[^>]*class=["'][^"']*(?:statement|problem-content|content)[^"']*["'][^>]*>([\s\S]*?)</div>)", std::regex::icase)
        );
    }

    std::string title = extractTitleTag(html);
    domain::Problem problem(problemId, title.empty() ? inferTitleFromUrl(url, "QOJ") : title, "QOJ", url, "Medium");
    problem.setTags({"QOJ"});
    if (!contentHtml.has_value()) {
        return problem;
    }

    std::string content = contentHtml.value();
    problem.setStatementMarkdown("## 题面\n\n" + stripTags(content));

    auto samples = extractPreBlocks(content);
    if (samples.size() >= 2) {
        problem.setSampleInput(samples[0]);
        problem.setSampleOutput(samples[1]);
    }
    return problem;
}

bool hasUsefulStatement(const domain::Problem& problem) {
    const auto& statement = problem.getStatementMarkdown();
    return statement.size() >= 40 &&
        statement.find("题面爬取失败") == std::string::npos &&
        statement.find("棰橀潰鐖彇澶辫触") == std::string::npos;
}

} // anonymous namespace

ProblemCrawlResult crawlProblem(int problemId, const std::string& rawUrl) {
    const std::string url = trim(rawUrl);
    auto parsed = parseUrl(url);
    const std::string oj = parsed.has_value() ? inferOj(parsed->host) : "Imported";

    if (!parsed.has_value()) {
        return {false, fallbackProblem(problemId, url, oj, "invalid URL"), "invalid URL"};
    }

    auto pythonResult = runPythonCrawler(problemId, url, oj);
    if (pythonResult.success) {
        return {true, pythonResult.problem, ""};
    }

    std::string errorMessage;
    auto fetched = fetchUrl(parsed.value(), errorMessage);
    if (!fetched.has_value()) {
        if (pythonResult.attempted && !pythonResult.errorMessage.empty()) {
            errorMessage = pythonResult.errorMessage;
        }
        return {false, fallbackProblem(problemId, url, oj, errorMessage), errorMessage};
    }

    domain::Problem problem;
    if (oj == "AtCoder") {
        problem = parseAtCoder(problemId, url, fetched->body);
    } else if (oj == "Codeforces") {
        problem = parseCodeforces(problemId, url, fetched->body);
    } else if (oj == "Luogu") {
        problem = parseLuogu(problemId, url, fetched->body);
    } else if (oj == "QOJ") {
        problem = parseQoj(problemId, url, fetched->body);
    } else {
        problem = fallbackProblem(problemId, url, oj, "unsupported OJ");
    }

    if (!hasUsefulStatement(problem)) {
        const std::string fallbackError = pythonResult.attempted && !pythonResult.errorMessage.empty()
            ? pythonResult.errorMessage
            : "page structure was not recognized";
        return {false, fallbackProblem(problemId, url, oj, fallbackError), fallbackError};
    }

    return {true, problem, ""};
}

} // namespace service
} // namespace algoforge
