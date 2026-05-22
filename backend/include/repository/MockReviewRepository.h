#pragma once

#include "domain/Note.h"
#include "domain/Problem.h"
#include "domain/ProblemList.h"
#include "domain/ReviewCandidate.h"
#include "domain/ReviewState.h"

#include <optional>
#include <string>
#include <vector>

namespace algoforge {
namespace repository {

struct ReviewPool {
    std::vector<domain::ProblemList> problemLists;
    std::vector<domain::ReviewCandidate> candidates;
};

class MockReviewRepository {
public:
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

private:
    static domain::Problem buildImportedProblem(int problemId, const std::string& url);
    static domain::Note buildImportedNote(int problemId, const std::string& url);
};

} // namespace repository
} // namespace algoforge
