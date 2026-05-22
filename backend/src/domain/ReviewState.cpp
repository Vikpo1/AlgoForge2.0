#include "domain/ReviewState.h"

#include <algorithm>

namespace algoforge {
namespace domain {

namespace {

// 限制题目用户权重范围，避免出现负数或过大权重
int clampProblemUserWeight(int weight) {
    return std::clamp(weight, 0, 100);
}

} // anonymous namespace

ReviewState::ReviewState(
    int problemId,
    ReviewStatus status,
    int problemUserWeight,
    int reviewCount,
    ReviewFeedback lastFeedback
)
    : problemId_(problemId),
      status_(status),
      problemUserWeight_(clampProblemUserWeight(problemUserWeight)),
      reviewCount_(reviewCount),
      lastFeedback_(lastFeedback) {}

int ReviewState::getProblemId() const {
    return problemId_;
}

ReviewStatus ReviewState::getStatus() const {
    return status_;
}

int ReviewState::getProblemUserWeight() const {
    return problemUserWeight_;
}

int ReviewState::getReviewCount() const {
    return reviewCount_;
}

ReviewFeedback ReviewState::getLastFeedback() const {
    return lastFeedback_;
}

void ReviewState::setProblemId(int problemId) {
    problemId_ = problemId;
}

void ReviewState::setStatus(ReviewStatus status) {
    status_ = status;
}

void ReviewState::setProblemUserWeight(int problemUserWeight) {
    problemUserWeight_ = clampProblemUserWeight(problemUserWeight);
}

void ReviewState::setReviewCount(int reviewCount) {
    reviewCount_ = reviewCount;
}

void ReviewState::setLastFeedback(ReviewFeedback feedback) {
    lastFeedback_ = feedback;
}

bool ReviewState::isArchived() const {
    return status_ == ReviewStatus::Archived;
}

int ReviewState::getStatusWeight() const {
    switch (status_) {
        case ReviewStatus::FirstFix:
            return 100;
        case ReviewStatus::Cooling:
            return 50;
        case ReviewStatus::Training:
            return 20;
        case ReviewStatus::Mastered:
            return 5;
        case ReviewStatus::Archived:
            return 0;
        default:
            return 0;
    }
}

int ReviewState::getScheduleWeight() const {
    return problemUserWeight_ * getStatusWeight();
}

std::string reviewStatusToString(ReviewStatus status) {
    switch (status) {
        case ReviewStatus::FirstFix:
            return "FIRST_FIX";
        case ReviewStatus::Cooling:
            return "COOLING";
        case ReviewStatus::Training:
            return "TRAINING";
        case ReviewStatus::Mastered:
            return "MASTERED";
        case ReviewStatus::Archived:
            return "ARCHIVED";
        default:
            return "UNKNOWN";
    }
}

std::string reviewFeedbackToString(ReviewFeedback feedback) {
    switch (feedback) {
        case ReviewFeedback::Failed:
            return "FAILED";
        case ReviewFeedback::SolvedWithHint:
            return "SOLVED_WITH_HINT";
        case ReviewFeedback::SolvedSlowly:
            return "SOLVED_SLOWLY";
        case ReviewFeedback::SolvedFast:
            return "SOLVED_FAST";
        default:
            return "UNKNOWN";
    }
}

} // namespace domain
} // namespace algoforge