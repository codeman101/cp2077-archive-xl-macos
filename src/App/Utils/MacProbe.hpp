#pragma once

// R22 (macOS, CODEWARE_MAC_DIAG=1 only, read-only). Does the game's depot serve the resources of the loose mod archives in archive/Mac/mod,
// and which groups does it hold? The result decides whether ArchiveXL has to mount that folder itself.
namespace App::MacProbe
{
// Logs CheckResource for Lizzie's journal and quest phase, the first resource hash of each loose archive, and every depot group.
void Run(const char* aWhere);

// With ARCHIVEXL_MAC_NOMOUNT=1 the plugin does not mount archive/Mac/mod (a probe boot that shows what the game does on its own).
bool NoMountRequested();

// Extensions register a read-only statistics line here; Run() calls every registered function with its label ("InitResourceDepot", "postLoadScripts",
// "LoadGatheredResources", "t+50s").
void AddStats(void (*aFunction)(const char* aWhere));

// Runs Run("t+<seconds>s") once on a detached thread after the delay (diagnostics only).
void RunLater(unsigned aSeconds);
}
