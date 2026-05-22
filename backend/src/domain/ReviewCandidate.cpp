#include "domain/ReviewCandidate.h"

#include <utility>

namespace algoforge {
namespace domain {

ReviewCandidate::ReviewCandidate(
    int listId,
    Problem problem,
    ReviewState reviewState
)
    : listId_(listId),
      problem_(std::move(problem)),
      reviewState_(std::move(reviewState)) {}

int ReviewCandidate::getListId() const {
    return listId_;
}

int ReviewCandidate::getProblemId() const {
    return problem_.getId();
}

const Problem& ReviewCandidate::getProblem() const {
    return problem_;
}

const ReviewState& ReviewCandidate::getReviewState() const {
    return reviewState_;
}

void ReviewCandidate::setListId(int listId) {
    listId_ = listId;
}

void ReviewCandidate::setProblem(const Problem& problem) {
    problem_ = problem;
}

void ReviewCandidate::setReviewState(const ReviewState& reviewState) {
    reviewState_ = reviewState;
}

int ReviewCandidate::getScheduleWeight() const {
    return reviewState_.getScheduleWeight();
}

bool ReviewCandidate::isAvailable() const {
    // 已归档题目不参与抽题；调度权重为 0 的题目也不参与抽题
    return !reviewState_.isArchived() && getScheduleWeight() > 0;
}

} // namespace domain
} // namespace algoforge
