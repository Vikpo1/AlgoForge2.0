#include "repository/ReviewRepository.h"

#include "repository/MysqlReviewRepository.h"

namespace algoforge {
namespace repository {

namespace {

bool useMysql() {
    return MysqlReviewRepository::isAvailable();
}

} // anonymous namespace

ReviewPool ReviewRepository::loadReviewPool() {
    return useMysql()
        ? MysqlReviewRepository::loadReviewPool()
        : MockReviewRepository::loadReviewPool();
}

std::vector<domain::ProblemList> ReviewRepository::listProblemLists() {
    return useMysql()
        ? MysqlReviewRepository::listProblemLists()
        : MockReviewRepository::listProblemLists();
}

std::optional<domain::ProblemList> ReviewRepository::findProblemList(int listId) {
    return useMysql()
        ? MysqlReviewRepository::findProblemList(listId)
        : MockReviewRepository::findProblemList(listId);
}

std::optional<domain::ProblemList> ReviewRepository::createProblemList(
    const std::string& name,
    const std::string& description,
    int listUserWeight
) {
    return useMysql()
        ? MysqlReviewRepository::createProblemList(name, description, listUserWeight)
        : MockReviewRepository::createProblemList(name, description, listUserWeight);
}

std::optional<domain::ProblemList> ReviewRepository::updateProblemList(
    int listId,
    const std::optional<std::string>& name,
    const std::optional<std::string>& description,
    const std::optional<int>& listUserWeight
) {
    return useMysql()
        ? MysqlReviewRepository::updateProblemList(listId, name, description, listUserWeight)
        : MockReviewRepository::updateProblemList(listId, name, description, listUserWeight);
}

bool ReviewRepository::deleteProblemList(int listId) {
    return useMysql()
        ? MysqlReviewRepository::deleteProblemList(listId)
        : MockReviewRepository::deleteProblemList(listId);
}

std::vector<domain::ReviewCandidate> ReviewRepository::listCandidates() {
    return useMysql()
        ? MysqlReviewRepository::listCandidates()
        : MockReviewRepository::listCandidates();
}

std::vector<domain::ReviewCandidate> ReviewRepository::listCandidatesByListId(int listId) {
    return useMysql()
        ? MysqlReviewRepository::listCandidatesByListId(listId)
        : MockReviewRepository::listCandidatesByListId(listId);
}

std::optional<domain::ReviewCandidate> ReviewRepository::findCandidateByProblemId(int problemId) {
    return useMysql()
        ? MysqlReviewRepository::findCandidateByProblemId(problemId)
        : MockReviewRepository::findCandidateByProblemId(problemId);
}

std::optional<domain::Problem> ReviewRepository::findProblemById(int problemId) {
    return useMysql()
        ? MysqlReviewRepository::findProblemById(problemId)
        : MockReviewRepository::findProblemById(problemId);
}

std::optional<domain::Note> ReviewRepository::findNoteByProblemId(int problemId) {
    return useMysql()
        ? MysqlReviewRepository::findNoteByProblemId(problemId)
        : MockReviewRepository::findNoteByProblemId(problemId);
}

std::optional<domain::Note> ReviewRepository::updateNote(
    int problemId,
    const std::string& hintMarkdown,
    const std::string& noteMarkdown
) {
    return useMysql()
        ? MysqlReviewRepository::updateNote(problemId, hintMarkdown, noteMarkdown)
        : MockReviewRepository::updateNote(problemId, hintMarkdown, noteMarkdown);
}

std::optional<domain::ReviewCandidate> ReviewRepository::importProblemToList(
    int listId,
    const std::string& url
) {
    return useMysql()
        ? MysqlReviewRepository::importProblemToList(listId, url)
        : MockReviewRepository::importProblemToList(listId, url);
}

bool ReviewRepository::updateProblemWeight(int problemId, int problemUserWeight) {
    return useMysql()
        ? MysqlReviewRepository::updateProblemWeight(problemId, problemUserWeight)
        : MockReviewRepository::updateProblemWeight(problemId, problemUserWeight);
}

bool ReviewRepository::updateReviewState(int problemId, const domain::ReviewState& reviewState) {
    return useMysql()
        ? MysqlReviewRepository::updateReviewState(problemId, reviewState)
        : MockReviewRepository::updateReviewState(problemId, reviewState);
}

bool ReviewRepository::recordJudgeSubmission(
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
    return useMysql()
        ? MysqlReviewRepository::recordJudgeSubmission(
            problemId,
            language,
            code,
            verdict,
            runtimeMs,
            memoryKb,
            message,
            remoteJudge,
            remoteSubmissionId,
            remoteSubmissionUrl
        )
        : false;
}

} // namespace repository
} // namespace algoforge
