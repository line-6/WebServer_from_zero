```
[客户端] --> [Socket] --> [HttpConn::read()] --> [Buffer] 
                                            --> [HttpRequest::parse()] 
                                            --> [HttpResponse::MakeResponse()] 
                                            --> [Buffer] 
                                            --> [HttpConn::write()] --> [客户端]
```
## httpconn
HttpConn类 代表一个 HTTP 连接，负责：  
1. I/O 操作（读取请求、写入响应）
2. 协调 HttpRequest 和 HttpResponse
3. 管理连接生命周期

# httprequest
HttpRequest类 负责解析 HTTP 请求：  
1. 请求行（方法、路径、版本）
2. 请求头
3. 请求体（POST 数据）  

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
only   │       │    BODY     │ ←─── 解析请求体（如果有）
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