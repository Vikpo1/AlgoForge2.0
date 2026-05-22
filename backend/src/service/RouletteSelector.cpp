#include "service/RouletteSelector.h"

#include <random>

namespace algoforge {
namespace service {

std::optional<std::size_t> RouletteSelector::selectIndex(const std::vector<int>& weights) {
    long long totalWeight = 0;

    // 只统计正权重，权重为 0 或负数的项不会被抽中
    for (int weight : weights) {
        if (weight > 0) {
            totalWeight += weight;
        }
    }

    if (totalWeight <= 0) {
        return std::nullopt;
    }

    // 使用线程局部随机数引擎，避免每次调用都重新构造随机引擎
    static thread_local std::mt19937 generator(std::random_device{}());

    std::uniform_int_distribution<long long> distribution(1, totalWeight);
    long long target = distribution(generator);

    long long currentWeight = 0;

    for (std::size_t i = 0; i < weights.size(); ++i) {
        if (weights[i] <= 0) {
            continue;
        }

        currentWeight += weights[i];

        if (currentWeight >= target) {
            return i;
        }
    }

    return std::nullopt;
}

} // namespace service
} // namespace algoforge