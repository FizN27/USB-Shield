// Ensure all generic function expands to W (Unicode) version of the function
// eg. CM_Get_Device_Interface_Property --> CM_Get_Device_Interface_PropertyW
#ifndef UNICODE
#define UNICODE
#endif // !UNICODE

#include <Windows.h>
#include <Dbt.h> // For WM_DEVICECHANGE event consts (DBT_*)
#include <winioctl.h> // For GUID consts (GUID_DEVINTERFACE_*)
#include <initguid.h> // Needed to be defined before defining devpkey.h for it to work properly
#include <devpkey.h> // To be used to get DEVINST device instance id
#include <cfgmgr32.h> // For CM_Get_Device_Interface_PropertyW to get instance id
#include <strsafe.h>
#include <winreg.h> // For registry manipulation functions

#include <cassert>
#include <string>
#include <vector>
#include <Lmcons.h>
#include <locale>
#include <codecvt>

#include <chrono> // To get current time
#include <ctime> // To get current time

#include <fstream> // To write into file
#include <sstream> // To concatenate string easily

#include <wincrypt.h> // For Base64 functions

#include <WtsApi32.h>

#include "resource2.h"

// Usage example: filePutContents("./yourfile.txt", "content", true);
void filePutContents(const std::string& name, const std::string& content, bool append = false) {
	std::ofstream outfile;
	if (append)
		outfile.open(name, std::ios_base::app);
	else
		outfile.open(name);
	outfile << content;
}
// Vector to hold dialog box instances
std::vector<HWND> g_dialogBoxes;

std::string Base64Decode(const std::string& encoded)
{
	DWORD dwDecodedSize = 0;
	CryptStringToBinaryA(encoded.c_str(), static_cast<DWORD>(encoded.length()), CRYPT_STRING_BASE64, NULL, &dwDecodedSize, NULL, NULL);

	std::vector<BYTE> buffer(dwDecodedSize);
	CryptStringToBinaryA(encoded.c_str(), static_cast<DWORD>(encoded.length()), CRYPT_STRING_BASE64, buffer.data(), &dwDecodedSize, NULL, NULL);

	return std::string(buffer.begin(), buffer.end());
}

// source: https://stackoverflow.com/questions/13563143/get-device-instance-dword-from-device-instance-path-string
std::wstring map_path(LPCWSTR device_path) {
	ULONG buf_size = 0;
	DEVPROPTYPE type;
	// call CM_Get_Device_Interface_PropertyW with buffer set to nullptr to get buffer size and store it in buf_size
	CM_Get_Device_Interface_PropertyW(
		device_path, &DEVPKEY_Device_InstanceId, &type,
		nullptr, &buf_size, 0);

	// create a buffer with len buf_size that we got from the first call of CM_Get_Device_Interface_PropertyW
	std::vector<BYTE> buffer(buf_size);

	// call CM_Get_Device_Interface_PropertyW a second time to actually populate buffer with instance id
	auto result = CM_Get_Device_Interface_PropertyW(
		device_path, &DEVPKEY_Device_InstanceId, &type,
		buffer.data(), &buf_size, 0);

	// make sure CM_Get_Device_Interface_PropertyW returns CR_SUCCESS and porperty is of DEVPROP_STRING type
	assert(result == CR_SUCCESS);
	assert(type == DEVPROP_TYPE_STRING);

	// buffer will be null-terminated
	return reinterpret_cast<wchar_t*>(buffer.data());
}

std::wstring get_deviceName(DEVINST instanceId) {
	ULONG buf_size = 0;
	DEVPROPTYPE type;
	// call CM_Get_Device_Interface_PropertyW with buffer set to nullptr to get buffer size and store it in buf_size
	CM_Get_DevNode_Property(instanceId, &DEVPKEY_Device_FriendlyName, &type,
		nullptr, &buf_size, 0);

	// create a buffer with len buf_size that we got from the first call of CM_Get_Device_Interface_PropertyW
	std::vector<BYTE> buffer(buf_size);

	// call CM_Get_Device_Interface_PropertyW a second time to actually populate buffer with instance id
	auto result = CM_Get_DevNode_Property(instanceId, &DEVPKEY_Device_FriendlyName, &type,
		buffer.data(), &buf_size, 0);

	// make sure CM_Get_Device_Interface_PropertyW returns CR_SUCCESS and porperty is of DEVPROP_STRING type
	assert(result == CR_SUCCESS);
	assert(type == DEVPROP_TYPE_STRING);

	// buffer will be null-terminated
	return reinterpret_cast<wchar_t*>(buffer.data());
}

BOOL DoRegisterDeviceNotification(HWND hWnd, HDEVNOTIFY* hDevNotify) {
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
	NotificationFilter.dbcc_size = sizeof(NotificationFilter);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = GUID_DEVINTERFACE_DISK;

	*hDevNotify = RegisterDeviceNotification(hWnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);

	if (*hDevNotify == NULL) {
		//MessageBox(hWnd, L"Register Device Notification Failed", L"Test", MB_OK);
		return FALSE;
	}

	return TRUE;
}

BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			SendMessage(hwndDlg, WM_CLOSE, 0, 0);
		}
		return TRUE;

	case WM_CLOSE:
		// Find the dialog box handle in the container and remove it
		auto tmp = std::find(g_dialogBoxes.begin(), g_dialogBoxes.end(), hwndDlg);
		if (tmp != g_dialogBoxes.end())
			g_dialogBoxes.erase(tmp);

		DestroyWindow(hwndDlg);
		return TRUE;
	}

	return FALSE;
}

VOID EjectDevice(DEVINST instanceId) {
	DEVINST parentInstanceId = 0;
	auto result = CM_Get_Parent(&parentInstanceId, instanceId, 0);

	PNP_VETO_TYPE VetoType = PNP_VetoTypeUnknown;
	WCHAR VetoName[MAX_PATH];

	for (int i = 0; i < 3; i++) {
		VetoName[0] = 0;
		result = CM_Request_Device_Eject(parentInstanceId, &VetoType, VetoName, MAX_PATH, 0);

		if (result == CR_SUCCESS && VetoType == PNP_VetoTypeUnknown) {
			std::wstring deviceFriendlyName = get_deviceName(instanceId);
			break;
		}
		Sleep(350);
	}
	return;
}

void HandleDeviceChange(HWND hWnd, WPARAM wParam, LPARAM lParam) {
	PDEV_BROADCAST_HDR pdbHdr = (PDEV_BROADCAST_HDR)lParam;

	switch (wParam) {
	case DBT_DEVICEARRIVAL:
		{
			// setup UNICODE to ANSI converter
			using convert_type = std::codecvt_utf8<wchar_t>;
			std::wstring_convert<convert_type, wchar_t> converter;

			// Manage USB
			if(pdbHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
				std::string usbStatus;
				std::wstring usbWhitelisted;
				PDEV_BROADCAST_DEVICEINTERFACE dbDi = (PDEV_BROADCAST_DEVICEINTERFACE)pdbHdr;
				LPCWSTR device_path = dbDi->dbcc_name;

				std::wstring pre_instanceId = map_path(device_path);

				DEVINST instanceId;

				auto result = CM_Locate_DevNode(&instanceId, (DEVINSTID)pre_instanceId.c_str(), CM_LOCATE_DEVNODE_NORMAL);

				//MessageBox(NULL, pre_instanceId.c_str(), L"TEST", MB_OK);

				HKEY hBlacklistKey;
				LONG lBlacklistkeyResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\USBShield\\Blacklist", 0, KEY_ALL_ACCESS, &hBlacklistKey);
				HKEY hWhitelistKey;
				LONG lWhitelistkeyResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\USBShield\\Whitelist", 0, KEY_ALL_ACCESS, &hWhitelistKey);

				std::string friendlyName;
				std::string emailSubject;

				if (lWhitelistkeyResult == ERROR_SUCCESS && lBlacklistkeyResult == ERROR_SUCCESS) {
					// Check whether instance id is in whitelist. If not in whitelist, eject the device and add it into blacklist.
					LONG lwhiteValResult = RegQueryValueEx(hWhitelistKey, pre_instanceId.c_str(), NULL, NULL, NULL, NULL);
					
					if (lwhiteValResult != ERROR_SUCCESS) {
						EjectDevice(instanceId);
						LONG lblackValResult = RegQueryValueEx(hBlacklistKey, pre_instanceId.c_str(), NULL, NULL, NULL, NULL);

						// If value is not in blacklist, add value into blacklist.
						if (lblackValResult != ERROR_SUCCESS) {
							LONG lsetResult = RegSetValueEx(hBlacklistKey, pre_instanceId.c_str(), 0, REG_SZ, NULL, 0);
							emailSubject = "New USB Insertion Report";
							usbStatus = "new";
						}
						else {
							emailSubject = "Blacklisted USB Insertion Report";
							usbStatus = "blacklisted";
						}

						DWORD bufferSize = 0;

						std::wstring iIDstring(pre_instanceId);
						std::string instanceIdA = converter.to_bytes(pre_instanceId);

						RegQueryValueExA(hBlacklistKey, instanceIdA.c_str(), NULL, NULL, NULL, &bufferSize);
						std::vector<char> buffer(bufferSize);
						RegQueryValueExA(hBlacklistKey, instanceIdA.c_str(), NULL, NULL, (LPBYTE)buffer.data(), &bufferSize);
						friendlyName.assign(buffer.begin(), buffer.end());
						friendlyName.erase(std::find(friendlyName.begin(), friendlyName.end(), '\0'), friendlyName.end());

						//NEW Notification
						HWND hDialogBox = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), hWnd, (DLGPROC)DialogProc);
						if (hDialogBox != NULL) {
							ShowWindow(hDialogBox, SW_SHOW);
						}
						g_dialogBoxes.push_back(hDialogBox);

						//OLD Notification
						//MessageBox(NULL, L"USB is not in whitelist, ejected", L"Warning", MB_OK);
					}
					else {
						usbStatus = "whitelisted";
						emailSubject = "Whitelisted USB Insertion Report";
						DWORD bufferSize = 0;

						std::wstring iIDstring(pre_instanceId);
						std::string instanceIdA = converter.to_bytes(pre_instanceId);

						RegQueryValueExA(hWhitelistKey, instanceIdA.c_str(), NULL, NULL, NULL, &bufferSize);
						std::vector<char> buffer(bufferSize);
						RegQueryValueExA(hWhitelistKey, instanceIdA.c_str(), NULL, NULL, (LPBYTE)buffer.data(), &bufferSize);
						friendlyName.assign(buffer.begin(), buffer.end());
						friendlyName.erase(std::find(friendlyName.begin(), friendlyName.end(), '\0'), friendlyName.end());
					}
					if (friendlyName == "") {
						friendlyName = "Unnamed USB";
					}
				}
				LONG lcloseWhiteResult = RegCloseKey(hWhitelistKey);
				LONG lcloseBlackResult = RegCloseKey(hBlacklistKey);

				// Get device information

				// Username
				DWORD sessionID = WTSGetActiveConsoleSessionId();
				wchar_t *pUserName = nullptr;
				DWORD userNameLen = 0;
				WTSQuerySessionInformationW(WTS_CURRENT_SERVER, sessionID, WTSUserName, &pUserName, &userNameLen);
				std::wstring loggedInUser(pUserName);


				/*TCHAR  szProgUser[128] = { 0 };
				LPTSTR pszLogonUser = nullptr;
				DWORD dwSessionId = -1, dwLen = 0, dwcch = ARRAYSIZE(szProgUser);

				if (ProcessIdToSessionId(GetCurrentProcessId(), &dwSessionId))
				{
					WTSQuerySessionInformation(WTS_CURRENT_SERVER, dwSessionId, WTSUserName, &pszLogonUser, &dwLen);
				}*/

				// Convert username to ANSI
				std::wstring userstring(loggedInUser);
				std::string usernameA = converter.to_bytes(loggedInUser);

				//wchar_t usernameW[UNLEN + 1];
				//DWORD username_len = UNLEN + 1;
				//GetUserName(usernameW, &username_len);
				

				// Hostname
				wchar_t hostnameW[MAX_COMPUTERNAME_LENGTH + 1];
				DWORD hostname_len = MAX_COMPUTERNAME_LENGTH + 1;
				GetComputerName(hostnameW, &hostname_len);
				// Convert hostname to ANSI
				std::wstring hoststring(hostnameW);
				std::string hostnameA = converter.to_bytes(hostnameW);

				// Current time
				auto curTime = std::chrono::system_clock::now();

				std::time_t currentTime = std::chrono::system_clock::to_time_t(curTime);
				char tmBuff[30];
				ctime_s(tmBuff, sizeof(tmBuff), &currentTime);

				// Get email data
				HKEY hSettingsKey;
				LONG lSettingskeyResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\USBShield", 0, KEY_ALL_ACCESS, &hSettingsKey);
				std::string dest_addrA, adm_emailA, adm_passwordA, smtp_serverA;

				if (lSettingskeyResult == ERROR_SUCCESS) {
					DWORD dwDataSize = 0;
					if (RegQueryValueExA(hSettingsKey, "dest_addr", NULL, NULL, NULL, &dwDataSize) == ERROR_SUCCESS) {
						std::vector<char> buffer(dwDataSize);
						if (RegQueryValueExA(hSettingsKey, "dest_addr", NULL, NULL, (LPBYTE)buffer.data(), &dwDataSize) == ERROR_SUCCESS)
						{
							dest_addrA.assign(buffer.begin(), buffer.end());
							dest_addrA.erase(std::find(dest_addrA.begin(), dest_addrA.end(), '\0'), dest_addrA.end());
						}
					}
					if (RegQueryValueExA(hSettingsKey, "adm_email", NULL, NULL, NULL, &dwDataSize) == ERROR_SUCCESS) {
						std::vector<char> buffer(dwDataSize);
						if (RegQueryValueExA(hSettingsKey, "adm_email", NULL, NULL, (LPBYTE)buffer.data(), &dwDataSize) == ERROR_SUCCESS)
						{
							adm_emailA.assign(buffer.begin(), buffer.end());
							adm_emailA.erase(std::find(adm_emailA.begin(), adm_emailA.end(), '\0'), adm_emailA.end());
						}
					}
					if (RegQueryValueExA(hSettingsKey, "adm_password", NULL, NULL, NULL, &dwDataSize) == ERROR_SUCCESS) {
						std::vector<char> buffer(dwDataSize);
						if (RegQueryValueExA(hSettingsKey, "adm_password", NULL, NULL, (LPBYTE)buffer.data(), &dwDataSize) == ERROR_SUCCESS)
						{
							adm_passwordA.assign(buffer.begin(), buffer.end());
							adm_passwordA.erase(std::find(adm_passwordA.begin(), adm_passwordA.end(), '\0'), adm_passwordA.end());
							adm_passwordA = Base64Decode(adm_passwordA);
						}
					}
					if (RegQueryValueExA(hSettingsKey, "smtp_server", NULL, NULL, NULL, &dwDataSize) == ERROR_SUCCESS) {
						std::vector<char> buffer(dwDataSize);
						if (RegQueryValueExA(hSettingsKey, "smtp_server", NULL, NULL, (LPBYTE)buffer.data(), &dwDataSize) == ERROR_SUCCESS)
						{
							smtp_serverA.assign(buffer.begin(), buffer.end());
							smtp_serverA.erase(std::find(smtp_serverA.begin(), smtp_serverA.end(), '\0'), smtp_serverA.end());
						}
					}
				}

				CHAR tmpPathBuffer[MAX_PATH];

				DWORD length = GetTempPathA(MAX_PATH, tmpPathBuffer);

				std::stringstream tmpName;
				tmpName << tmpPathBuffer << "tmp.txt";
				std::string fileName = tmpName.str();
				std::wstring wstmp(fileName.begin(), fileName.end());

				std::ofstream file(fileName);

				std::string instanceidA = converter.to_bytes(pre_instanceId);
				file << "Subject: " << emailSubject << "\n\n\nHostname: " << hostnameA << "\nEvent time: " << tmBuff << "Logged in user: " << usernameA << "\nUSB device status: " << usbStatus << "\nUSB Instance ID: " << instanceidA << "\nUSB friendly name: " << friendlyName;
				file.close();
				std::stringstream ss;
				ss << "-n --ssl-reqd --mail-from \"" << adm_emailA << "\" --mail-rcpt \"" << dest_addrA << "\" --url " << smtp_serverA << " -T \"" << fileName << "\" -u " << adm_emailA << ":" << adm_passwordA;
				std::string s = ss.str();
				std::wstring ws(s.begin(), s.end());

				STARTUPINFOW si;
				PROCESS_INFORMATION pi;

				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				ZeroMemory(&pi, sizeof(pi));

				//std::stringstream logName;
				//logName << "C:\\Program Files\\USB Shield\\log.txt";
				//std::string logfileName = logName.str();
				//std::wstring wslog(logfileName.begin(), logfileName.end());

				std::string timeString = tmBuff;
				timeString.erase(std::find(timeString.begin(), timeString.end(), '\n'), timeString.end());

				std::stringstream logText;
				logText << "[" << timeString << "] " << usernameA << " inserted a " << usbStatus << " USB " << instanceidA << "(" << friendlyName << ")\n";
				std::string logtextString = logText.str();
				std::wstring wslogtext(logtextString.begin(), logtextString.end());

				filePutContents("C:\\Program Files\\USB Shield\\log.txt", logtextString, true);

				//std::ofstream logfile(logfileName);
				
				//logfile << "[" << timeString << "] " << usernameA << " inserted a " << usbStatus << " USB " << instanceidA << "(" << friendlyName << ")";
				//logfile.close();

				if (CreateProcessW(L"C:\\Windows\\System32\\curl.exe", (LPWSTR)ws.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
				{
					WaitForSingleObject(pi.hProcess, INFINITE);
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
				}
				remove(fileName.c_str());
			}
		}
	}
}

// Windows Process prototype -> made here so we can reference it when creating window class in wWinMain
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) {

	// Set window class property
	const wchar_t CLASS_NAME[] = L"Main Class";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	// Register window class with OS
	RegisterClass(&wc);

	// Actually create the window
	HWND hWnd = CreateWindowEx(
		0,						// Optional window style
		CLASS_NAME,				// Window class -> set to the one we created before
		L"Test Window",			// Window Text -> for main window will show in title bar
		WS_OVERLAPPEDWINDOW,	// Mandatory window style

		// Window position
		//    X				 Y
		CW_USEDEFAULT, CW_USEDEFAULT,

		// Window size
		//  width		  height
		CW_USEDEFAULT, CW_USEDEFAULT,

		NULL,					// Parent handle -> because this is a parent window, set it to NULL. Otherwise, set it to handle of parent window.
		NULL,					// Window menu (not sure what this is)
		hInstance,				// Set instance to be used as window to instance of current wWinMain (not sure what this is)
		NULL					// Additional application data (used to do something during WM_CREATE event?)
	);

	// If window creation failed, exit program
	if (hWnd == NULL) {
		return 0;
	}

	// Function to actually show a window. If commented / removed, will not show window.
	// ShowWindow(hWnd, nCmdShow);

	// Run message loop

	MSG msg = {};

	while (GetMessage(&msg, NULL, 0, 0) != 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HDEVNOTIFY hDeviceNotify;
	static HWND hMsgWnd;
	switch (uMsg) {
	case WM_CREATE:
		{
			if (!DoRegisterDeviceNotification(hWnd, &hDeviceNotify)) {
				//DestroyWindow(hWnd);
			}
			break;
		}
	//case WM_CLOSE:
	//	{
	//		if (MessageBox(hWnd, L"Close?", L"Test", MB_OKCANCEL) == IDOK) {
	//			DestroyWindow(hWnd);
	//		}
	//		return 0;
	//	}
	case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);

			FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW-2));
			EndPaint(hWnd, &ps);
			break;
		}
	case WM_DEVICECHANGE:
		{
			HandleDeviceChange(hWnd, wParam, lParam);
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}