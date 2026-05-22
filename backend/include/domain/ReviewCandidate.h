#pragma once

#include "domain/Problem.h"
#include "domain/ReviewState.h"

namespace algoforge {
namespace domain {

class ReviewCandidate {
public:
    ReviewCandidate() = default;

    ReviewCandidate(
        int listId,
        Problem problem,
        ReviewState reviewState
    );

    int getListId() const;
    int getProblemId() const;

    const Problem& getProblem() const;
    const ReviewState& getReviewState() const;

    void setListId(int listId);
    void setProblem(const Problem& problem);
    void setReviewState(const ReviewState& reviewState);

    // 获取该题在题单内部参与抽取时的调度权重
    int getScheduleWeight() const;

    // 判断该题当前是否可以参与抽题
    bool isAvailable() const;

private:
    // 候选题所属题单 ID
    int listId_ = 0;

    // 题目基础信息
    Problem problem_;

    // 题目复习状态与单题权重
    ReviewState reviewState_;
};

} // namespace domain
} // namespace algoforge
