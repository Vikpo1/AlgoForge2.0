#include "repository/MysqlReviewRepository.h"

#include "json.hpp"
#include "service/ProblemCrawler.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifdef ALGOFORGE_USE_MYSQL
#include <mysql/mysql.h>
#endif

using json = nlohmann::json;

namespace algoforge {
namespace repository {

namespace {

constexpr int kDefaultUserId = 1;

std::string getEnvOrDefault(const char* name, const std::string& defaultValue) {
    const char* value = std::getenv(name);
    if (value == nullptr || std::string(value).empty()) {
        return defaultValue;
    }
    return value;
}

unsigned int getEnvPortOrDefault(const char* name, unsigned int defaultValue) {
    const char* value = std::getenv(name);
    if (value == nullptr || std::string(value).empty()) {
        return defaultValue;
    }

    try {
        return static_cast<unsigned int>(std::stoul(value));
    } catch (const std::exception&) {
        return defaultValue;
    }
}

domain::ReviewStatus reviewStatusFromString(const std::string& status) {
    if (status == "COOLING") {
        return domain::ReviewStatus::Cooling;
    }
    if (status == "TRAINING") {
        return domain::ReviewStatus::Training;
    }
    if (status == "MASTERED") {
        return domain::ReviewStatus::Mastered;
    }
    if (status == "ARCHIVED") {
        return domain::ReviewStatus::Archived;
    }
    return domain::ReviewStatus::FirstFix;
}

domain::ReviewFeedback reviewFeedbackFromString(const std::string& feedback) {
    if (feedback == "SOLVED_WITH_HINT") {
        return domain::ReviewFeedback::SolvedWithHint;
    }
    if (feedback == "SOLVED_SLOWLY") {
        return domain::ReviewFeedback::SolvedSlowly;
    }
    if (feedback == "SOLVED_FAST") {
        return domain::ReviewFeedback::SolvedFast;
    }
    return domain::ReviewFeedback::Failed;
}

std::vector<std::string> parseTagsJson(const std::string& tagsText) {
    std::vector<std::string> tags;

    if (tagsText.empty()) {
        return tags;
    }

    try {
        auto parsed = json::parse(tagsText);
        if (!parsed.is_array()) {
            return tags;
        }

        for (const auto& tag : parsed) {
            if (tag.is_string()) {
                tags.push_back(tag.get<std::string>());
            }
        }
    } catch (const std::exception&) {
        return tags;
    }

    return tags;
}

#ifdef ALGOFORGE_USE_MYSQL

class MysqlConnection {
public:
    MysqlConnection() {
        auto config = MysqlReviewRepository::loadConfigFromEnv();
        connection_ = mysql_init(nullptr);

        if (connection_ == nullptr) {
            error_ = "mysql_init failed";
            return;
        }

        MYSQL* connected = mysql_real_connect(
            connection_,
            config.host.c_str(),
            config.user.c_str(),
            config.password.c_str(),
            config.database.c_str(),
            config.port,
            nullptr,
            CLIENT_MULTI_STATEMENTS
        );

        if (connected == nullptr) {
            error_ = mysql_error(connection_);
            mysql_close(connection_);
            connection_ = nullptr;
        }

        if (connection_ != nullptr) {
            mysql_set_character_set(connection_, "utf8mb4");
        }
    }

    ~MysqlConnection() {
        if (connection_ != nullptr) {
            mysql_close(connection_);
        }
    }

    bool ok() const {
        return connection_ != nullptr;
    }

    MYSQL* get() const {
        return connection_;
    }

    const std::string& error() const {
        return error_;
    }

private:
    MYSQL* connection_ = nullptr;
    std::string error_;
};

std::string escape(MYSQL* connection, const std::string& value) {
    std::string result;
    result.resize(value.size() * 2 + 1);
    unsigned long length = mysql_real_escape_string(
        connection,
        result.data(),
        value.c_str(),
        static_cast<unsigned long>(value.size())
    );
    result.resize(length);
    return result;
}

std::string limitUtf8Bytes(const std::string& value, std::size_t maxBytes) {
    if (value.size() <= maxBytes) {
        return value;
    }

    std::size_t length = 0;
    while (length < value.size() && length < maxBytes) {
        unsigned char ch = static_cast<unsigned char>(value[length]);
        std::size_t charBytes = 1;
        if ((ch & 0x80) == 0) {
            charBytes = 1;
        } else if ((ch & 0xE0) == 0xC0) {
            charBytes = 2;
        } else if ((ch & 0xF0) == 0xE0) {
            charBytes = 3;
        } else if ((ch & 0xF8) == 0xF0) {
            charBytes = 4;
        }

        if (length + charBytes > maxBytes) {
            break;
        }
        length += charBytes;
    }

    return value.substr(0, length);
}

bool executeSql(MYSQL* connection, const std::string& sql) {
    return mysql_query(connection, sql.c_str()) == 0;
}

bool executeSqlWithLog(MYSQL* connection, const std::string& sql, const std::string& context) {
    if (mysql_query(connection, sql.c_str()) == 0) {
        return true;
    }

    std::cerr << "[AlgoForge][MySQL] " << context << " failed: " << mysql_error(connection) << std::endl;
    return false;
}

std::optional<std::string> querySingleString(MYSQL* connection, const std::string& sql) {
    if (mysql_query(connection, sql.c_str()) != 0) {
        return std::nullopt;
    }

    MYSQL_RES* result = mysql_store_result(connection);
    if (result == nullptr) {
        return std::nullopt;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == nullptr || row[0] == nullptr) {
        mysql_free_result(result);
        return std::nullopt;
    }

    std::string value(row[0]);
    mysql_free_result(result);
    return value;
}

std::vector<int> queryIntList(MYSQL* connection, const std::string& sql) {
    std::vector<int> values;

    if (mysql_query(connection, sql.c_str()) != 0) {
        return values;
    }

    MYSQL_RES* result = mysql_store_result(connection);
    if (result == nullptr) {
        return values;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr) {
        if (row[0] != nullptr) {
            values.push_back(std::stoi(row[0]));
        }
    }

    mysql_free_result(result);
    return values;
}

std::string cell(MYSQL_ROW row, unsigned long* lengths, int index) {
    if (row[index] == nullptr) {
        return "";
    }
    return std::string(row[index], lengths[index]);
}

int cellInt(MYSQL_ROW row, int index) {
    if (row[index] == nullptr) {
        return 0;
    }
    return std::stoi(row[index]);
}

domain::Problem problemFromRow(MYSQL_ROW row, unsigned long* lengths, int offset = 0) {
    domain::Problem problem(
        cellInt(row, offset + 0),
        cell(row, lengths, offset + 1),
        cell(row, lengths, offset + 2),
        cell(row, lengths, offset + 3),
        cell(row, lengths, offset + 4)
    );
    problem.setTags(parseTagsJson(cell(row, lengths, offset + 5)));
    problem.setStatementMarkdown(cell(row, lengths, offset + 6));
    problem.setInputDescription(cell(row, lengths, offset + 7));
    problem.setOutputDescription(cell(row, lengths, offset + 8));
    problem.setSampleInput(cell(row, lengths, offset + 9));
    problem.setSampleOutput(cell(row, lengths, offset + 10));
    return problem;
}

domain::ReviewState reviewStateFromRow(MYSQL_ROW row, unsigned long* lengths, int offset = 0) {
    return domain::ReviewState(
        cellInt(row, offset + 0),
        reviewStatusFromString(cell(row, lengths, offset + 1)),
        cellInt(row, offset + 2),
        cellInt(row, offset + 3),
        reviewFeedbackFromString(cell(row, lengths, offset + 4))
    );
}

std::optional<domain::ReviewCandidate> candidateFromSql(const std::string& sql) {
    MysqlConnection connection;
    if (!connection.ok()) {
        return std::nullopt;
    }

    if (mysql_query(connection.get(), sql.c_str()) != 0) {
        return std::nullopt;
    }

    MYSQL_RES* result = mysql_store_result(connection.get());
    if (result == nullptr) {
        return std::nullopt;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == nullptr) {
        mysql_free_result(result);
        return std::nullopt;
    }

    unsigned long* lengths = mysql_fetch_lengths(result);
    int listId = cellInt(row, 0);
    auto problem = problemFromRow(row, lengths, 1);
    auto state = reviewStateFromRow(row, lengths, 12);

    mysql_free_result(result);
    return domain::ReviewCandidate(listId, problem, state);
}

std::string candidateSelectSql(const std::string& whereClause) {
    std::ostringstream sql;
    sql
        << "SELECT pli.list_id, "
        << "p.id, p.title, p.oj, p.url, p.difficulty, "
        << "CAST(COALESCE(p.tags_json, JSON_ARRAY()) AS CHAR), "
        << "COALESCE(p.statement_markdown, ''), "
        << "COALESCE(p.input_description, ''), "
        << "COALESCE(p.output_description, ''), "
        << "COALESCE(p.sample_input, ''), "
        << "COALESCE(p.sample_output, ''), "
        << "rs.problem_id, rs.status, rs.problem_user_weight, rs.review_count, rs.last_feedback "
        << "FROM problem_list_items pli "
        << "JOIN problems p ON p.id = pli.problem_id "
        << "JOIN review_states rs ON rs.problem_id = p.id AND rs.user_id = " << kDefaultUserId << " "
        << whereClause
        << " ORDER BY p.id";
    return sql.str();
}

std::vector<domain::ReviewCandidate> queryCandidates(const std::string& whereClause) {
    MysqlConnection connection;
    if (!connection.ok()) {
        return {};
    }

    auto sql = candidateSelectSql(whereClause);
    if (mysql_query(connection.get(), sql.c_str()) != 0) {
        return {};
    }

    MYSQL_RES* result = mysql_store_result(connection.get());
    if (result == nullptr) {
        return {};
    }

    std::vector<domain::ReviewCandidate> candidates;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result);
        candidates.push_back(domain::ReviewCandidate(
            cellInt(row, 0),
            problemFromRow(row, lengths, 1),
            reviewStateFromRow(row, lengths, 12)
        ));
    }

    mysql_free_result(result);
    return candidates;
}

int reviewDelayHours(domain::ReviewFeedback feedback) {
    switch (feedback) {
        case domain::ReviewFeedback::Failed:
            return 0;
        case domain::ReviewFeedback::SolvedWithHint:
            return 24;
        case domain::ReviewFeedback::SolvedSlowly:
            return 72;
        case domain::ReviewFeedback::SolvedFast:
            return 168;
        default:
            return 0;
    }
}

domain::Problem buildImportedProblem(int problemId, const std::string& url) {
    return service::crawlProblem(problemId, url).problem;

    auto imported = MockReviewRepository::importProblemToList(1, url);
    if (imported.has_value()) {
        auto problem = imported->getProblem();
        problem.setId(problemId);
        return problem;
    }

    domain::Problem problem(problemId, "导入题目", "Imported", url, "Medium");
    problem.setTags({"基础"});
    problem.setStatementMarkdown("## 题面\n\n这是从原题链接导入的本地记录。");
    problem.setInputDescription("输入格式待补充。");
    problem.setOutputDescription("输出格式待补充。");
    return problem;
}

bool looksLikeFailedCrawlStatement(const std::string& statement) {
    return statement.find("remote site") != std::string::npos ||
        statement.find("request failed:") != std::string::npos ||
        statement.find("page structure was not recognized") != std::string::npos ||
        statement.find("backend was built without HTTPS/OpenSSL support") != std::string::npos ||
        statement.find("invalid URL") != std::string::npos ||
        statement.find("unsupported OJ") != std::string::npos ||
        statement.find("题面爬取失败") != std::string::npos ||
        statement.find("é¢˜é¢ç¬åå¤±è´¥") != std::string::npos ||
        statement.find("棰橀潰鐖") != std::string::npos;
}

bool hasUsefulStoredStatement(const std::string& statement) {
    return statement.size() >= 40 && !looksLikeFailedCrawlStatement(statement);
}

bool writeProblem(MYSQL* connection, const domain::Problem& problem, bool insert) {
    auto tagsJson = json(problem.getTags()).dump();
    const std::string title = limitUtf8Bytes(problem.getTitle(), 240);
    const std::string oj = limitUtf8Bytes(problem.getOj(), 60);
    const std::string url = limitUtf8Bytes(problem.getUrl(), 1024);
    const std::string difficulty = limitUtf8Bytes(problem.getDifficulty(), 30);
    const std::string statement = limitUtf8Bytes(problem.getStatementMarkdown(), 1000000);
    const std::string inputDescription = limitUtf8Bytes(problem.getInputDescription(), 60000);
    const std::string outputDescription = limitUtf8Bytes(problem.getOutputDescription(), 60000);
    const std::string sampleInput = limitUtf8Bytes(problem.getSampleInput(), 60000);
    const std::string sampleOutput = limitUtf8Bytes(problem.getSampleOutput(), 60000);

    if (insert) {
        std::ostringstream insertProblem;
        insertProblem
            << "INSERT INTO problems (id, title, oj, url, difficulty, tags_json, statement_markdown, input_description, output_description, sample_input, sample_output) VALUES ("
            << problem.getId() << ", "
            << "'" << escape(connection, title) << "', "
            << "'" << escape(connection, oj) << "', "
            << "'" << escape(connection, url) << "', "
            << "'" << escape(connection, difficulty) << "', "
            << "CAST('" << escape(connection, tagsJson) << "' AS JSON), "
            << "'" << escape(connection, statement) << "', "
            << "'" << escape(connection, inputDescription) << "', "
            << "'" << escape(connection, outputDescription) << "', "
            << "'" << escape(connection, sampleInput) << "', "
            << "'" << escape(connection, sampleOutput) << "')";

        return executeSqlWithLog(connection, insertProblem.str(), "insert problem");
    }

    std::ostringstream updateProblem;
    updateProblem
        << "UPDATE problems SET "
        << "title = '" << escape(connection, title) << "', "
        << "oj = '" << escape(connection, oj) << "', "
        << "url = '" << escape(connection, url) << "', "
        << "difficulty = '" << escape(connection, difficulty) << "', "
        << "tags_json = CAST('" << escape(connection, tagsJson) << "' AS JSON), "
        << "statement_markdown = '" << escape(connection, statement) << "', "
        << "input_description = '" << escape(connection, inputDescription) << "', "
        << "output_description = '" << escape(connection, outputDescription) << "', "
        << "sample_input = '" << escape(connection, sampleInput) << "', "
        << "sample_output = '" << escape(connection, sampleOutput) << "' "
        << "WHERE id = " << problem.getId();

    return executeSqlWithLog(connection, updateProblem.str(), "update problem");
}

#endif

} // anonymous namespace

MysqlConfig MysqlReviewRepository::loadConfigFromEnv() {
    MysqlConfig config;
    config.host = getEnvOrDefault("ALGOFORGE_DB_HOST", config.host);
    config.port = getEnvPortOrDefault("ALGOFORGE_DB_PORT", config.port);
    config.user = getEnvOrDefault("ALGOFORGE_DB_USER", config.user);
    config.password = getEnvOrDefault("ALGOFORGE_DB_PASSWORD", config.password);
    config.database = getEnvOrDefault("ALGOFORGE_DB_NAME", config.database);
    return config;
}

MysqlConnectionStatus MysqlReviewRepository::testConnection() {
    auto config = loadConfigFromEnv();

    MysqlConnectionStatus status;
    status.host = config.host;
    status.port = config.port;
    status.database = config.database;

#ifndef ALGOFORGE_USE_MYSQL
    status.enabled = false;
    status.connected = false;
    status.message = "MySQL support is not compiled. Rebuild with -DALGOFORGE_USE_MYSQL=ON.";
    return status;
#else
    status.enabled = true;

    MysqlConnection connection;
    if (!connection.ok()) {
        status.connected = false;
        status.message = connection.error();
        return status;
    }

    status.connected = true;
    status.message = "Connected to MySQL";
    return status;
#endif
}

bool MysqlReviewRepository::isAvailable() {
    auto status = testConnection();
    return status.enabled && status.connected;
}

ReviewPool MysqlReviewRepository::loadReviewPool() {
#ifndef ALGOFORGE_USE_MYSQL
    return {};
#else
    return {
        listProblemLists(),
        queryCandidates("WHERE (rs.next_review_at IS NULL OR rs.next_review_at <= NOW())")
    };
#endif
}

std::vector<domain::ProblemList> MysqlReviewRepository::listProblemLists() {
#ifndef ALGOFORGE_USE_MYSQL
    return {};
#else
    MysqlConnection connection;
    if (!connection.ok()) {
        return {};
    }

    std::string sql =
        "SELECT pl.id, pl.name, COALESCE(pl.description, ''), pl.list_user_weight, COUNT(pli.problem_id) "
        "FROM problem_lists pl "
        "LEFT JOIN problem_list_items pli ON pli.list_id = pl.id "
        "WHERE pl.user_id = " + std::to_string(kDefaultUserId) + " "
        "GROUP BY pl.id, pl.name, pl.description, pl.list_user_weight "
        "ORDER BY pl.id";

    if (mysql_query(connection.get(), sql.c_str()) != 0) {
        return {};
    }

    MYSQL_RES* result = mysql_store_result(connection.get());
    if (result == nullptr) {
        return {};
    }

    std::vector<domain::ProblemList> lists;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result);
        domain::ProblemList list(
            cellInt(row, 0),
            cell(row, lengths, 1),
            cell(row, lengths, 2),
            cellInt(row, 3)
        );

        int problemCount = cellInt(row, 4);
        for (int i = 0; i < problemCount; ++i) {
            list.addProblemId(-1 - i);
        }

        lists.push_back(list);
    }

    mysql_free_result(result);
    return lists;
#endif
}

std::optional<domain::ProblemList> MysqlReviewRepository::findProblemList(int listId) {
    for (const auto& list : listProblemLists()) {
        if (list.getId() == listId) {
            return list;
        }
    }
    return std::nullopt;
}

std::optional<domain::ProblemList> MysqlReviewRepository::createProblemList(
    const std::string& name,
    const std::string& description,
    int listUserWeight
) {
#ifndef ALGOFORGE_USE_MYSQL
    return std::nullopt;
#else
    MysqlConnection connection;
    if (!connection.ok()) {
        return std::nullopt;
    }

    std::ostringstream sql;
    sql
        << "INSERT INTO problem_lists (user_id, name, description, list_user_weight) VALUES ("
        << kDefaultUserId << ", "
        << "'" << escape(connection.get(), name) << "', "
        << "'" << escape(connection.get(), description) << "', "
        << listUserWeight << ")";

    if (!executeSql(connection.get(), sql.str())) {
        return std::nullopt;
    }

    int listId = static_cast<int>(mysql_insert_id(connection.get()));
    return findProblemList(listId);
#endif
}

std::optional<domain::ProblemList> MysqlReviewRepository::updateProblemList(
    int listId,
    const std::optional<std::string>& name,
    const std::optional<std::string>& description,
    const std::optional<int>& listUserWeight
) {
#ifndef ALGOFORGE_USE_MYSQL
    return std::nullopt;
#else
    MysqlConnection connection;
    if (!connection.ok()) {
        return std::nullopt;
    }

    std::vector<std::string> assignments;
    if (name.has_value()) {
        assignments.push_back("name = '" + escape(connection.get(), name.value()) + "'");
    }
    if (description.has_value()) {
        assignments.push_back("description = '" + escape(connection.get(), description.value()) + "'");
    }
    if (listUserWeight.has_value()) {
        assignments.push_back("list_user_weight = " + std::to_string(listUserWeight.value()));
    }

    if (assignments.empty()) {
        return findProblemList(listId);
    }

    std::ostringstream sql;
    sql << "UPDATE problem_lists SET ";
    for (std::size_t i = 0; i < assignments.size(); ++i) {
        if (i > 0) {
            sql << ", ";
        }
        sql << assignments[i];
    }
    sql << " WHERE id = " << listId << " AND user_id = " << kDefaultUserId;

    if (!executeSql(connection.get(), sql.str())) {
        return std::nullopt;
    }

    return findProblemList(listId);
#endif
}

bool MysqlReviewRepository::deleteProblemList(int listId) {
#ifndef ALGOFORGE_USE_MYSQL
    return false;
#else
    MysqlConnection connection;
    if (!connection.ok()) {
        return false;
    }

    auto problemIds = queryIntList(
        connection.get(),
        "SELECT pli.problem_id FROM problem_list_items pli "
        "JOIN problem_lists pl ON pl.id = pli.list_id "
        "WHERE pli.list_id = " + std::to_string(listId) +
            " AND pl.user_id = " + std::to_string(kDefaultUserId)
    );

    std::ostringstream sql;
    sql << "DELETE FROM problem_lists WHERE id = " << listId << " AND user_id = " << kDefaultUserId;
    if (!executeSql(connection.get(), sql.str()) || mysql_affected_rows(connection.get()) <= 0) {
        return false;
    }

    for (int problemId : problemIds) {
        executeSql(
            connection.get(),
            "DELETE FROM problems WHERE id = " + std::to_string(problemId) +
                " AND NOT EXISTS (SELECT 1 FROM problem_list_items WHERE problem_id = " +
                std::to_string(problemId) + ")"
        );
    }

    return true;
#endif
}

std::vector<domain::ReviewCandidate> MysqlReviewRepository::listCandidates() {
#ifndef ALGOFORGE_USE_MYSQL
    return {};
#else
    return queryCandidates("");
#endif
}

std::vector<domain::ReviewCandidate> MysqlReviewRepository::listCandidatesByListId(int listId) {
#ifndef ALGOFORGE_USE_MYSQL
    return {};
#else
    MysqlConnection connection;
    if (!connection.ok()) {
        return {};
    }

    auto sql = candidateSelectSql("WHERE pli.list_id = " + std::to_string(listId));
    if (mysql_query(connection.get(), sql.c_str()) != 0) {
        return {};
    }

    MYSQL_RES* result = mysql_store_result(connection.get());
    if (result == nullptr) {
        return {};
    }

    std::vector<domain::ReviewCandidate> candidates;
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(result);
        candidates.push_back(domain::ReviewCandidate(
            cellInt(row, 0),
            problemFromRow(row, lengths, 1),
            reviewStateFromRow(row, lengths, 12)
        ));
    }

    mysql_free_result(result);
    return candidates;
#endif
}

std::optional<domain::ReviewCandidate> MysqlReviewRepository::findCandidateByProblemId(int problemId) {
#ifndef ALGOFORGE_USE_MYSQL
    return std::nullopt;
#else
    return candidateFromSql(candidateSelectSql("WHERE p.id = " + std::to_string(problemId)));
#endif
}

std::optional<domain::Problem> MysqlReviewRepository::findProblemById(int problemId) {
    auto candidate = findCandidateByProblemId(problemId);
    if (!candidate.has_value()) {
        return std::nullopt;
    }
    return candidate->getProblem();
}

std::optional<domain::Note> MysqlReviewRepository::findNoteByProblemId(int problemId) {
#ifndef ALGOFORGE_USE_MYSQL
    return std::nullopt;
#else
    MysqlConnection connection;
    if (!connection.ok()) {
        return std::nullopt;
    }

    std::ostringstream sql;
    sql
        << "SELECT id, problem_id, COALESCE(hint_markdown, ''), COALESCE(note_markdown, '') "
        << "FROM notes WHERE user_id = " << kDefaultUserId
        << " AND problem_id = " << problemId
        << " LIMIT 1";

    if (mysql_query(connection.get(), sql.str().c_str()) != 0) {
        return std::nullopt;
    }

    MYSQL_RES* result = mysql_store_result(connection.get());
    if (result == nullptr) {
        return std::nullopt;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == nullptr) {
        mysql_free_result(result);
        return std::nullopt;
    }

    unsigned long* lengths = mysql_fetch_lengths(result);
    domain::Note note(
        cellInt(row, 0),
        cellInt(row, 1),
        cell(row, lengths, 2),
        cell(row, lengths, 3)
    );

    mysql_free_result(result);
    return note;
#endif
}

std::optional<domain::Note> MysqlReviewRepository::updateNote(
    int problemId,
    const std::string& hintMarkdown,
    const std::string& noteMarkdown
) {
#ifndef ALGOFORGE_USE_MYSQL
    return std::nullopt;
#else
    MysqlConnection connection;
    if (!connection.ok()) {
        return std::nullopt;
    }

    if (!findCandidateByProblemId(problemId).has_value()) {
        return std::nullopt;
    }

    std::ostringstream sql;
    sql
        << "INSERT INTO notes (user_id, problem_id, hint_markdown, note_markdown) VALUES ("
        << kDefaultUserId << ", "
        << problemId << ", "
        << "'" << escape(connection.get(), hintMarkdown) << "', "
        << "'" << escape(connection.get(), noteMarkdown) << "') "
        << "ON DUPLICATE KEY UPDATE "
        << "hint_markdown = VALUES(hint_markdown), "
        << "note_markdown = VALUES(note_markdown), "
        << "updated_at = NOW()";

    if (!executeSqlWithLog(connection.get(), sql.str(), "update note")) {
        return std::nullopt;
    }

    return findNoteByProblemId(problemId);
#endif
}

std::optional<domain::ReviewCandidate> MysqlReviewRepository::importProblemToList(
    int listId,
    const std::string& url
) {
#ifndef ALGOFORGE_USE_MYSQL
    return std::nullopt;
#else
    MysqlConnection connection;
    if (!connection.ok()) {
        return std::nullopt;
    }

    auto escapedUrl = escape(connection.get(), url);
    auto existingProblemId = querySingleString(
        connection.get(),
        "SELECT id FROM problems WHERE url = '" + escapedUrl + "' LIMIT 1"
    );

    int problemId = 0;
    if (existingProblemId.has_value()) {
        problemId = std::stoi(existingProblemId.value());
        auto existingStatement = querySingleString(
            connection.get(),
            "SELECT COALESCE(statement_markdown, '') FROM problems WHERE id = " + std::to_string(problemId) + " LIMIT 1"
        ).value_or("");

        auto crawlResult = service::crawlProblem(problemId, url);
        if (crawlResult.success || !hasUsefulStoredStatement(existingStatement)) {
            if (!writeProblem(connection.get(), crawlResult.problem, false)) {
                return std::nullopt;
            }
        }
    } else {
        auto maxId = querySingleString(connection.get(), "SELECT COALESCE(MAX(id), 299) + 1 FROM problems");
        problemId = maxId.has_value() ? std::stoi(maxId.value()) : 300;
        auto problem = service::crawlProblem(problemId, url).problem;
        if (!writeProblem(connection.get(), problem, true)) {
            return std::nullopt;
        }
    }

    executeSql(
        connection.get(),
        "INSERT IGNORE INTO problem_list_items (list_id, problem_id) VALUES (" +
            std::to_string(listId) + ", " + std::to_string(problemId) + ")"
    );

    executeSql(
        connection.get(),
        "INSERT IGNORE INTO review_states (user_id, problem_id, status, problem_user_weight, review_count, last_feedback) VALUES (" +
            std::to_string(kDefaultUserId) + ", " + std::to_string(problemId) + ", 'FIRST_FIX', 1, 0, 'FAILED')"
    );

    std::string hint = "这是从链接 `" + url + "` 导入的模拟提示。";
    std::string note = "### 导入复盘\n\n该题目前由数据库仓库记录，后续可替换为真实 OJ 抓取结果。";
    executeSql(
        connection.get(),
        "INSERT INTO notes (user_id, problem_id, hint_markdown, note_markdown) VALUES (" +
            std::to_string(kDefaultUserId) + ", " + std::to_string(problemId) + ", '" +
            escape(connection.get(), hint) + "', '" + escape(connection.get(), note) + "') " +
            "ON DUPLICATE KEY UPDATE hint_markdown = VALUES(hint_markdown), note_markdown = VALUES(note_markdown)"
    );

    return findCandidateByProblemId(problemId);
#endif
}

bool MysqlReviewRepository::updateProblemWeight(int problemId, int problemUserWeight) {
#ifndef ALGOFORGE_USE_MYSQL
    return false;
#else
    MysqlConnection connection;
    if (!connection.ok()) {
        return false;
    }

    std::ostringstream sql;
    sql
        << "UPDATE review_states SET problem_user_weight = " << problemUserWeight
        << " WHERE user_id = " << kDefaultUserId
        << " AND problem_id = " << problemId;

    return executeSql(connection.get(), sql.str()) && mysql_affected_rows(connection.get()) > 0;
#endif
}

bool MysqlReviewRepository::updateReviewState(int problemId, const domain::ReviewState& reviewState) {
#ifndef ALGOFORGE_USE_MYSQL
    return false;
#else
    MysqlConnection connection;
    if (!connection.ok()) {
        return false;
    }

    std::ostringstream sql;
    sql
        << "UPDATE review_states SET "
        << "status = '" << domain::reviewStatusToString(reviewState.getStatus()) << "', "
        << "problem_user_weight = " << reviewState.getProblemUserWeight() << ", "
        << "review_count = " << reviewState.getReviewCount() << ", "
        << "last_feedback = '" << domain::reviewFeedbackToString(reviewState.getLastFeedback()) << "', "
        << "next_review_at = DATE_ADD(NOW(), INTERVAL "
        << reviewDelayHours(reviewState.getLastFeedback()) << " HOUR), "
        << "last_reviewed_at = NOW() "
        << "WHERE user_id = " << kDefaultUserId
        << " AND problem_id = " << problemId;

    return executeSql(connection.get(), sql.str()) && mysql_affected_rows(connection.get()) > 0;
#endif
}

bool MysqlReviewRepository::recordJudgeSubmission(
    int problemId,
    const std::string& language,
    const std::string& code,
    const std::string& verdict,
    int runtimeMs,
    int memoryKb,
    const std::string& message,
    const std::string& remoteJudge,
    const std::string& remoteSubmissionId,
    const std::string& remoteSubmissionUrl
) {
#ifndef ALGOFORGE_USE_MYSQL
    return false;
#else
    MysqlConnection connection;
    if (!connection.ok()) {
        return false;
    }
    if (
        verdict != "AC" &&
        verdict != "WA" &&
        verdict != "TLE" &&
        verdict != "RE" &&
        verdict != "CE"
    ) {
        return false;
    }

    const bool hasRemoteColumns = querySingleString(
        connection.get(),
        "SHOW COLUMNS FROM judge_submissions LIKE 'remote_judge'"
    ).has_value();

    if (hasRemoteColumns) {
        std::ostringstream remoteSql;
        remoteSql
            << "INSERT INTO judge_submissions "
            << "(user_id, problem_id, language, code, verdict, runtime_ms, memory_kb, message, "
            << "remote_judge, remote_submission_id, remote_submission_url) VALUES ("
            << kDefaultUserId << ", "
            << problemId << ", "
            << "'" << escape(connection.get(), language) << "', "
            << "'" << escape(connection.get(), code) << "', "
            << "'" << escape(connection.get(), verdict) << "', "
            << runtimeMs << ", "
            << memoryKb << ", "
            << "'" << escape(connection.get(), message) << "', "
            << "'" << escape(connection.get(), remoteJudge) << "', "
            << "'" << escape(connection.get(), remoteSubmissionId) << "', "
            << "'" << escape(connection.get(), remoteSubmissionUrl) << "')";

        return executeSql(connection.get(), remoteSql.str());
    }

    std::ostringstream legacySql;
    legacySql
        << "INSERT INTO judge_submissions "
        << "(user_id, problem_id, language, code, verdict, runtime_ms, memory_kb, message) VALUES ("
        << kDefaultUserId << ", "
        << problemId << ", "
        << "'" << escape(connection.get(), language) << "', "
        << "'" << escape(connection.get(), code) << "', "
        << "'" << escape(connection.get(), verdict) << "', "
        << runtimeMs << ", "
        << memoryKb << ", "
        << "'" << escape(connection.get(), message) << "')";

    return executeSql(connection.get(), legacySql.str());
#endif
}

} // namespace repository
} // namespace algoforge
