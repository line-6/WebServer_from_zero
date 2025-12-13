#include "../code/buffer/buffer.h"
#include <iostream>
#include <assert.h>
#include <string.h>

void TestAppendRetrieve() {
    std::cout << "Testing Append and Retrieve..." << std::endl;
    Buffer buff;
    std::string data = "Hello, WebServer!";
    buff.Append(data);
    
    assert(buff.ReadableBytes() == data.length());
    assert(buff.WritableBytes() == 1024 - data.length()); // 默认初始化大小 1024
    
    std::string retrieved = buff.RetrieveAllToStr();
    assert(retrieved == data);
    assert(buff.ReadableBytes() == 0);
    std::cout << "Pass!" << std::endl;
}

void TestGrow() {
    std::cout << "Testing Buffer Growth..." << std::endl;
    Buffer buff;
    std::string data(2000, 'a'); // 大于默认 1024
    
    buff.Append(data);
    
    assert(buff.ReadableBytes() == 2000);
    assert(buff.WritableBytes() < 2000); // 应该扩展了
    
    std::string retrieved = buff.RetrieveAllToStr();
    assert(retrieved.length() == 2000);
    for(char c : retrieved) {
        assert(c == 'a');
    }
    std::cout << "Pass!" << std::endl;
}

int main() {
    TestAppendRetrieve();
    TestGrow();
    std::cout << "All Buffer tests passed!" << std::endl;
    return 0;
}




