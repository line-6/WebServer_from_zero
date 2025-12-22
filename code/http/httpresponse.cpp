#include "httpresponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const std::unordered_map<int, std::string> HttpResponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

HttpResponse::HttpResponse(): code_(-1), isKeepAlive_(false), path_(""), 
                            srcDir_(""), mmFile_(nullptr),
                            mmFileStat_({0}) {}

HttpResponse::~HttpResponse() {

}

void HttpResponse::init(const std::string& srcDir, std::string& path, 
        bool isKeepAlive, int code) {
    
}

void HttpResponse::makeResponse(Buffer& buffer) {
    /* 路径不存在或是目录 */
    if (stat((srcDir_ + path_).data(), &mmFileStat_) < 0 ||
        S_ISDIR(mmFileStat_.st_mode)) {
        code_ = 404;
    }
    /* 无读权限 */
    else if (!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    else if (code_ == -1) {
        code_ = 200;
    }
    errorHtml_();
    addStateLine_(buffer);
    addHeader_(buffer);
    addContent_(buffer);
}

void HttpResponse::addStateLine_(Buffer& buffer) {

}

void HttpResponse::addHeader_(Buffer& buffer) {

}

void HttpResponse::addContent_(Buffer& buffer) {
    
}
