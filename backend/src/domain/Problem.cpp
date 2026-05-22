#include "domain/Problem.h"

#include <utility>

namespace algoforge {
namespace domain {

Problem::Problem(
    int id,
    std::string title,
    std::string oj,
    std::string url,
    std::string difficulty
)
    : id_(id),
      title_(std::move(title)),
      oj_(std::move(oj)),
      url_(std::move(url)),
      difficulty_(std::move(difficulty)) {}

int Problem::getId() const {
    return id_;
}

const std::string& Problem::getTitle() const {
    return title_;
}

const std::string& Problem::getOj() const {
    return oj_;
}

const std::string& Problem::getUrl() const {
    return url_;
}

const std::string& Problem::getDifficulty() const {
    return difficulty_;
}

const std::vector<std::string>& Problem::getTags() const {
    return tags_;
}

const std::string& Problem::getStatementMarkdown() const {
    return statementMarkdown_;
}

const std::string& Problem::getInputDescription() const {
    return inputDescription_;
}

const std::string& Problem::getOutputDescription() const {
    return outputDescription_;
}

const std::string& Problem::getSampleInput() const {
    return sampleInput_;
}

const std::string& Problem::getSampleOutput() const {
    return sampleOutput_;
}

void Problem::setId(int id) {
    id_ = id;
}

void Problem::setTitle(const std::string& title) {
    title_ = title;
}

void Problem::setOj(const std::string& oj) {
    oj_ = oj;
}

void Problem::setUrl(const std::string& url) {
    url_ = url;
}

void Problem::setDifficulty(const std::string& difficulty) {
    difficulty_ = difficulty;
}

void Problem::setTags(const std::vector<std::string>& tags) {
    tags_ = tags;
}

void Problem::setStatementMarkdown(const std::string& statementMarkdown) {
    statementMarkdown_ = statementMarkdown;
}

void Problem::setInputDescription(const std::string& inputDescription) {
    inputDescription_ = inputDescription;
}

void Problem::setOutputDescription(const std::string& outputDescription) {
    outputDescription_ = outputDescription;
}

void Problem::setSampleInput(const std::string& sampleInput) {
    sampleInput_ = sampleInput;
}

void Problem::setSampleOutput(const std::string& sampleOutput) {
    sampleOutput_ = sampleOutput;
}

} // namespace domain
} // namespace algoforge
