#pragma once

#include "domain/ProblemList.h"
#include "domain/ReviewCandidate.h"

#include <optional>
#include <vector>

namespace algoforge {
namespace service {

class ReviewScheduler {
public:
    // 两级调度：
    // 1. 先根据题单权重抽取一个题单
    // 2. 再在该题单内部根据题目调度权重抽取一道题
    static std::optional<domain::ReviewCandidate> selectNextProblem(
        const std::vector<domain::ProblemList>& problemLists,
        const std::vector<domain::ReviewCandidate>& candidates
    );
};

} // namespace service
} // namespace algoforge