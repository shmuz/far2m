#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 3,0,0,802 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x4EBBEFC8;
  aInfo->Version       = Version;
  aInfo->Title         = L"LuaMacro";
  aInfo->Description   = L"Far macros in Lua";
  aInfo->Author        = L"Shmuel Zeigerman & Far Group";
}
//---------------------------------------------------------------------------

SHAREDSYMBOL int WINAPI GetMinFarVersionW(void)
{
  return MAKEFARVERSION(2,4);
}
//---------------------------------------------------------------------------
