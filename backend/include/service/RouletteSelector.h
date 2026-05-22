#pragma once

#include <optional>
#include <vector>
#include <cstddef>

namespace algoforge {
namespace service {

class RouletteSelector {
public:
    // 根据权重数组进行轮盘赌抽取，返回被选中的下标
    // 如果所有权重都 <= 0，则返回 std::nullopt
    static std::optional<std::size_t> selectIndex(const std::vector<int>& weights);
};

} // namespace service
} // namespace algoforge