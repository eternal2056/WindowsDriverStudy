#include <windows.h>
#include <iostream>

int main() {
	// 替换成你的设备名称
	LPCWSTR deviceName = L"\\\\.\\MyCoverProcessLink";

	// 打开设备
	HANDLE hDevice = CreateFile(
		deviceName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hDevice == INVALID_HANDLE_VALUE) {
		std::cerr << "Failed to open device. Error code: " << GetLastError() << std::endl;
		return -1;
	}

	// 这里可以在设备上执行读写操作
	char buffer[1024];
	DWORD bytesRead;

	// 读取数据
	if (ReadFile(hDevice, buffer, sizeof(buffer), &bytesRead, NULL)) {
		std::cout << "Read " << bytesRead << " bytes from the device." << std::endl;

		// 处理读取到的数据，可以根据实际情况进行操作
	}
	else {
		std::cerr << "Failed to read from device. Error code: " << GetLastError() << std::endl;
	}

	// 关闭设备句柄
	CloseHandle(hDevice);

	return 0;
}