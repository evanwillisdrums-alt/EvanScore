/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include "processmemory.h"
#include <QFile>
#include <algorithm>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#endif

QJsonObject mu::notation::systemMemoryDiagnostics()
{
    QJsonObject result;
    double total=0,available=-1;
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);
    if(GlobalMemoryStatusEx(&memory)) {total=double(memory.ullTotalPhys);available=double(memory.ullAvailPhys);}
#elif defined(Q_OS_LINUX)
    QFile memory("/proc/meminfo");
    if(memory.open(QIODevice::ReadOnly))for(const auto& line:memory.readAll().split('\n')) {
        if(line.startsWith("MemTotal:"))total=line.mid(9).simplified().split(' ').front().toDouble()*1024;
        if(line.startsWith("MemAvailable:"))available=line.mid(13).simplified().split(' ').front().toDouble()*1024;
    }
#endif
    if(total>0 && available>=0) {
        result.insert("systemTotalBytes",total);result.insert("systemAvailableBytes",available);
        result.insert("systemUsedPercent",std::clamp((total-available)*100/total,0.0,100.0));
    }
    return result;
}
QJsonObject mu::notation::processMemoryDiagnostics()
{
    QJsonObject result=systemMemoryDiagnostics();
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX memory {};
    memory.cb = sizeof(memory);
    if (K32GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory))) {
        result.insert("workingSetBytes", static_cast<double>(memory.WorkingSetSize));
        result.insert("privateBytes", static_cast<double>(memory.PrivateUsage));
    }
#elif defined(Q_OS_LINUX)
    QFile status(QStringLiteral("/proc/self/status"));
    if (status.open(QIODevice::ReadOnly)) {
        for (const auto& line : status.readAll().split('\n')) {
            if (line.startsWith("VmRSS:")) {
                result.insert("workingSetBytes", line.mid(6).simplified().split(' ').first().toDouble() * 1024);
            }
        }
    }
#endif
    const double total=result.value("systemTotalBytes").toDouble();
    if(total>0 && result.contains("workingSetBytes"))result.insert("processPhysicalMemoryPercent",result.value("workingSetBytes").toDouble()*100/total);
    return result;
}
