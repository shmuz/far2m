#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 1,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x05E91C96;
  aInfo->Version       = Version;
  aInfo->Title         = L"Image Viewer";
  aInfo->Description   = L"Image Viewer";
  aInfo->Author        = L"unxed, elfmz, FAR People";
  aInfo->UseMenuGuids  = 0;
}
