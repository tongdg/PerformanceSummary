
#include <windows.h>

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

PerformSummaryEngine* createPerformSummaryEngine();
