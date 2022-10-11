#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Web.Http.h>
#include <stdio.h>
#include <stdlib.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Web::Http;

IAsyncAction GetRequest()
{
  try {
    auto client = HttpClient{};
    auto response = co_await client.GetAsync(
        Uri{L"https://raw.githubusercontent.com/microsoft/react-native-windows/main/.yarnrc.yml"});
    auto body = co_await response.Content().ReadAsStringAsync();

    wprintf(L"\n[SUCCESS]\nHTTP STATUS [%d]\n\nHTTP CONTENT:\n%s\n\n", response.StatusCode(), body.c_str());
  } catch (const winrt::hresult_error &e) {
    wprintf(L"\n[FAILURE]\n[0x%x] %s\n", static_cast<unsigned int>(e.code()), e.message().c_str());
  }
}

int main(int argc, char ** argv)
{
  winrt::init_apartment();

  GetRequest().get();

  system("pause");
}
