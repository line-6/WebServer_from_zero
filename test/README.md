# 调试过程中遇到的BUG
### 主程序卡死在 epoller_->wait，接收不到连接请求，检查了listenEvent_的设置和端口设置都没发现问题。@518b1c7
解决：对listenFd_的port使用了htonl()而非htons(),造成sockaddrin的sin_port值错误。导致主程序一直在监听错误的端口.
### 访问图片资源时发生404错误，其他资源均无此问题。@a3494ff
解决：追查发现访问图片的HttpRequest的path_路径不对，进一步发现HttpRequest::DEFAULT_HTML的"picture"应为"/picture".