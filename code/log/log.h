#pragma once

class Log {
private:
    Log();
    virtual ~Log();

public:
    void init(int level, const char* path = "./log",
                const char* suffix = ".log", 
                int maxQueueCapacity = 1024);

    static Log* Instance();


}; 