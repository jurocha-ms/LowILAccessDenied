#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Networking.Sockets.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Storage.Streams.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <Windows.h>
#include <WinInet.h>

#pragma comment(lib, "WinInet.lib")

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Web::Http;

using winrt::hresult_error;
using std::string;
using std::wstring;

// Taken from https://github.com/ondradus/WinINet---Simple-GET-POST-request
// Succeeds even when debugging LowIL.
// Likely, the issue relies among VS Debugger - C++/WinRT/COM, not WinInet per se.
string HttpRequest(string site, string param)
{
	HINTERNET hInternet = InternetOpenW(L"YourUserAgent", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);

	if (hInternet == NULL)
	{
		return "InternetOpenW failed(hInternet): " + GetLastError();
	}
	else
	{
		wstring widestr;
		for (int i = 0; i < site.length(); ++i)
		{
			widestr += wchar_t(site[i]);
		}
		const wchar_t* site_name = widestr.c_str();

		wstring widestr2;
		for (int i = 0; i < param.length(); ++i)
		{
			widestr2 += wchar_t(param[i]);
		}
		const wchar_t* site_param = widestr2.c_str();

		// We need to convert str to const wchar_t as the args require!

		HINTERNET hConnect = InternetConnectW(hInternet, site_name, 80, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);

		if (hConnect == NULL)
		{
			return "InternetConnectW failed(hConnect == NULL): " + GetLastError();
		}
		else
		{
			const wchar_t* parrAcceptTypes[] = { L"text/*", NULL }; // accepted types. We'll choose text.

			HINTERNET hRequest = HttpOpenRequestW(hConnect, L"GET", site_param, NULL, NULL, parrAcceptTypes, 0, 0);

			if (hRequest == NULL)
			{
				return "HttpOpenRequestW failed(hRequest == NULL): " + GetLastError();
			}
			else
			{
				BOOL bRequestSent = HttpSendRequestW(hRequest, NULL, 0, NULL, 0);

				if (!bRequestSent)
				{
					return "!bRequestSent    HttpSendRequestW failed with error code " + GetLastError();
				}
				else
				{
					std::string strResponse;
					const int nBuffSize = 1024;
					char buff[nBuffSize];

					BOOL bKeepReading = true;
					DWORD dwBytesRead = -1;

					while (bKeepReading && dwBytesRead != 0)
					{
						bKeepReading = InternetReadFile(hRequest, buff, nBuffSize, &dwBytesRead);
						strResponse.append(buff, dwBytesRead);
					}
					return strResponse;
				}
				InternetCloseHandle(hRequest);
			}
			InternetCloseHandle(hConnect);
		}
		InternetCloseHandle(hInternet);
	}
}

// HTTP
IAsyncAction GetRequest()
{
	try
	{
		auto client = HttpClient{};
		auto response = co_await client.GetAsync(
			Uri{L"https://raw.githubusercontent.com/microsoft/react-native-windows/main/.yarnrc.yml"});
		auto body = co_await response.Content().ReadAsStringAsync();

		wprintf(L"\n[SUCCESS]\nHTTP STATUS [%d]\n\nHTTP CONTENT:\n%s\n\n", response.StatusCode(), body.c_str());
	}
	catch (const hresult_error& e)
	{
		wprintf(L"\n[FAILURE]\n[0x%x] %s\n", static_cast<unsigned int>(e.code()), e.message().c_str());
	}
}

// WebSockets
IAsyncAction SendReceive()
{
	using namespace winrt::Windows::Networking::Sockets;
	using winrt::Windows::Storage::Streams::DataWriter;
	using winrt::Windows::Storage::Streams::UnicodeEncoding;
	try
	{
		auto received = CreateEventA(nullptr, false, false, nullptr);

		auto socket = MessageWebSocket{};
		socket.Control().MessageType(SocketMessageType::Utf8);
		socket.MessageReceived([&received](IWebSocket const& sender, MessageWebSocketMessageReceivedEventArgs const& args)
			{
				auto reader = args.GetDataReader();
				reader.UnicodeEncoding(UnicodeEncoding::Utf8);

				auto message = reader.ReadString(reader.UnconsumedBufferLength());
				if (message != L"echo.websocket.events sponsored by Lob.com")
				{
					wprintf(L"\n[SUCCESS]\nWS MESSAGE:\n%s\n\n", message.c_str());

					winrt::check_bool(SetEvent(received));
				}
			});

		co_await socket.ConnectAsync(Uri{ L"wss://echo.websocket.events" });

		auto writer = DataWriter{ socket.OutputStream() };
		auto sent = writer.WriteString(L"ECHO ME");
		auto asyncSent = co_await writer.StoreAsync();

		co_await winrt::resume_on_signal(received);

		socket.Close();
	}
	catch (const hresult_error& e)
	{
		wprintf(L"\n[FAILURE]\n[0x%x] %s\n", static_cast<unsigned int>(e.code()), e.message().c_str());
	}
}

int main(int argc, char ** argv)
{
	winrt::init_apartment();

	printf("Please attach a debugger.\n");
	system("PAUSE");

	if (argc < 2 || string{ argv[1] } == "http")
	{
		GetRequest().get();
	}
	else if (string{ argv[1] } == "ws")
	{
		SendReceive().get();
	}

  // Successful case using raw WinInet.
  //auto response = HttpRequest("raw.githubusercontent.com", "microsoft/react-native-windows/main/.yarnrc.yml");
  //printf("[%s]\n", response.c_str());
}
