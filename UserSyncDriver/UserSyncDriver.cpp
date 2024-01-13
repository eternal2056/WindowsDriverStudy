#include "Header.h"

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



void writeToDeviceStucture(HANDLE hDevice) {
	PMY_DATA myData = new MY_DATA();
	myData->Value1 = 1000;
	myData->Value2 = 2000;
	myData->MyPoint = new MY_POINT();
	myData->MyPoint->Value1 = 3000;
	myData->MyPoint->Value2 = 4000;
	// 使用动态内存分配
	DWORD bytesWritten;
	// 写入数据
	if (WriteFile(hDevice, myData, sizeof(myData), &bytesWritten, NULL)) {
		std::cout << "Write " << bytesWritten << " bytes to the device." << std::endl;
	}
	else {
		std::cerr << "Failed to write to device. Error code: " << GetLastError() << std::endl;
	}
}

void getProcessName(HANDLE hDevice) {

	/*
	DeviceIoControl(
	_In_ HANDLE hDevice,
	_In_ DWORD dwIoControlCode,
	_In_reads_bytes_opt_(nInBufferSize) LPVOID lpInBuffer,
	_In_ DWORD nInBufferSize,
	_Out_writes_bytes_to_opt_(nOutBufferSize,*lpBytesReturned) LPVOID lpOutBuffer,
	_In_ DWORD nOutBufferSize,
	_Out_opt_ LPDWORD lpBytesReturned,
	_Inout_opt_ LPOVERLAPPED lpOverlapped
	);
	*/

	PMY_DATA myData = new MY_DATA();
	myData->Value1 = 1000;
	myData->Value2 = 2000;
	myData->MyPoint = new MY_POINT();
	myData->MyPoint->Value1 = 3000;
	myData->MyPoint->Value2 = 4000;

	DWORD bytesReturned;
	char outBuffer[1024];
	// 发送IO控制码 IOCTL_MY_FUNCTION
	if (!DeviceIoControl(hDevice, IOCTL_KILLRULE_PROCESS, myData, sizeof(MY_DATA), outBuffer, sizeof(outBuffer), &bytesReturned, NULL)) {
		// 处理控制码发送失败的情况
	}

	std::cout << "Write " << sizeof(myData) << " bytes to the device." << std::endl;
}

int main() {
	// 打开设备
	HANDLE hDevice = CreateFile(
		KILLRULE_USER_SYMBOLINK,
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
	//writeToDevice(hDevice);
	//writeToDeviceStucture(hDevice);
	getProcessName(hDevice);

	// 关闭设备句柄
	CloseHandle(hDevice);

	return 0;
}