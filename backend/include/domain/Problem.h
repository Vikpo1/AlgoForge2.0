#pragma once

#include <vector>
#include <string>

namespace algoforge {
namespace domain {

class Problem {
public:
    Problem() = default;

    Problem(
        int id,
        std::string title,
        std::string oj,
        std::string url,
        std::string difficulty
    );

    int getId() const;
    const std::string& getTitle() const;
    const std::string& getOj() const;
    const std::string& getUrl() const;
    const std::string& getDifficulty() const;
    const std::vector<std::string>& getTags() const;
    const std::string& getStatementMarkdown() const;
    const std::string& getInputDescription() const;
    const std::string& getOutputDescription() const;
    const std::string& getSampleInput() const;
    const std::string& getSampleOutput() const;

    void setId(int id);
    void setTitle(const std::string& title);
    void setOj(const std::string& oj);
    void setUrl(const std::string& url);
    void setDifficulty(const std::string& difficulty);
    void setTags(const std::vector<std::string>& tags);
    void setStatementMarkdown(const std::string& statementMarkdown);
    void setInputDescription(const std::string& inputDescription);
    void setOutputDescription(const std::string& outputDescription);
    void setSampleInput(const std::string& sampleInput);
    void setSampleOutput(const std::string& sampleOutput);

private:
    int id_ = 0;
    std::string title_;
    std::string oj_;
    std::string url_;
    std::string difficulty_;
    std::vector<std::string> tags_;
    std::string statementMarkdown_;
    std::string inputDescription_;
    std::string outputDescription_;
    std::string sampleInput_;
    std::string sampleOutput_;
};

} // namespace domain
} // namespace algoforge
