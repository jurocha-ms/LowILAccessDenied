// Windows API
#include <Windows.h>
#include <sddl.h>

// Standard Library
#include <stdio.h>
#include <stdlib.h>
#include <string>

using std::string;
using std::wstring;

static constexpr const WCHAR *lowIlSidStr = L"S-1-16-4096";
static constexpr const WCHAR *privNetSidStr = L"S-1-15-3-3";

PROCESS_INFORMATION processInfoSandbox;
PSID lowIlPsid;
HRESULT CreateLowILProcess() noexcept
{
  wstring commandLine = L"SandboxTest.exe";
  wstring eventSyncName =
      L"60758A28-A98E-47F8-84D8-795A8D5A0C54"; // Random string

  BOOL fSetToken = FALSE;
  HANDLE hProcTokenLowIl;
  HANDLE hMICToken;
  BOOL fOpenProcToken = OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hProcTokenLowIl);
  {
    BOOL fConvertSid = TRUE;
    BOOL fDuplicateToken = DuplicateTokenEx(
      hProcTokenLowIl,
      MAXIMUM_ALLOWED,
      NULL,
      SecurityImpersonation,
      TokenImpersonation,
      &hMICToken);

    if (fDuplicateToken)
    {
      if (lowIlPsid == nullptr)
      {
        fConvertSid = ConvertStringSidToSid(lowIlSidStr, &lowIlPsid);
      }

      if (fConvertSid) {
        // https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/ne-ntifs-_token_information_class
        // Set Process IL to Low
        TOKEN_MANDATORY_LABEL TML = {0};
        TML.Label.Attributes = SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED;
        TML.Label.Sid = lowIlPsid;

        BOOL fPILToken =
            SetTokenInformation(hMICToken, TokenIntegrityLevel, &TML, sizeof(TML) + GetLengthSid(lowIlPsid));

        if (fPILToken)
        {
          fSetToken = SetThreadToken(nullptr, hMICToken);
        }
      } // if (fConvertSid)

      if (!fOpenProcToken || !fDuplicateToken || !fConvertSid || !fSetToken)
      {
        //TODO: FAIL
      }
    }
  } // Scope

  HANDLE hEventRemoter = ::CreateEvent(
      NULL /*lpEventAttributes*/, false /*bManualReset*/, false /*bInitialState*/, eventSyncName.c_str());
  //TODO: check

  STARTUPINFOEX si = {};
  si.StartupInfo.cb = sizeof(si);

  const int32_t iCountRetry = 5;
  bool fCreateProcess = false;
  fCreateProcess = CreateProcessAsUser(
        hMICToken,
        nullptr,
        (LPWSTR)commandLine.c_str(),
        nullptr /*lpProcessAttributes*/,
        nullptr /*lpThreadAttributes*/,
        false /*bInheritHandles*/,
        0 /*EXTENDED_STARTUP_INFO_PRESENT*/,
        NULL /*lpEnvironment*/,
        NULL /*lpCurrentDirectory*/, (LPSTARTUPINFO)&si, &processInfoSandbox);

  if (!fCreateProcess || processInfoSandbox.hProcess == NULL || processInfoSandbox.hThread == NULL)
  {
    auto err = HRESULT_FROM_WIN32(GetLastError());
  }

  auto dwWaitStatus = WaitForSingleObject(hEventRemoter, 0);

  return S_OK;
}

int main(int argc, char ** argv)
{
  auto ans = CreateLowILProcess();
}
