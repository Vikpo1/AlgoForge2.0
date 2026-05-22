#pragma once

#include <string>
#include <vector>

namespace algoforge {
namespace domain {

class ProblemList {
public:
    ProblemList() = default;

    ProblemList(
        int id,
        std::string name,
        std::string description,
        int listUserWeight
    );

    int getId() const;
    const std::string& getName() const;
    const std::string& getDescription() const;
    int getListUserWeight() const;
    const std::vector<int>& getProblemIds() const;

    void setId(int id);
    void setName(const std::string& name);
    void setDescription(const std::string& description);

    // 设置题单用户权重，内部会限制到 0~100
    void setListUserWeight(int listUserWeight);

    // 向题单中加入题目 ID
    void addProblemId(int problemId);

    // 从题单中移除题目 ID
    void removeProblemId(int problemId);

    // 判断题单是否被用户启用参与调度
    bool isEnabled() const;

private:
    int id_ = 0;
    std::string name_;
    std::string description_;

    // 用户对题单设置的权重，范围 0~100
    // 权重为 0 时，该题单不会参与第一级抽取
    int listUserWeight_ = 50;

    // 当前题单包含的题目 ID
    std::vector<int> problemIds_;
};

} // namespace domain
} // namespace algoforge