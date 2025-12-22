#pragma once

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void init(const std::string& srcDir, std::string& path, 
            bool isKeepAlive = false, int code = -1);

    void makeResponse(Buffer& buffer);


private:
    void errorHtml_();
    void addStateLine_(Buffer& buffer);
    void addHeader_(Buffer& buffer);
    void addContent_(Buffer& buffer);

    int code_; // HTTP状态码
    bool isKeepAlive_;  // 是否保持连接
    std::string path_;  // 请求文件路径
    std::string srcDir_;    // 请求文件目录

    char* mmFile_;  // mmap 映射的文件内存地址
    struct stat mmFileStat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
};