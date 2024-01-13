#include <windows.h>
#include <iostream>

void readToDevice(HANDLE hDevice) {
	// 这里可以在设备上执行读写操作
	int stringSize = 10;

	// 使用动态内存分配
	char* buffer = (char*)malloc(stringSize * sizeof(char));

	// 获取动态分配数组的大小
	size_t bufferSize = stringSize * sizeof(char);
	DWORD bytesRead;

	// 读取数据
	if (ReadFile(hDevice, buffer, bufferSize, &bytesRead, NULL)) {
		buffer[stringSize - 1] = '\0';
		std::cout << "Read " << bytesRead << " bytes from the device." << std::endl;
		std::cout << "Read " << buffer << " from the device." << std::endl;

		// 处理读取到的数据，可以根据实际情况进行操作
	}
	else {
		std::cerr << "Failed to read from device. Error code: " << GetLastError() << std::endl;
	}
}
void writeToDevice(HANDLE hDevice) {
	// 使用动态内存分配
	char buffer[] = "Data to be written to the driver.";
	DWORD bytesWritten;
	// 写入数据
	if (WriteFile(hDevice, buffer, sizeof(buffer), &bytesWritten, NULL)) {
		std::cout << "Write " << bytesWritten << " bytes to the device." << std::endl;
		std::cout << "Write " << buffer << " to the device." << std::endl;

		// 处理读取到的数据，可以根据实际情况进行操作
	}
	else {
		std::cerr << "Failed to write to device. Error code: " << GetLastError() << std::endl;
	}
}

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


	//readToDevice(hDevice);
	writeToDevice(hDevice);






	// 关闭设备句柄
	CloseHandle(hDevice);

	return 0;
}