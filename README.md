# WebServer_from_zero
**分阶段从零到一开发WebServer**
## 阶段
Step 1.缓冲区模块 (Buffer): 封装自动增长的读写缓冲区，是数据处理的基础。
Step 2.日志系统 (Log): 实现同步/异步日志，方便后续调试。
Step 3.池化技术 (Pool):
Step 4.线程池 (ThreadPool): 管理工作线程，处理高并发任务。
Step 5.数据库连接池 (SqlConnPool): 复用 MySQL 连接 (RAII 机制)。
Step 6.核心组件 (Core Components):
·定时器 (Timer): 基于小根堆管理超时连接。
·Epoll 封装: 管理文件描述符事件。
Step 7.HTTP 协议栈 (Http):
·解析 HTTP 请求 (状态机)。
·构建 HTTP 响应。
