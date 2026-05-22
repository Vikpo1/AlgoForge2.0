#include "service/ReviewScheduler.h"
#include "service/RouletteSelector.h"

namespace algoforge {
namespace service {

std::optional<domain::ReviewCandidate> ReviewScheduler::selectNextProblem(
    const std::vector<domain::ProblemList>& problemLists,
    const std::vector<domain::ReviewCandidate>& candidates
) {
    std::vector<domain::ProblemList> availableLists;
    std::vector<int> listWeights;

    // 第一级：筛选可参与调度的题单
    // 条件：
    // 1. 题单权重 > 0
    // 2. 题单内部至少有一道可抽取题目
    for (const auto& list : problemLists) {
        if (!list.isEnabled()) {
            continue;
        }

        bool hasAvailableCandidate = false;

        for (const auto& candidate : candidates) {
            if (
                candidate.getListId() == list.getId() &&
                candidate.isAvailable()
            ) {
                hasAvailableCandidate = true;
                break;
            }
        }

        if (hasAvailableCandidate) {
            availableLists.push_back(list);
            listWeights.push_back(list.getListUserWeight());
        }
    }

    auto selectedListIndex = RouletteSelector::selectIndex(listWeights);

    if (!selectedListIndex.has_value()) {
        return std::nullopt;
    }

    const auto& selectedList = availableLists[selectedListIndex.value()];

    std::vector<domain::ReviewCandidate> availableCandidates;
    std::vector<int> candidateWeights;

    // 第二级：在选中的题单内部筛选可抽取题目
    for (const auto& candidate : candidates) {
        if (
            candidate.getListId() == selectedList.getId() &&
            candidate.isAvailable()
        ) {
            availableCandidates.push_back(candidate);
            candidateWeights.push_back(candidate.getScheduleWeight());
        }
    }

    auto selectedCandidateIndex = RouletteSelector::selectIndex(candidateWeights);

    if (!selectedCandidateIndex.has_value()) {
        return std::nullopt;
    }

    return availableCandidates[selectedCandidateIndex.value()];
}

} // namespace service
} // namespace algoforge