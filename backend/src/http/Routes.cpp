#include "http/Routes.h"

#include "domain/Note.h"
#include "domain/Problem.h"
#include "domain/ProblemList.h"
#include "domain/ReviewCandidate.h"
#include "domain/ReviewState.h"
#include "json.hpp"
#include "repository/MysqlReviewRepository.h"
#include "repository/ReviewRepository.h"
#include "service/RouletteSelector.h"
#include "service/ReviewScheduler.h"

#include <functional>
#include <optional>
#include <random>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace algoforge {
namespace http {

namespace {

void setCorsHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

json successResponse(const json& data) {
    return {
        {"code", 0},
        {"message", "ok"},
        {"data", data}
    };
}

json errorResponse(int code, const std::string& message) {
    return {
        {"code", code},
        {"message", message},
        {"data", nullptr}
    };
}

json problemToJson(const algoforge::domain::Problem& problem) {
    return {
        {"id", problem.getId()},
        {"title", problem.getTitle()},
        {"oj", problem.getOj()},
        {"url", problem.getUrl()},
        {"difficulty", problem.getDifficulty()},
        {"tags", problem.getTags()},
        {"statementMarkdown", problem.getStatementMarkdown()},
        {"inputDescription", problem.getInputDescription()},
        {"outputDescription", problem.getOutputDescription()},
        {"sampleInput", problem.getSampleInput()},
        {"sampleOutput", problem.getSampleOutput()}
    };
}

json reviewStateToJson(const algoforge::domain::ReviewState& state) {
    return {
        {"status", algoforge::domain::reviewStatusToString(state.getStatus())},
        {"problemUserWeight", state.getProblemUserWeight()},
        {"statusWeight", state.getStatusWeight()},
        {"scheduleWeight", state.getScheduleWeight()},
        {"reviewCount", state.getReviewCount()},
        {"lastFeedback", algoforge::domain::reviewFeedbackToString(state.getLastFeedback())}
    };
}

json candidateToJson(const algoforge::domain::ReviewCandidate& candidate) {
    return {
        {"listId", candidate.getListId()},
        {"problem", problemToJson(candidate.getProblem())},
        {"reviewState", reviewStateToJson(candidate.getReviewState())}
    };
}

json listToJson(const algoforge::domain::ProblemList& list) {
    return {
        {"id", list.getId()},
        {"name", list.getName()},
        {"description", list.getDescription()},
        {"listUserWeight", list.getListUserWeight()},
        {"enabled", list.isEnabled()},
        {"problemCount", list.getProblemIds().size()}
    };
}

json noteToJson(const algoforge::domain::Note& material) {
    return {
        {"problemId", material.getProblemId()},
        {"hintMarkdown", material.getHintMarkdown()},
        {"noteMarkdown", material.getNoteMarkdown()},
        {"hasHint", material.hasHint()},
        {"hasNote", material.hasNote()}
    };
}

std::optional<int> parseInt(const std::string& text) {
    try {
        return std::stoi(text);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<algoforge::domain::ReviewFeedback> parseReviewFeedback(
    const std::string& feedbackText
) {
    using algoforge::domain::ReviewFeedback;

    if (feedbackText == "FAILED") {
        return ReviewFeedback::Failed;
    }
    if (feedbackText == "SOLVED_WITH_HINT") {
        return ReviewFeedback::SolvedWithHint;
    }
    if (feedbackText == "SOLVED_SLOWLY") {
        return ReviewFeedback::SolvedSlowly;
    }
    if (feedbackText == "SOLVED_FAST") {
        return ReviewFeedback::SolvedFast;
    }
    return std::nullopt;
}

algoforge::domain::ReviewStatus nextStatusByFeedback(
    algoforge::domain::ReviewFeedback feedback
) {
    using algoforge::domain::ReviewFeedback;
    using algoforge::domain::ReviewStatus;

    switch (feedback) {
        case ReviewFeedback::Failed:
            return ReviewStatus::FirstFix;
        case ReviewFeedback::SolvedWithHint:
            return ReviewStatus::Cooling;
        case ReviewFeedback::SolvedSlowly:
            return ReviewStatus::Cooling;
        case ReviewFeedback::SolvedFast:
            return ReviewStatus::Mastered;
        default:
            return ReviewStatus::FirstFix;
    }
}

int nextDelayHoursByFeedback(algoforge::domain::ReviewFeedback feedback) {
    using algoforge::domain::ReviewFeedback;

    switch (feedback) {
        case ReviewFeedback::Failed:
            return 0;
        case ReviewFeedback::SolvedWithHint:
            return 24;
        case ReviewFeedback::SolvedSlowly:
            return 72;
        case ReviewFeedback::SolvedFast:
            return 168;
        default:
            return 0;
    }
}

std::optional<algoforge::domain::ReviewCandidate> selectRandomCandidate(
    const std::vector<algoforge::domain::ReviewCandidate>& candidates
) {
    std::vector<algoforge::domain::ReviewCandidate> available;
    for (const auto& candidate : candidates) {
        if (candidate.isAvailable()) {
            available.push_back(candidate);
        }
    }

    if (available.empty()) {
        return std::nullopt;
    }

    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<std::size_t> distribution(0, available.size() - 1);
    return available[distribution(generator)];
}

void writeJson(httplib::Response& res, const json& payload, int status = 200) {
    res.status = status;
    res.set_content(payload.dump(4), "application/json");
}

} // anonymous namespace

void registerRoutes(httplib::Server& server) {
    server.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        res.status = 204;
    });

    server.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        writeJson(res, successResponse({
            {"service", "AlgoForge Backend"},
            {"version", "0.5.0-problem-crawler"},
            {"judgeSubmitRoute", false}
        }));
    });

    server.Get("/api/db/health", [](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);

        auto status = algoforge::repository::MysqlReviewRepository::testConnection();
        writeJson(res, successResponse({
            {"enabled", status.enabled},
            {"connected", status.connected},
            {"message", status.message},
            {"host", status.host},
            {"port", status.port},
            {"database", status.database}
        }));
    });

    server.Get("/api/problem-lists", [](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);

        json lists = json::array();
        for (const auto& list : algoforge::repository::ReviewRepository::listProblemLists()) {
            lists.push_back(listToJson(list));
        }

        writeJson(res, successResponse({{"lists", lists}}));
    });

    server.Post("/api/problem-lists", [](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);

        json body;
        try {
            body = json::parse(req.body);
        } catch (const std::exception&) {
            writeJson(res, errorResponse(400, "Invalid JSON body"), 400);
            return;
        }

        const std::string name = body.value("name", "");
        const std::string description = body.value("description", "");
        const int weight = body.value("listUserWeight", 50);

        if (name.empty()) {
            writeJson(res, errorResponse(400, "Problem list name is required"), 400);
            return;
        }

        auto list = algoforge::repository::ReviewRepository::createProblemList(
            name,
            description,
            weight
        );

        writeJson(res, successResponse({{"list", listToJson(list.value())}}));
    });

    server.Patch(R"(/api/problem-lists/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);

        auto listId = parseInt(req.matches[1]);
        if (!listId.has_value()) {
            writeJson(res, errorResponse(400, "Invalid list id"), 400);
            return;
        }

        json body;
        try {
            body = json::parse(req.body);
        } catch (const std::exception&) {
            writeJson(res, errorResponse(400, "Invalid JSON body"), 400);
            return;
        }

        std::optional<std::string> name;
        std::optional<std::string> description;
        std::optional<int> listUserWeight;

        if (body.contains("name")) {
            name = body.value("name", "");
        }
        if (body.contains("description")) {
            description = body.value("description", "");
        }
        if (body.contains("listUserWeight")) {
            listUserWeight = body.value("listUserWeight", 50);
        }

        auto updated = algoforge::repository::ReviewRepository::updateProblemList(
            listId.value(),
            name,
            description,
            listUserWeight
        );

        if (!updated.has_value()) {
            writeJson(res, errorResponse(404, "Problem list not found"), 404);
            return;
        }

        writeJson(res, successResponse({{"list", listToJson(updated.value())}}));
    });

    server.Delete(R"(/api/problem-lists/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);

        auto listId = parseInt(req.matches[1]);
        if (!listId.has_value()) {
            writeJson(res, errorResponse(400, "Invalid list id"), 400);
            return;
        }

        if (!algoforge::repository::ReviewRepository::deleteProblemList(listId.value())) {
            writeJson(res, errorResponse(404, "Problem list not found"), 404);
            return;
        }

        writeJson(res, successResponse({{"deletedListId", listId.value()}}));
    });

    server.Get(R"(/api/problem-lists/(\d+)/problems)", [](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);

        auto listId = parseInt(req.matches[1]);
        if (!listId.has_value()) {
            writeJson(res, errorResponse(400, "Invalid list id"), 400);
            return;
        }

        json problems = json::array();
        for (const auto& candidate : algoforge::repository::ReviewRepository::listCandidatesByListId(listId.value())) {
            problems.push_back(candidateToJson(candidate));
        }

        writeJson(res, successResponse({{"problems", problems}}));
    });

    server.Post("/api/problems/import", [](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);

        json body;
        try {
            body = json::parse(req.body);
        } catch (const std::exception&) {
            writeJson(res, errorResponse(400, "Invalid JSON body"), 400);
            return;
        }

        const int listId = body.value("listId", 0);
        const std::string url = body.value("url", "");

        if (url.empty()) {
            writeJson(res, errorResponse(400, "Problem URL is required"), 400);
            return;
        }

        if (!algoforge::repository::ReviewRepository::findProblemList(listId).has_value()) {
            writeJson(res, errorResponse(404, "Target problem list not found"), 404);
            return;
        }

        auto candidate = algoforge::repository::ReviewRepository::importProblemToList(listId, url);
        if (!candidate.has_value()) {
            writeJson(res, errorResponse(500, "Problem import failed. Check backend MySQL logs for details."), 500);
            return;
        }

        writeJson(res, successResponse({{"candidate", candidateToJson(candidate.value())}}));
    });

    server.Patch(R"(/api/problems/(\d+)/weight)", [](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);

        auto problemId = parseInt(req.matches[1]);
        if (!problemId.has_value()) {
            writeJson(res, errorResponse(400, "Invalid problem id"), 400);
            return;
        }

        json body;
        try {
            body = json::parse(req.body);
        } catch (const std::exception&) {
            writeJson(res, errorResponse(400, "Invalid JSON body"), 400);
            return;
        }

        int weight = body.value("problemUserWeight", 1);
        if (!algoforge::repository::ReviewRepository::updateProblemWeight(problemId.value(), weight)) {
            writeJson(res, errorResponse(404, "Problem not found"), 404);
            return;
        }

        auto candidate = algoforge::repository::ReviewRepository::findCandidateByProblemId(problemId.value());
        writeJson(res, successResponse({{"candidate", candidateToJson(candidate.value())}}));
    });

    server.Get(R"(/api/problems/(\d+)/detail)", [](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);

        auto problemId = parseInt(req.matches[1]);
        if (!problemId.has_value()) {
            writeJson(res, errorResponse(400, "Invalid problem id"), 400);
            return;
        }

        auto candidate = algoforge::repository::ReviewRepository::findCandidateByProblemId(problemId.value());
        if (!candidate.has_value()) {
            writeJson(res, errorResponse(404, "Problem not found"), 404);
            return;
        }

        writeJson(res, successResponse({{"candidate", candidateToJson(candidate.value())}}));
    });

    server.Get("/api/review/next", [](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);

        using algoforge::repository::ReviewRepository;
        using algoforge::service::ReviewScheduler;

        std::string mode = req.has_param("mode") ? req.get_param_value("mode") : "weighted";
        std::optional<int> listId;
        if (req.has_param("listId")) {
            listId = parseInt(req.get_param_value("listId"));
        }

        auto pool = ReviewRepository::loadReviewPool();

        if (listId.has_value()) {
            pool.problemLists.erase(
                std::remove_if(pool.problemLists.begin(), pool.problemLists.end(), [listId](const algoforge::domain::ProblemList& list) {
                    return list.getId() != listId.value();
                }),
                pool.problemLists.end()
            );
            pool.candidates.erase(
                std::remove_if(pool.candidates.begin(), pool.candidates.end(), [listId](const algoforge::domain::ReviewCandidate& candidate) {
                    return candidate.getListId() != listId.value();
                }),
                pool.candidates.end()
            );
        }

        std::optional<algoforge::domain::ReviewCandidate> selected;
        if (mode == "random") {
            selected = selectRandomCandidate(pool.candidates);
        } else {
            selected = ReviewScheduler::selectNextProblem(pool.problemLists, pool.candidates);
        }

        if (!selected.has_value()) {
            writeJson(res, errorResponse(404, "No available review problem"), 404);
            return;
        }

        writeJson(res, successResponse(candidateToJson(selected.value())));
    });

    server.Get("/api/dev/next-review", [](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);

        using algoforge::repository::ReviewRepository;
        using algoforge::service::ReviewScheduler;

        auto pool = ReviewRepository::loadReviewPool();
        auto selected = ReviewScheduler::selectNextProblem(pool.problemLists, pool.candidates);

        if (!selected.has_value()) {
            writeJson(res, errorResponse(404, "No available review problem"), 404);
            return;
        }

        writeJson(res, successResponse(candidateToJson(selected.value())));
    });

    server.Get(R"(/api/problems/(\d+)/review-material)", [](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);

        auto problemId = parseInt(req.matches[1]);
        if (!problemId.has_value()) {
            writeJson(res, errorResponse(400, "Invalid problem id"), 400);
            return;
        }

        auto note = algoforge::repository::ReviewRepository::findNoteByProblemId(problemId.value());
        if (!note.has_value()) {
            if (!algoforge::repository::ReviewRepository::findProblemById(problemId.value()).has_value()) {
                writeJson(res, errorResponse(404, "Problem not found"), 404);
                return;
            }

            writeJson(res, successResponse({
                {"problemId", problemId.value()},
                {"hintMarkdown", ""},
                {"noteMarkdown", ""},
                {"hasHint", false},
                {"hasNote", false}
            }));
            return;
        }

        writeJson(res, successResponse(noteToJson(note.value())));
    });

    server.Patch(R"(/api/problems/(\d+)/review-material)", [](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);

        auto problemId = parseInt(req.matches[1]);
        if (!problemId.has_value()) {
            writeJson(res, errorResponse(400, "Invalid problem id"), 400);
            return;
        }

        json body;
        try {
            body = json::parse(req.body);
        } catch (const std::exception&) {
            writeJson(res, errorResponse(400, "Invalid JSON body"), 400);
            return;
        }

        const std::string hintMarkdown = body.value("hintMarkdown", "");
        const std::string noteMarkdown = body.value("noteMarkdown", "");
        auto note = algoforge::repository::ReviewRepository::updateNote(
            problemId.value(),
            hintMarkdown,
            noteMarkdown
        );

        if (!note.has_value()) {
            writeJson(res, errorResponse(404, "Problem not found"), 404);
            return;
        }

        writeJson(res, successResponse(noteToJson(note.value())));
    });

    server.Post(R"(/api/review/(\d+)/feedback)", [](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);

        auto problemId = parseInt(req.matches[1]);
        if (!problemId.has_value()) {
            writeJson(res, errorResponse(400, "Invalid problem id"), 400);
            return;
        }

        json body;
        try {
            body = json::parse(req.body);
        } catch (const std::exception&) {
            writeJson(res, errorResponse(400, "Invalid JSON body"), 400);
            return;
        }

        auto feedback = parseReviewFeedback(body.value("feedback", ""));
        if (!feedback.has_value()) {
            writeJson(res, errorResponse(400, "Invalid feedback value"), 400);
            return;
        }

        int durationSeconds = body.value("durationSeconds", 0);
        auto candidate = algoforge::repository::ReviewRepository::findCandidateByProblemId(problemId.value());
        if (!candidate.has_value()) {
            writeJson(res, errorResponse(404, "Problem not found"), 404);
            return;
        }

        const auto& currentState = candidate->getReviewState();
        algoforge::domain::ReviewState updatedState(
            problemId.value(),
            nextStatusByFeedback(feedback.value()),
            currentState.getProblemUserWeight(),
            currentState.getReviewCount() + 1,
            feedback.value()
        );

        algoforge::repository::ReviewRepository::updateReviewState(problemId.value(), updatedState);

        writeJson(res, successResponse({
            {"problemId", problemId.value()},
            {"submittedFeedback", algoforge::domain::reviewFeedbackToString(feedback.value())},
            {"durationSeconds", durationSeconds},
            {"updatedReviewState", reviewStateToJson(updatedState)},
            {"nextReviewDelayHours", nextDelayHoursByFeedback(feedback.value())}
        }));
    });
}

} // namespace http
} // namespace algoforge
