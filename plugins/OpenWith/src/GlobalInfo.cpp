#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->MinFarVersion = MAKEFARVERSION2(2,9,0,0);
  aInfo->Version       = MAKEFARVERSION2(1,3,0,0);
  aInfo->SysID         = 0x93CDEF19;
  aInfo->Title         = L"OpenWith";
  aInfo->Description   = L"Provides a context menu to open the currently selected file(s)";
  aInfo->Author        = L"Ivan <spnethw@gmail.com>";
  aInfo->UseMenuGuids  = 0;
}
