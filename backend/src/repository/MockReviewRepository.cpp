#include "repository/MockReviewRepository.h"

#include "domain/ReviewState.h"
#include "service/ProblemCrawler.h"

#include <algorithm>
#include <cctype>
#include <mutex>
#include <sstream>
#include <unordered_map>

namespace algoforge {
namespace repository {

namespace {

struct Store {
    int nextListId = 3;
    int nextProblemId = 300;
    int nextNoteId = 5;
    std::vector<domain::ProblemList> problemLists;
    std::vector<domain::ReviewCandidate> candidates;
    std::unordered_map<int, domain::Note> notes;
};

Store& store() {
    static Store data = [] {
        Store seed;

        using namespace algoforge::domain;

        ProblemList list1(1, "赛后补题", "比赛后未解决题目", 80);
        ProblemList list2(2, "DP 专题", "动态规划专项训练", 20);

        list1.addProblemId(101);
        list1.addProblemId(102);
        list2.addProblemId(201);
        list2.addProblemId(202);

        Problem p1(101, "补题 A - 最短路", "Codeforces", "https://codeforces.com/problemset/problem/1/A", "Medium");
        p1.setTags({"图论", "最短路"});
        p1.setStatementMarkdown("## 题面\n\n给定一张带权图，请求从起点到终点的最短路径。\n\n### 约束\n\n- 点数较小，适合最短路建模。\n- 需要判断是否存在特殊边权。");
        p1.setInputDescription("输入包含图的点数、边数以及所有边的信息。");
        p1.setOutputDescription("输出最短路径长度或最优答案。");
        p1.setSampleInput("4 4\n1 2 3\n2 4 5\n1 3 2\n3 4 4");
        p1.setSampleOutput("6");

        Problem p2(102, "补题 B - 贪心", "AtCoder", "https://atcoder.jp/contests/abc001/tasks/abc001_1", "Easy");
        p2.setTags({"贪心", "排序"});
        p2.setStatementMarkdown("## 题面\n\n根据题意做出若干次局部最优选择，求最终结果。");
        p2.setInputDescription("输入为若干整数和约束条件。");
        p2.setOutputDescription("输出满足要求的方案或数值。");
        p2.setSampleInput("3\n1 2 3");
        p2.setSampleOutput("2");

        Problem p3(201, "DP A - 背包", "Luogu", "https://www.luogu.com.cn/problem/P1048", "Medium");
        p3.setTags({"DP", "背包"});
        p3.setStatementMarkdown("## 题面\n\n这是一个经典背包问题，请注意状态定义与转移方向。");
        p3.setInputDescription("输入包括物品数量和容量上限。");
        p3.setOutputDescription("输出最大价值或最优方案。");
        p3.setSampleInput("4 10\n2 3\n3 4\n4 5\n5 8");
        p3.setSampleOutput("12");

        Problem p4(202, "DP B - 区间 DP", "Luogu", "https://www.luogu.com.cn/problem/P1880", "Hard");
        p4.setTags({"DP", "区间DP"});
        p4.setStatementMarkdown("## 题面\n\n请在区间长度递增的过程中完成状态转移。");
        p4.setInputDescription("输入为区间相关的序列数据。");
        p4.setOutputDescription("输出区间 DP 的最优值。");
        p4.setSampleInput("5\n1 2 3 4 5");
        p4.setSampleOutput("9");

        ReviewState s1(101, ReviewStatus::FirstFix, 1, 0, ReviewFeedback::Failed);
        ReviewState s2(102, ReviewStatus::Cooling, 1, 1, ReviewFeedback::SolvedWithHint);
        ReviewState s3(201, ReviewStatus::Training, 5, 2, ReviewFeedback::SolvedSlowly);
        ReviewState s4(202, ReviewStatus::Archived, 100, 3, ReviewFeedback::SolvedFast);

        seed.problemLists = {list1, list2};
        seed.candidates = {
            ReviewCandidate(1, p1, s1),
            ReviewCandidate(1, p2, s2),
            ReviewCandidate(2, p3, s3),
            ReviewCandidate(2, p4, s4)
        };
        seed.notes.emplace(101, Note(1, 101, "可以先思考图上边权和路径转移关系，不要急着看完整题解。", "### 解题复盘\n\n这道题可以抽象成最短路模型。\n\n关键点：\n\n1. 建图方式是否正确；\n2. 起点和终点是否明确；\n3. 是否存在多源或特殊边权；\n4. 根据边权选择 Dijkstra 或 BFS 变体。"));
        seed.notes.emplace(102, Note(2, 102, "先观察局部最优选择是否会影响后续状态。", "### 解题复盘\n\n这道题偏贪心。\n\n核心是找到一个可以证明的局部最优策略，而不是只凭直觉选择。"));
        seed.notes.emplace(201, Note(3, 201, "先定义 dp[i][j] 或 dp[i] 的含义，再考虑转移方向。", "### 解题复盘\n\n这是一道背包类 DP。\n\n建议先明确：\n\n1. 物品维度；\n2. 容量维度；\n3. 状态含义；\n4. 顺序枚举还是逆序枚举。"));
        seed.notes.emplace(202, Note(4, 202, "区间 DP 一般先枚举区间长度，再枚举左端点。", "### 解题复盘\n\n这是一道区间 DP。\n\n常见套路：\n\n1. 枚举区间长度 len；\n2. 枚举左端点 l；\n3. 计算右端点 r；\n4. 枚举断点 k 进行状态转移。"));

        seed.nextProblemId = 300;
        seed.nextNoteId = 5;
        return seed;
    }();

    return data;
}

std::string normalizeUrl(std::string url) {
    while (!url.empty() && std::isspace(static_cast<unsigned char>(url.front()))) {
        url.erase(url.begin());
    }
    while (!url.empty() && std::isspace(static_cast<unsigned char>(url.back()))) {
        url.pop_back();
    }
    return url;
}

std::string inferOj(const std::string& url) {
    if (url.find("codeforces") != std::string::npos) {
        return "Codeforces";
    }
    if (url.find("atcoder") != std::string::npos) {
        return "AtCoder";
    }
    if (url.find("luogu") != std::string::npos) {
        return "Luogu";
    }
    if (url.find("leetcode") != std::string::npos) {
        return "LeetCode";
    }
    return "Imported";
}

std::vector<std::string> inferTags(const std::string& url) {
    std::vector<std::string> tags;

    auto pushIfMissing = [&tags](const std::string& tag) {
        if (std::find(tags.begin(), tags.end(), tag) == tags.end()) {
            tags.push_back(tag);
        }
    };

    if (url.find("graph") != std::string::npos || url.find("shortest") != std::string::npos) {
        pushIfMissing("图论");
        pushIfMissing("最短路");
    }
    if (url.find("dp") != std::string::npos || url.find("dynamic") != std::string::npos) {
        pushIfMissing("DP");
    }
    if (url.find("greedy") != std::string::npos) {
        pushIfMissing("贪心");
    }
    if (url.find("tree") != std::string::npos) {
        pushIfMissing("树");
    }
    if (tags.empty()) {
        pushIfMissing("基础");
    }

    return tags;
}

std::string makeTitle(const std::string& oj, int problemId) {
    std::ostringstream oss;
    oss << oj << " 导入题目 " << problemId;
    return oss.str();
}

std::string makeStatement(const std::string& oj, const std::string& url) {
    std::ostringstream oss;
    oss << "## 题面\n\n";
    oss << "这是从 " << oj << " 原题链接抓取到的本地模拟题面。\n\n";
    oss << "- 原始链接：`" << url << "`\n";
    oss << "- 当前实现：本地内存版抓取结果\n";
    oss << "- 目的：先把题面、标签和笔记在网页里直接展开";
    return oss.str();
}

std::string makeSampleInput(int problemId) {
    std::ostringstream oss;
    oss << problemId % 7 + 2 << " " << problemId % 11 + 3;
    return oss.str();
}

std::string makeSampleOutput(int problemId) {
    std::ostringstream oss;
    oss << problemId % 13 + 1;
    return oss.str();
}

std::optional<std::size_t> findListIndex(int listId) {
    auto& data = store();
    for (std::size_t i = 0; i < data.problemLists.size(); ++i) {
        if (data.problemLists[i].getId() == listId) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> findCandidateIndexByProblemId(int problemId) {
    auto& data = store();
    for (std::size_t i = 0; i < data.candidates.size(); ++i) {
        if (data.candidates[i].getProblemId() == problemId) {
            return i;
        }
    }
    return std::nullopt;
}

} // anonymous namespace

ReviewPool MockReviewRepository::loadReviewPool() {
    auto& data = store();
    return {data.problemLists, data.candidates};
}

std::vector<domain::ProblemList> MockReviewRepository::listProblemLists() {
    return store().problemLists;
}

std::optional<domain::ProblemList> MockReviewRepository::findProblemList(int listId) {
    auto index = findListIndex(listId);
    if (!index.has_value()) {
        return std::nullopt;
    }
    return store().problemLists[index.value()];
}

std::optional<domain::ProblemList> MockReviewRepository::createProblemList(
    const std::string& name,
    const std::string& description,
    int listUserWeight
) {
    auto& data = store();
    domain::ProblemList list(data.nextListId++, name, description, listUserWeight);
    data.problemLists.push_back(list);
    return list;
}

std::optional<domain::ProblemList> MockReviewRepository::updateProblemList(
    int listId,
    const std::optional<std::string>& name,
    const std::optional<std::string>& description,
    const std::optional<int>& listUserWeight
) {
    auto index = findListIndex(listId);
    if (!index.has_value()) {
        return std::nullopt;
    }

    auto& list = store().problemLists[index.value()];
    if (name.has_value()) {
        list.setName(name.value());
    }
    if (description.has_value()) {
        list.setDescription(description.value());
    }
    if (listUserWeight.has_value()) {
        list.setListUserWeight(listUserWeight.value());
    }
    return list;
}

bool MockReviewRepository::deleteProblemList(int listId) {
    auto& data = store();
    auto listIndex = findListIndex(listId);
    if (!listIndex.has_value()) {
        return false;
    }

    data.problemLists.erase(data.problemLists.begin() + static_cast<std::ptrdiff_t>(listIndex.value()));
    data.candidates.erase(
        std::remove_if(data.candidates.begin(), data.candidates.end(), [listId](const domain::ReviewCandidate& candidate) {
            return candidate.getListId() == listId;
        }),
        data.candidates.end()
    );
    return true;
}

bool MockReviewRepository::removeProblemFromList(int listId, int problemId) {
    auto& data = store();
    auto listIndex = findListIndex(listId);
    if (!listIndex.has_value()) {
        return false;
    }

    const auto& problemIds = data.problemLists[listIndex.value()].getProblemIds();
    if (std::find(problemIds.begin(), problemIds.end(), problemId) == problemIds.end()) {
        return false;
    }

    data.problemLists[listIndex.value()].removeProblemId(problemId);
    data.candidates.erase(
        std::remove_if(data.candidates.begin(), data.candidates.end(), [listId, problemId](const domain::ReviewCandidate& candidate) {
            return candidate.getListId() == listId && candidate.getProblem().getId() == problemId;
        }),
        data.candidates.end()
    );

    const bool stillUsed = std::any_of(data.candidates.begin(), data.candidates.end(), [problemId](const domain::ReviewCandidate& candidate) {
        return candidate.getProblem().getId() == problemId;
    });
    if (!stillUsed) {
        data.notes.erase(problemId);
    }

    return true;
}

std::vector<domain::ReviewCandidate> MockReviewRepository::listCandidates() {
    return store().candidates;
}

std::vector<domain::ReviewCandidate> MockReviewRepository::listCandidatesByListId(int listId) {
    std::vector<domain::ReviewCandidate> result;
    for (const auto& candidate : store().candidates) {
        if (candidate.getListId() == listId) {
            result.push_back(candidate);
        }
    }
    return result;
}

std::optional<domain::ReviewCandidate> MockReviewRepository::findCandidateByProblemId(int problemId) {
    auto index = findCandidateIndexByProblemId(problemId);
    if (!index.has_value()) {
        return std::nullopt;
    }
    return store().candidates[index.value()];
}

std::optional<domain::Problem> MockReviewRepository::findProblemById(int problemId) {
    auto candidate = findCandidateByProblemId(problemId);
    if (!candidate.has_value()) {
        return std::nullopt;
    }
    return candidate->getProblem();
}

std::optional<domain::Note> MockReviewRepository::findNoteByProblemId(int problemId) {
    auto& data = store();
    auto it = data.notes.find(problemId);
    if (it == data.notes.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<domain::Note> MockReviewRepository::updateNote(
    int problemId,
    const std::string& hintMarkdown,
    const std::string& noteMarkdown
) {
    if (!findCandidateByProblemId(problemId).has_value()) {
        return std::nullopt;
    }

    auto& data = store();
    auto it = data.notes.find(problemId);
    if (it == data.notes.end()) {
        auto inserted = data.notes.emplace(
            problemId,
            domain::Note(data.nextNoteId++, problemId, hintMarkdown, noteMarkdown)
        );
        return inserted.first->second;
    }

    it->second.setHintMarkdown(hintMarkdown);
    it->second.setNoteMarkdown(noteMarkdown);
    return it->second;
}

domain::Problem MockReviewRepository::buildImportedProblem(int problemId, const std::string& url) {
    return service::crawlProblem(problemId, normalizeUrl(url)).problem;

    const std::string trimmedUrl = normalizeUrl(url);
    const std::string oj = inferOj(trimmedUrl);
    const auto tags = inferTags(trimmedUrl);

    domain::Problem problem(
        problemId,
        makeTitle(oj, problemId),
        oj,
        trimmedUrl,
        "Medium"
    );
    problem.setTags(tags);
    problem.setStatementMarkdown(makeStatement(oj, trimmedUrl));
    problem.setInputDescription("输入格式由题目网页决定，这里展示模拟抓取内容。");
    problem.setOutputDescription("输出格式由题目网页决定，这里展示模拟抓取内容。");
    problem.setSampleInput(makeSampleInput(problemId));
    problem.setSampleOutput(makeSampleOutput(problemId));
    return problem;
}

domain::Note MockReviewRepository::buildImportedNote(int problemId, const std::string& url) {
    std::ostringstream hint;
    hint << "这是从链接 `" << url << "` 导入的模拟提示。";

    std::ostringstream note;
    note << "### 导入复盘\n\n";
    note << "该题目前由本地仓库模拟抓取，后续可替换为真实 OJ 抓取结果。\n\n";
    note << "建议先根据标签推导思路，再展开完整笔记。";

    return domain::Note(store().nextNoteId++, problemId, hint.str(), note.str());
}

std::optional<domain::ReviewCandidate> MockReviewRepository::importProblemToList(
    int listId,
    const std::string& url
) {
    auto listIndex = findListIndex(listId);
    if (!listIndex.has_value()) {
        return std::nullopt;
    }

    auto& data = store();
    const int problemId = data.nextProblemId++;
    auto problem = buildImportedProblem(problemId, url);
    auto note = buildImportedNote(problemId, url);

    domain::ReviewState state(
        problemId,
        domain::ReviewStatus::FirstFix,
        1,
        0,
        domain::ReviewFeedback::Failed
    );

    domain::ReviewCandidate candidate(listId, problem, state);
    data.candidates.push_back(candidate);
    data.problemLists[listIndex.value()].addProblemId(problemId);
    data.notes.emplace(problemId, note);

    return candidate;
}

bool MockReviewRepository::updateProblemWeight(int problemId, int problemUserWeight) {
    auto index = findCandidateIndexByProblemId(problemId);
    if (!index.has_value()) {
        return false;
    }

    auto& candidate = store().candidates[index.value()];
    auto state = candidate.getReviewState();
    state.setProblemUserWeight(problemUserWeight);
    candidate.setReviewState(state);
    return true;
}

bool MockReviewRepository::updateReviewState(int problemId, const domain::ReviewState& reviewState) {
    auto index = findCandidateIndexByProblemId(problemId);
    if (!index.has_value()) {
        return false;
    }

    store().candidates[index.value()].setReviewState(reviewState);
    return true;
}

} // namespace repository
} // namespace algoforge
