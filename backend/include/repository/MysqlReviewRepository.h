#pragma once

#include "domain/Note.h"
#include "domain/Problem.h"
#include "domain/ProblemList.h"
#include "domain/ReviewCandidate.h"
#include "domain/ReviewState.h"
#include "repository/MockReviewRepository.h"

#include <optional>
#include <string>
#include <vector>

namespace algoforge {
namespace repository {

struct MysqlConfig {
    std::string host = "127.0.0.1";
    unsigned int port = 3306;
    std::string user = "algoforge";
    std::string password = "algoforge";
    std::string database = "algoforge";
};

struct MysqlConnectionStatus {
    bool enabled = false;
    bool connected = false;
    std::string message;
    std::string host;
    unsigned int port = 0;
    std::string database;
};

struct AuthUser {
    int id = 0;
    std::string username;
    std::string email;
};

struct DailyActivity {
    std::string date;
    int count = 0;
};

class MysqlReviewRepository {
public:
    static MysqlConfig loadConfigFromEnv();
    static MysqlConnectionStatus testConnection();

    static bool isAvailable();
    static void setCurrentUserId(int userId);
    static int currentUserId();

    static std::optional<AuthUser> createUser(
        const std::string& username,
        const std::string& password,
        const std::string& email
    );
    static std::optional<AuthUser> authenticateUser(
        const std::string& username,
        const std::string& password
    );
    static std::optional<AuthUser> findUserById(int userId);
    static std::vector<DailyActivity> dailyActivity(int days);

    static ReviewPool loadReviewPool();

    static std::vector<domain::ProblemList> listProblemLists();
    static std::optional<domain::ProblemList> findProblemList(int listId);
    static std::optional<domain::ProblemList> createProblemList(
        const std::string& name,
        const std::string& description,
        int listUserWeight
    );
    static std::optional<domain::ProblemList> updateProblemList(
        int listId,
        const std::optional<std::string>& name,
        const std::optional<std::string>& description,
        const std::optional<int>& listUserWeight
    );
    static bool deleteProblemList(int listId);
    static bool removeProblemFromList(int listId, int problemId);

    static std::vector<domain::ReviewCandidate> listCandidates();
    static std::vector<domain::ReviewCandidate> listCandidatesByListId(int listId);
    static std::optional<domain::ReviewCandidate> findCandidateByProblemId(int problemId);

    static std::optional<domain::Problem> findProblemById(int problemId);
    static std::optional<domain::Note> findNoteByProblemId(int problemId);
    static std::optional<domain::Note> updateNote(
        int problemId,
        const std::string& hintMarkdown,
        const std::string& noteMarkdown
    );

    static std::optional<domain::ReviewCandidate> importProblemToList(
        int listId,
        const std::string& url
    );
    static bool updateProblemWeight(int problemId, int problemUserWeight);
    static bool updateReviewState(int problemId, const domain::ReviewState& reviewState);
    static bool recordReviewFeedback(
        int problemId,
        const std::string& feedback,
        int durationSeconds,
        const std::string& previousStatus,
        const std::string& nextStatus
    );
    static bool recordJudgeSubmission(
        int problemId,
        const std::string& language,
        const std::string& code,
        const std::string& verdict,
        int runtimeMs,
        int memoryKb,
        const std::string& message,
        const std::string& remoteJudge = "",
        const std::string& remoteSubmissionId = "",
        const std::string& remoteSubmissionUrl = ""
    );
};

} // namespace repository
} // namespace algoforge
