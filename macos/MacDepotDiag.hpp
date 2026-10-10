#pragma once

// R22 module a. Read-only diagnostics for the game's ResourceDepot on macOS. Everything is copied with vm_read_overwrite, so a wrong
// layout prints garbage instead of crashing. Prints only with CODEWARE_MAC_DIAG=1.

#include <string>

namespace Mac
{
// Copies aSize bytes from aSrc. False when any of the range is not readable.
bool SafeRead(const void* aSrc, void* aDst, size_t aSize);

// The text of a Red::CString at aCString (SDK layout: inline text or pointer, length at +0x14, bit 30 = heap), or "<unreadable>".
std::string SafeCString(const void* aCString);

// Logs the depot object, its group array and the first archives of the first groups (raw words and the decoded fields).
void DumpDepot(const void* aDepot, const char* aWhere);
}
