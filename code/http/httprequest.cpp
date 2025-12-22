#include "httprequest.h"
#include "pool/sqlconnRAII.h"
#include "pool/sqlconnpool.h"
#include <mysql/mysql.h>

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index", "/register", "/login", "welcome",
    "/video", "picture",
};

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG {
    {"/register.html", 0}, {"/login.html", 1},
};


void HttpRequest::init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

std::string HttpRequest::GetPost(const std::string& key) const {
    assert(key != "");
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}
std::string HttpRequest::GetPost(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

bool HttpRequest::IsKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
}

bool HttpRequest::parse(Buffer& buffer) {
    const char CRLF[] = "\r\n";
    if (buffer.ReadableBytes() <= 0) {
        return false;
    }
    while (buffer.ReadableBytes() && state_ != FINISH) {
        // 搜索 '\r\n' 第一次在buffer的出现位置
        const char* lineEnd = std::search(buffer.ReadPtr(),
                    buffer.WritePtrConst(), CRLF, CRLF + 2);
        
        // 拿到一行的数据
        std::string line(buffer.ReadPtr(), lineEnd);
        // 根据当前的不同状态解析此行数据
        switch(state_) {
            case REQUEST_LINE: {
                if (!parseRequestLine_(line)) {
                    return false;
                }
                parsePath_();
            }break;

            case HEADERS: {
                parseHeader_(line);
                if (buffer.ReadableBytes() <= 2) {
                    state_ = FINISH;
                }
            }break;

            case BODY: {
                parseBody_(line);
            }break;

            default: break;
        }
        if (lineEnd == buffer.WritePtr()) {break;}
        buffer.RetrieveUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

bool HttpRequest::parseRequestLine_(const std::string& line) {
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, patten)) {
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::parsePath_() {
    if (path_ == "/") {
        path_ = "/index.html";
    }
    else {
        for (auto &item : DEFAULT_HTML) {
            if (item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

void HttpRequest::parseHeader_(const std::string& line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, patten)) {
        header_[subMatch[1]] = subMatch[2];
    }
    else {
        state_ = BODY;
    }
}

void HttpRequest::parseBody_(const std::string& line) {
    body_ = line;
    parsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

void HttpRequest::parsePost_() {
    if(method_ == "POST" && header_["Content-Type"] == 
                "application/x-www-form-urlencoded") {
        parseFromUrlencoded_();
        if(DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if (tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if (userVerify(post_["username"], 
                    post_["password"], isLogin)) {
                    path_ = "/welcome.html";
                }
                else {
                    path_ = "/error.html";
                }
            }
        }
    }
}

void HttpRequest::parseFromUrlencoded_() {
    if(body_.size() == 0) { return; }
    
    std::string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for (; i < n; i ++) {
        char ch = body_[i];
        switch (ch) {
            case '=': {
                key = body_.substr(j, i - j);
                j = i + 1;
            }break;

            case '+': {
                /* '+' 解释为空格 */
                body_[i] = ' ';
            }break;

            case '%': {
                num = converHex(body_[i + 1] * 16 + converHex(body_[i + 2] * 16));
                body_[i + 2] = num % 10 + '0';
                body_[i + 1] = num / 10 + '0';
                i += 2;
            }break;

            case '&': {
                value = body_.substr(j, i - j);
                j = i + 1;
                post_[key] = value;
                LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            }break;

            default: break;
        }   
    }
    assert(j <= i);
    
    // 尾部处理
    if(post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

bool HttpRequest::userVerify(const std::string& name, 
                        const std::string& pwd, bool isLogin) {
    if(name == "" || pwd == "") { return false; }
    LOG_INFO("Verify User, name:%s pwd:%s", name.c_str(), pwd.c_str());  

    SqlConnRAII raii(SqlConnPool::Instance());
    MYSQL* sql = raii.get();
    assert(sql);

    bool flag = false;  // return value
    char order[256] = { 0 };    // mysql 语句
    MYSQL_RES *res = nullptr;   // 储存查询结果

    if (!isLogin) {flag = true;}
    snprintf(order, 256, 
    "SELECT username, password FROM user WHERE username='%s' LIMIT 1",
    name.c_str());
    LOG_DEBUG("Run MYSQL: %s", order);

    if (mysql_query(sql, order)) {
        // 查询失败
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);

    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("Read MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);

        if (isLogin) {
            if (pwd == password) {flag = true;}
            else {
                flag = false;
                LOG_DEBUG("Read MYSQL: pwd error!");
            }
        }
        else {
            flag = false;
            LOG_DEBUG("Read MYSQL: user used!");
        }
    }
    mysql_free_result(res);

    if (!isLogin && flag == true) {
        LOG_DEBUG("Write MYSQL: user regirster!");
        bzero(order, 256);
        snprintf(order, 256,
         "INSERT INTO user(username, password) VALUES('%s','%s')",
         name.c_str(), pwd.c_str());
        LOG_DEBUG("Run MYSQL: %s", order);
        if (mysql_query(sql, order)) {
            LOG_DEBUG("Write MYSQL error!");
            flag = false;
        }
        flag = true;
    }
    SqlConnPool::Instance()->freeConn(sql);
    LOG_DEBUG( "Read MYSQL: UserVerify success!");
    return flag;
}

/*16进制 -> 10进制*/
int HttpRequest::converHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}