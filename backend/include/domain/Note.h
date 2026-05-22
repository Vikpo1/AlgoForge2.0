#pragma once

#include <string>

namespace algoforge {
namespace domain {

class Note {
public:
    Note() = default;

    Note(
        int id,
        int problemId,
        std::string hintMarkdown,
        std::string noteMarkdown
    );

    int getId() const;
    int getProblemId() const;
    const std::string& getHintMarkdown() const;
    const std::string& getNoteMarkdown() const;

    void setId(int id);
    void setProblemId(int problemId);
    void setHintMarkdown(const std::string& hintMarkdown);
    void setNoteMarkdown(const std::string& noteMarkdown);

    // 判断是否存在轻提示
    bool hasHint() const;

    // 判断是否存在完整笔记
    bool hasNote() const;

private:
    int id_ = 0;

    // 这条笔记属于哪一道题
    int problemId_ = 0;

    // 轻提示：复习时可以选择先展开
    std::string hintMarkdown_;

    // 完整复盘笔记：默认隐藏，完成思考后再展开
    std::string noteMarkdown_;
};

} // namespace domain
} // namespace algoforge