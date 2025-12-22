```
[客户端] --> [Socket] --> [HttpConn::read()] --> [Buffer] 
                                            --> [HttpRequest::parse()] 
                                            --> [HttpResponse::MakeResponse()] 
                                            --> [Buffer] 
                                            --> [HttpConn::write()] --> [客户端]
```
# httpconn
HttpConn类 代表一个 HTTP 连接，负责：  
1. I/O 操作（读取请求、写入响应）
2. 协调 HttpRequest 和 HttpResponse
3. 管理连接生命周期

# httprequest
HttpRequest类 负责解析 HTTP 请求：  
1. 请求行（方法、路径、版本）
2. 请求头
3. 请求体（POST 数据）  

请求报文样例
```
GET /index.html HTTP/1.1\r\n          ← 请求行
Host: www.example.com\r\n             ← 请求头
Content-Type: text/html\r\n           ← 请求头
\r\n                                  ← 空行（分隔头部和体）
[请求体数据]                           ← 请求体（POST 请求才有）
```

采用如下状态机模型：
```
              ┌─────────────┐
              │ REQUEST_LINE│ ←─── 初始状态
              └──────┬──────┘
                     │ ParseRequestLine_() 成功
                     ↓
              ┌─────────────┐
       ------ │   HEADERS   │ ←─── 解析请求头
       │      └──────┬──────┘
       │             │ 遇到空行（\r\n）
       │             ↓
Buffer │       ┌─────────────┐
only:  │       │    BODY     │ ←─── 解析请求体（如果有）
/r/n   │       └──────┬──────┘
       │              │ ParseBody_()
       │              ↓
       │      ┌─────────────┐
       │----->│   FINISH    │ ←─── 解析完成
              └─────────────┘
```

# httpresponse
HttpResponse类 负责生成 HTTP 响应：  
1. 生成响应行、响应头
2. 使用 mmap 映射文件到内存
3. 处理错误页面  

响应状态行样例
```
HTTP/1.1 200 OK\r\n                    ← 响应行（状态行）
Connection: keep-alive\r\n             ← 响应头
Content-type: text/html\r\n
Content-length: 1234\r\n
\r\n                                   ← 空行（分隔头部和体）
[文件内容]                              ← 响应体
```