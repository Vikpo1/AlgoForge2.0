#include "domain/ProblemList.h"

#include <algorithm>
#include <utility>

namespace algoforge {
namespace domain {

namespace {

// 限制题单用户权重范围，避免出现负数或过大权重
int clampListUserWeight(int weight) {
    return std::clamp(weight, 0, 100);
}

} // anonymous namespace

ProblemList::ProblemList(
    int id,
    std::string name,
    std::string description,
    int listUserWeight
)
    : id_(id),
      name_(std::move(name)),
      description_(std::move(description)),
      listUserWeight_(clampListUserWeight(listUserWeight)) {}

int ProblemList::getId() const {
    return id_;
}

const std::string& ProblemList::getName() const {
    return name_;
}

const std::string& ProblemList::getDescription() const {
    return description_;
}

int ProblemList::getListUserWeight() const {
    return listUserWeight_;
}

const std::vector<int>& ProblemList::getProblemIds() const {
    return problemIds_;
}

void ProblemList::setId(int id) {
    id_ = id;
}

void ProblemList::setName(const std::string& name) {
    name_ = name;
}

void ProblemList::setDescription(const std::string& description) {
    description_ = description;
}

void ProblemList::setListUserWeight(int listUserWeight) {
    listUserWeight_ = clampListUserWeight(listUserWeight);
}

void ProblemList::addProblemId(int problemId) {
    // 避免重复加入同一道题
    auto it = std::find(problemIds_.begin(), problemIds_.end(), problemId);
    if (it == problemIds_.end()) {
        problemIds_.push_back(problemId);
    }
}

void ProblemList::removeProblemId(int problemId) {
    problemIds_.erase(
        std::remove(problemIds_.begin(), problemIds_.end(), problemId),
        problemIds_.end()
    );
}

bool ProblemList::isEnabled() const {
    return listUserWeight_ > 0;
}

} // namespace domain
} // namespace algoforge