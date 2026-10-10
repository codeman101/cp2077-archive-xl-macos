#pragma once

namespace Mac
{
// Installs a handler that prints the fault and the top frames on a fatal signal (see MacCrashDiag.cpp). Safe to call more than once.
void InstallCrashDiag();
}
