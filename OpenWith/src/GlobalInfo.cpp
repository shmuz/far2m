#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 1,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x93CDEF19;
  aInfo->Version       = Version;
  aInfo->Title         = L"OpenWith";
  aInfo->Description   = L"Provides a context-aware menu to open the currently selected file";
  aInfo->Author        = L"spnethw, FAR People";
  aInfo->UseMenuGuids  = 0;
}
