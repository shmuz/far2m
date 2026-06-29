#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 1,2,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x93CDEF19;
  aInfo->Version       = Version;
  aInfo->Title         = L"OpenWith";
  aInfo->Description   = L"Provides a context menu to open the currently selected file(s)";
  aInfo->Author        = L"Ivan <spnethw@gmail.com>";
  aInfo->UseMenuGuids  = 0;
}
