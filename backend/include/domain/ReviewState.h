#pragma once

#include <string>

namespace algoforge {
namespace domain {

// 题目的复习生命周期状态
enum class ReviewStatus {
    FirstFix,   // 赛后首补：最高优先级
    Cooling,    // 冷却重刷：中优先级
    Training,   // 穿插特训：低优先级
    Mastered,   // 已掌握：极低优先级
    Archived    // 已归档：不再参与抽题
};

// 用户完成复习后的反馈
enum class ReviewFeedback {
    Failed,           // 完全不会
    SolvedWithHint,   // 看提示后做出
    SolvedSlowly,     // 独立做出，但耗时较长
    SolvedFast        // 快速解决，基本掌握
};

class ReviewState {
public:
    ReviewState() = default;

    ReviewState(
        int problemId,
        ReviewStatus status,
        int problemUserWeight,
        int reviewCount,
        ReviewFeedback lastFeedback
    );

    int getProblemId() const;
    ReviewStatus getStatus() const;
    int getProblemUserWeight() const;
    int getReviewCount() const;
    ReviewFeedback getLastFeedback() const;

    void setProblemId(int problemId);
    void setStatus(ReviewStatus status);

    // 设置用户对单题的手动权重，内部会限制到 0~100
    void setProblemUserWeight(int problemUserWeight);

    void setReviewCount(int reviewCount);
    void setLastFeedback(ReviewFeedback feedback);

    // 判断题目是否已经归档
    bool isArchived() const;

    // 根据当前生命周期状态返回系统状态权重
    int getStatusWeight() const;

    // 返回题目最终调度权重：题目用户权重 × 状态权重
    int getScheduleWeight() const;

private:
    int problemId_ = 0;

    // 初始状态为赛后首补
    ReviewStatus status_ = ReviewStatus::FirstFix;

    // 用户对单题设置的权重，默认 1，范围 0~100
    int problemUserWeight_ = 1;

    int reviewCount_ = 0;

    // 默认认为最近一次反馈是不会，便于初始阶段保持高优先级
    ReviewFeedback lastFeedback_ = ReviewFeedback::Failed;
};

// 枚举转字符串，方便后续返回 JSON 或写入数据库
std::string reviewStatusToString(ReviewStatus status);
std::string reviewFeedbackToString(ReviewFeedback feedback);

} // namespace domain
} // namespace algoforge