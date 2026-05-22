#pragma once

#include "httplib.h"

namespace algoforge {
namespace http {

// 以后所有后端接口都统一在这个函数里注册。
void registerRoutes(httplib::Server& server);

} // namespace http
} // namespace algoforge