#include <future>
#include <iostream>
#include "logger.h"

using namespace std;

std::atomic<int> commonData = 0;
std::mutex mtx;

int slowAddTwoPositiveNumAsync(int a, int b)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    if(a <= 0 || b <= 0)
    {
        throw std::runtime_error("Invalid Positive Number");
    }
    return a+b;
}
void printMyName ()
{
    for(int i = 0; i < 1000000; i++)
    {
        ++commonData;
        if(i%1000 == 0)
        {
            TM_LOG_DEBUG("Nam {}", i);
        }
        if ((i & 2047) == 0) std::this_thread::yield();
    }
}

int main()
{
    treeManagement::LoggingConfig LogConfig;
    LogConfig.filePath = "tree.log";
    LogConfig.maxFileSize = 5;
    LogConfig.pattern = "%Y-%m-%d %H:%M:%S.%e [%^%l%$] %v";
    LogConfig.maxFiles = 3;
    LogConfig.level = "debug";
    std::vector<treeManagement::SinkType> sinkTypes = {treeManagement::SinkType::Console, treeManagement::SinkType::File};
    LogConfig.sinks = sinkTypes;

    treeManagement::Logger::instance().Init(LogConfig);

    /*** SIMPLE THREAD
    std::thread newThread(printMyName);
    std::thread newThread1(printMyName);
    std::thread newThread2(printMyName);
    std::thread newThread3(printMyName);
    std::thread newThread4(printMyName);
    std::thread newThread5(printMyName);
    newThread.join();
    newThread1.join();
    newThread2.join();
    newThread3.join();
    newThread4.join();
    newThread5.join();

    long long expected = 6LL * 1000000;
    std::cout << "expected=" << expected << "  actual=" << commonData << "\n";

    TM_LOG_ERROR("Main Stoped After Thread ?")
    ***/

    int a,b;
    std::cout<<"INPUT a number: ";
    std::cin>>a;
    std::cout<<"INPUT b number: ";
    std::cin>>b;
    try{
       std::future future = std::async(std::launch::async,slowAddTwoPositiveNumAsync,a,b);
        while(true)
        {
            std::future_status status = future.wait_for(std::chrono::milliseconds(1000));
            if(status == std::future_status::ready)
            {
                TM_LOG_DEBUG("Slow Add Result: {}",future.get());
                break;
            }
            else
            {
                TM_LOG_DEBUG("Polling Slow Add Result...");
            }
        }
    }
    catch (const std::exception &e)
    {
        TM_LOG_ERROR("ERROR: {}",e.what());
    }
    const int seconds = 5;
    std::chrono::seconds time_sleep(seconds);
    TM_LOG_INFO("Console will close in {} seconds", seconds);
    std::this_thread::sleep_for(time_sleep);
    return 0;
}
