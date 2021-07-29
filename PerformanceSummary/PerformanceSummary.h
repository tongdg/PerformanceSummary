// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 PERFORMANCESUMMARY_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// PERFORMANCESUMMARY_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。

#include <windows.h>
#ifdef PERFORMANCESUMMARY_EXPORTS
#define PERFORMANCESUMMARY_API __declspec(dllexport)
#else
#define PERFORMANCESUMMARY_API __declspec(dllimport)
#endif

typedef struct _PerformanceData {
	ULONGLONG ProcessCpuCycles;
	double ProcessPhyMem;
	double ProcessVirtMem;
	double ProcessCpuUsage;
	double SystemCpuUsage;
} PerformanceData;

enum PERFORM_SUMMARY_ERR_CODE
{
	ERR_PERFORM_NOT_INITIALIZE = -10001,
	ERR_OPEN_PROCESS_FAIL = -10002,
	ERR_TIMER_ALREADY_STARTED = -10003,
	ERR_TIMER_ALREADY_STOPPED = -10004,
	ERR_TIMER_NOT_OVER_YET = -10005
};

class PerformDataReport
{
public:
	virtual void onPerformDataReport(const PerformanceData &pf) {};
};

typedef struct _PerformSummaryEngineContext {
	PerformDataReport* performDataRefort;
	DWORD pid;
	int span;
}PerformSummaryEngineContext;

class PerformSummaryEngine
{
protected:
	virtual ~PerformSummaryEngine() {};
public:
	virtual int initialize(const PerformSummaryEngineContext &ctx) = 0;
	virtual int startTimer() = 0;
	virtual int stopTimer() = 0;
	virtual int release() = 0;
};

PERFORMANCESUMMARY_API PerformSummaryEngine* createPerformSummaryEngine();
