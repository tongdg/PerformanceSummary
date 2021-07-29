
#include "stdafx.h"
#include "pch.h"
#include "framework.h"
#include "PerformanceSummary.h"
#include <psapi.h>
#include <stdarg.h>
#include <stdlib.h>
#include <Windows.h>
#include <thread>
#include <mmsystem.h>

#pragma comment(lib,"Winmm.lib")

typedef struct _SystemSummary {
	ULONGLONG PhyMem;
	ULONGLONG VirtMem;
	ULONGLONG IdleTime;
	ULONGLONG KernelTime;
	ULONGLONG UserTime;
}SystemSummary;

typedef struct _ProcessSummary {
	ULONGLONG CpuCycles;
	ULONGLONG PhyMem;
	ULONGLONG VirtMem;
	ULONGLONG RunningTime;
} ProcessSummary;

typedef struct _ThreadSummary {
	ULONGLONG CpuCycles;
	ULONGLONG RunningTime;
	int Priority;
} ThreadSummary;

class PerformSummary : public PerformSummaryEngine
{

private:
	PerformSummaryEngineContext pc;
	SystemSummary sysStartSummary;
	ProcessSummary proStartSummary;
	bool timerFlag = false;
	static PerformSummary* performSummary;
	MMRESULT timerId;
	bool initializeStatus = false;

private:
	PerformSummary() {};
	~PerformSummary();
	int SummaryProcess(DWORD pid);
	ULONGLONG CalculateFileTime(FILETIME filetime);
	double CalculaSysCpuUsageRate(SystemSummary &start, SystemSummary &end);
	ULONGLONG CalculaSysCpuTotalTime(SystemSummary &start, SystemSummary &end);
	ULONGLONG GetCurrentProcessTime(HANDLE p);
	ULONGLONG GetCurrentThreadTime(HANDLE t);
	ProcessSummary GetProcessSummary(HANDLE p);
	SystemSummary GetSystemSummary();
	void setTimer();
	static void WINAPI onTimeFunc(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dwl, DWORD dw2);

public:
	static PerformSummary* createPerformSummaryEngine();
	virtual int initialize(const PerformSummaryEngineContext &ctx)override;
	virtual int startTimer()override;
	virtual int stopTimer()override;
	virtual int release()override;
};

ULONGLONG PerformSummary::CalculateFileTime(FILETIME filetime)
{
	ULONGLONG time = (((ULONGLONG)filetime.dwHighDateTime << 32) |
		(ULONGLONG)(filetime.dwLowDateTime));
	return time;
}

double PerformSummary::CalculaSysCpuUsageRate(SystemSummary &start, SystemSummary &end)
{
	double sysCpuUsageRate = static_cast<double>(((end.KernelTime - start.KernelTime) +
		(end.UserTime - start.UserTime) -
		(end.IdleTime - start.IdleTime)) * 100) /
		((end.KernelTime - start.KernelTime) +
		(end.UserTime - start.UserTime));
	return sysCpuUsageRate;
}

ULONGLONG PerformSummary::CalculaSysCpuTotalTime(SystemSummary &start, SystemSummary &end)
{
	ULONGLONG sysCpuTotalTime = ((end.KernelTime - start.KernelTime) +
		(end.UserTime - start.UserTime)) / 10000;
	return sysCpuTotalTime;
}


ULONGLONG PerformSummary::GetCurrentProcessTime(HANDLE p) {
	FILETIME kernel, user, c, e;
	GetProcessTimes(p, &c, &e, &kernel, &user);
	ULONGLONG time = (((ULONGLONG)user.dwHighDateTime << 32) |
		(ULONGLONG)(user.dwLowDateTime)) +
		(((ULONGLONG)kernel.dwHighDateTime << 32) |
		(ULONGLONG)(kernel.dwLowDateTime));

	time = time / 10000;
	return time;
}

ULONGLONG PerformSummary::GetCurrentThreadTime(HANDLE t) {
	FILETIME kernel, user, c, e;
	GetThreadTimes(t, &c, &e, &kernel, &user);
	ULONGLONG time = (((ULONGLONG)user.dwHighDateTime << 32) |
		(ULONGLONG)(user.dwLowDateTime)) +
		(((ULONGLONG)kernel.dwHighDateTime << 32) |
		(ULONGLONG)(kernel.dwLowDateTime));

	time = time / 10000;
	return time;
}

SystemSummary PerformSummary::GetSystemSummary()
{
	SystemSummary summary = { 0 };
	FILETIME kernel, user, idle;
	GetSystemTimes(&idle, &kernel, &user);
	summary.KernelTime = CalculateFileTime(kernel);
	summary.UserTime = CalculateFileTime(user);
	summary.IdleTime = CalculateFileTime(idle);
	return summary;
}

ProcessSummary PerformSummary::GetProcessSummary(HANDLE p) {

	ProcessSummary summary = { 0 };
	ULONGLONG cycle = 0;
	QueryProcessCycleTime(p, &cycle);

	summary.CpuCycles = cycle;
	summary.RunningTime = GetCurrentProcessTime(p);

	PROCESS_MEMORY_COUNTERS_EX info = { 0 };
	info.cb = sizeof(info);
	GetProcessMemoryInfo(p, (PROCESS_MEMORY_COUNTERS *)&info, sizeof(info));

	summary.VirtMem = info.PrivateUsage;
	summary.PhyMem = info.WorkingSetSize;
	return summary;
}

int PerformSummary::SummaryProcess(DWORD pid)
{
	SystemSummary sysEndSummary = GetSystemSummary();
	double sysCpuUsageRate = CalculaSysCpuUsageRate(sysStartSummary, sysEndSummary);
	double sysCpuTotalTime = CalculaSysCpuTotalTime(sysStartSummary, sysEndSummary);

	HANDLE p = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (p == NULL)
		return 0;
	ProcessSummary proEndSummary = GetProcessSummary(p);
	CloseHandle(p);
	double proCpuUsage = 0.0;
	if (sysCpuTotalTime) {
		proCpuUsage = static_cast<double>(proEndSummary.RunningTime - proStartSummary.RunningTime) / sysCpuTotalTime;
		proCpuUsage *= 100.0;
	}

	PerformanceData pfObj = { 0 };
	pfObj.ProcessCpuCycles = proEndSummary.CpuCycles - proStartSummary.CpuCycles;
	pfObj.ProcessCpuUsage = proCpuUsage;
	pfObj.ProcessPhyMem = (static_cast<double>(proEndSummary.PhyMem) / (1024 * 1024));
	pfObj.ProcessVirtMem = (static_cast<double>(proEndSummary.VirtMem) / (1024 * 1024));
	pfObj.SystemCpuUsage = sysCpuUsageRate;
	std::thread run(&PerformDataReport::onPerformDataReport, pc.performDataRefort, pfObj);
	run.detach();
	sysStartSummary = sysEndSummary;
	proStartSummary = proEndSummary;
	return 0;
}

PerformSummary* PerformSummary::performSummary = NULL;


void WINAPI PerformSummary::onTimeFunc(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dwl, DWORD dw2)
{
	PerformSummary *ps = PerformSummary::performSummary;
	if (ps == NULL)return;
	std::thread run(&PerformSummary::SummaryProcess, ps, ps->pc.pid);
	run.detach();
}


PerformSummary* PerformSummary::createPerformSummaryEngine()
{
	if (performSummary == NULL)
	{
		performSummary = new PerformSummary();
		return performSummary;
	}
	else
		return performSummary;
}

int PerformSummary::initialize(const PerformSummaryEngineContext &ctx)
{
	pc = ctx;
	HANDLE p = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pc.pid);
	if (p == NULL)
	{
		initializeStatus = false;
		return ERR_OPEN_PROCESS_FAIL;
	}
	sysStartSummary = GetSystemSummary();
	proStartSummary = GetProcessSummary(p);
	CloseHandle(p);
	initializeStatus = true;
	return 0;
}

void PerformSummary::setTimer()
{
	timerId = timeSetEvent(pc.span, 1, (LPTIMECALLBACK)onTimeFunc, (DWORD)NULL, TIME_PERIODIC);
	timerFlag = true;
}

int PerformSummary::startTimer()
{
	if (!initializeStatus)
		return ERR_PERFORM_NOT_INITIALIZE;
	if (!timerFlag) {
		std::thread run(&PerformSummary::setTimer, this);
		run.detach();
	}
	else
		return ERR_TIMER_ALREADY_STARTED;
	return 0;
}

int PerformSummary::stopTimer()
{
	if (!initializeStatus)
		return ERR_PERFORM_NOT_INITIALIZE;
	if (timerFlag) {
		timeKillEvent(timerId);
		timerFlag = false;
	}
	else
		return ERR_TIMER_ALREADY_STOPPED;
	return 0;
}

int PerformSummary::release()
{
	if (!initializeStatus)
		return ERR_PERFORM_NOT_INITIALIZE;
	if (timerFlag)
		return ERR_TIMER_NOT_OVER_YET;
	if (performSummary != NULL) {
		delete performSummary;
		performSummary = NULL;
	}
	return 0;
}

PerformSummary::~PerformSummary()
{
	
}

PerformSummaryEngine* createPerformSummaryEngine()
{
	return PerformSummary::createPerformSummaryEngine();
}
