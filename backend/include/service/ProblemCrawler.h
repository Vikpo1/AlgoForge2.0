#pragma once

#include "domain/Problem.h"

#include <string>

namespace algoforge {
namespace service {

struct ProblemCrawlResult {
    bool success = false;
    domain::Problem problem;
    std::string errorMessage;
};

ProblemCrawlResult crawlProblem(int problemId, const std::string& url);

} // namespace service
} // namespace algoforge
