cmake_minimum_required(VERSION 3.15)

# 定义项目名称和版本
project(AlgoArchiveServer VERSION 1.0)

# 强制使用现代 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 强制告诉编译器目标系统是 Windows 10 或更高版本 (解决 httplib 报错)
add_definitions(-D_WIN32_WINNT=0x0A00)

option(ALGOFORGE_USE_MYSQL "Build backend with MySQL client support" OFF)

# 1. 告诉编译器去哪里找头文件 (.h / .hpp)
include_directories(${CMAKE_SOURCE_DIR}/include)

# 2. 收集 src 目录下的所有 .cpp 源文件
# 这里使用了递归查找，这样以后你在子文件夹加代码也不用改 CMake
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# 3. 生成可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 4. 链接多线程库 (Linux 下 cpp-httplib 必须依赖 pthread)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

find_package(OpenSSL QUIET)
if(OpenSSL_FOUND)
    target_compile_definitions(${PROJECT_NAME} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
else()
    message(WARNING "OpenSSL not found. Codeforces remote judge requires HTTPS and will return a clear error until OpenSSL is installed.")
endif()

if(ALGOFORGE_USE_MYSQL)
    find_path(MYSQL_INCLUDE_DIR mysql/mysql.h)
    find_library(MYSQL_CLIENT_LIBRARY mysqlclient)

    if(NOT MYSQL_INCLUDE_DIR OR NOT MYSQL_CLIENT_LIBRARY)
        message(FATAL_ERROR "MySQL client library not found. Install libmysqlclient-dev or libmariadb-dev.")
    endif()

    target_compile_definitions(${PROJECT_NAME} PRIVATE ALGOFORGE_USE_MYSQL)
    target_include_directories(${PROJECT_NAME} PRIVATE ${MYSQL_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${MYSQL_CLIENT_LIBRARY})
endif()
