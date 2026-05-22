#include "domain/Note.h"

#include <utility>

namespace algoforge {
namespace domain {

Note::Note(
    int id,
    int problemId,
    std::string hintMarkdown,
    std::string noteMarkdown
)
    : id_(id),
      problemId_(problemId),
      hintMarkdown_(std::move(hintMarkdown)),
      noteMarkdown_(std::move(noteMarkdown)) {}

int Note::getId() const {
    return id_;
}

int Note::getProblemId() const {
    return problemId_;
}

const std::string& Note::getHintMarkdown() const {
    return hintMarkdown_;
}

const std::string& Note::getNoteMarkdown() const {
    return noteMarkdown_;
}

void Note::setId(int id) {
    id_ = id;
}

void Note::setProblemId(int problemId) {
    problemId_ = problemId;
}

void Note::setHintMarkdown(const std::string& hintMarkdown) {
    hintMarkdown_ = hintMarkdown;
}

void Note::setNoteMarkdown(const std::string& noteMarkdown) {
    noteMarkdown_ = noteMarkdown;
}

bool Note::hasHint() const {
    return !hintMarkdown_.empty();
}

bool Note::hasNote() const {
    return !noteMarkdown_.empty();
}

} // namespace domain
} // namespace algoforge