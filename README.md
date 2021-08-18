# PerformaceSummary
###### Windows性能测试组件
###### 实时上报物理内存，虚拟内存，系统cpu, 进程cpu, cycle数
###### 可以方便快速集成在任何demo中

## 集成示例如下：

```
#include <iostream>
#include "PerformanceSummary.h"
#include <iostream>
#include <process.h>

#pragma comment(lib, "perform_summary_sdk.lib")

class MyPerformDataReport : public PerformDataReport
{
    void onPerformDataReport(const PerformanceData& pf)
    {
        std::cout << "ProcessCpuCycles：" << pf.ProcessCpuCycles << ","
            << "ProcessPhyMem: " << pf.ProcessPhyMem << ","
            << "ProcessVirtMem: " << pf.ProcessVirtMem << ","
            << "ProcessCpuUsage: " << pf.ProcessCpuUsage << ","
            << "SystemCpuUsage: " << pf.SystemCpuUsage << ".\n";
    };
};

int main()
{   
    // create engine class
    PerformSummaryEngine* engine = createPerformSummaryEngine();
    // create data report class
    MyPerformDataReport* report = new MyPerformDataReport();
    // initialize context stuct
    PerformSummaryEngineContext context;
    context.performDataRefort = report;
    context.pid = _getpid();
    context.span = 1000;
    engine->initialize(context);
    // start collecting
    engine->startTimer();
    // setting the collection Time
    Sleep(100000);
    // stop collecting
    engine->stopTimer();
    // release related resources
    engine->release();

}
```
## 效果展示如下：控制台应用程序为例
![image](https://user-images.githubusercontent.com/33676214/129882180-77cd9475-6b05-490f-9079-7a755f502970.png)

