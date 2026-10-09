/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include "processmemory.h"
#include <QFile>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#endif

QJsonObject mu::notation::processMemoryDiagnostics()
{
    QJsonObject result;
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
    return result;
}
