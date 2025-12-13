# WebServer_from_zero
**分阶段从零到一开发WebServer，每阶段均可直接运行**
## 阶段
### Step 1.缓冲区模块 (Buffer): 封装自动增长的读写缓冲区，是数据处理的基础。  
```
client->server:

[客户端] ---> [网络] ---> [OS 内核缓冲区] 
                              |
                              v (sys_read / readv)
                       [用户态 Input Buffer]  <--- buffer 模块
                              |
                              v (解析)
                       [HttpConn / HttpRequest]
```
```
server->client:

[HttpConn / HttpResponse] --> [生成响应数据]
                              |
                              v (append)
                       [用户态 Output Buffer] <--- buffer 模块
                              |
                              v (sys_write)
                       [OS 内核缓冲区]
                              |
                              v
[客户端] <--- [网络] <-------+
```
### Step 2.日志系统 (Log): 实现同步/异步日志，方便后续调试。  

### Step 3.池 (Pool):  
1. 线程池 (ThreadPool): 管理工作线程，处理高并发任务。  
2. 数据库连接池 (SqlConnPool): 复用 MySQL 连接 (RAII 机制)。  

### Step 4.核心组件 (Core Components):  
1. 定时器 (Timer): 基于小根堆管理超时连接。  
2. Epoll 封装: 管理文件描述符事件。  

### Step 5.HTTP 协议栈 (Http):  
1. 解析 HTTP 请求 (状态机)。  
2. 构建 HTTP 响应。  
