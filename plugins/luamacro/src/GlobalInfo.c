#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->MinFarVersion = MAKEFARVERSION2(2,9,0,0);
  aInfo->Version       = MAKEFARVERSION2(3,0,0,802);
  aInfo->SysID         = 0x4EBBEFC8;
  aInfo->Title         = L"LuaMacro";
  aInfo->Description   = L"Far macros in Lua";
  aInfo->Author        = L"Shmuel Zeigerman & Far Group";
  aInfo->UseMenuGuids  = 1;
}
