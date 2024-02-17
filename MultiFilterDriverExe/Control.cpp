#include "Control.h"

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

void getProcessNameTest(HANDLE hDevice) {

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
void getProcessName(HANDLE hDevice, int processId) {

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

	PROCESS_MY* processData = new PROCESS_MY();
	processData->ProcessId = processId;

	DWORD bytesReturned;
	char outBuffer[1024];
	// 发送IO控制码 IOCTL_MY_FUNCTION
	if (!DeviceIoControl(hDevice, IOCTL_KILLRULE_PROCESS, processData, sizeof(PROCESS_MY), outBuffer, sizeof(outBuffer), &bytesReturned, NULL)) {
		// 处理控制码发送失败的情况
	}

	std::cout << "Write " << sizeof(PROCESS_MY) << " bytes to the device." << std::endl;
}
void controlHideProcess(HANDLE hDevice, const char* className) {
	HWND window_handle = FindWindowA(className, 0);
	if (window_handle)
	{
		/*
		查找了网上,发现WDA_EXCLUDEFROMCAPTURE标识在一些新版Win10上才有效果(变透明)
		在旧版Win10上表现为黑色窗口,这是没有办法的事情
		*/
		MyMessage64 info{ 0 };
		info.window_attributes = WDA_EXCLUDEFROMCAPTURE;
		info.window_handle = (__int64)window_handle;
		DeviceIoControl(hDevice, HIDE_WINDOW, &info, sizeof(info), &info, sizeof(info), 0, 0);

		/*
		注意这里,就算上面设置了WDA_EXCLUDEFROMCAPTURE标识,但是这里也是返回0
		在一定作用上也能干扰下反作弊系统吧
		*/
		DWORD Style = 0;
		GetWindowDisplayAffinity(window_handle, &Style);
		printf("style is %d \n", Style);
	}
}


void SendIoctl_HideObjectPacket(HANDLE hDevice, const wchar_t* path, unsigned short type)
{
	PHidContextInternal context = (PHidContextInternal)malloc(sizeof(HidContextInternal));
	context->hdevice = hDevice;

	PHid_HideObjectPacket hide;
	Hid_StatusPacket result;
	size_t size, len, total;
	DWORD returned;

	len = wcslen(path);

	// Pack data to packet

	total = (len + 1) * sizeof(wchar_t);
	size = sizeof(Hid_HideObjectPacket) + total;
	hide = (PHid_HideObjectPacket)_alloca(size);
	hide->dataSize = (unsigned short)total;
	hide->objType = type;

	memcpy((char*)hide + sizeof(Hid_HideObjectPacket), path, total);

	// Send IOCTL to device
	std::cout << "Write " << sizeof(Hid_HideObjectPacket) << " bytes to the device." << std::endl;
	if (!DeviceIoControl(context->hdevice, HID_IOCTL_ADD_HIDDEN_OBJECT, hide, (DWORD)size, &result, sizeof(result), &returned, NULL)) {

	}
}

void SendIoctl_UnHideObjectPacket(HANDLE hDevice, unsigned short type)
{
	PHidContextInternal context = (PHidContextInternal)malloc(sizeof(HidContextInternal));
	context->hdevice = hDevice;

	Hid_StatusPacket result;
	size_t size, len, total;
	DWORD returned;
	Hid_UnhideAllObjectsPacket unhide;

	unhide.objType = type;

	// Send IOCTL to device
	std::cout << "Write " << sizeof(Hid_HideObjectPacket) << " bytes to the device." << std::endl;
	if (!DeviceIoControl(context->hdevice, HID_IOCTL_REMOVE_ALL_HIDDEN_OBJECTS, &unhide, sizeof(unhide), &result, sizeof(result), &returned, NULL)) {

	}
}

int ControlMain(int argc, CHAR* argv[]) {
	HANDLE hDevice = NULL;
	int processId;
	std::string param2 = argv[2];

	if (param2 == "CoverProcess") {
		// 打开设备
		hDevice = CreateFile(
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
		std::string param3 = argv[3];
		processId = std::stoi(param3);

		//readToDevice(hDevice);
		//writeToDevice(hDevice);
		//writeToDeviceStucture(hDevice);
		//getProcessNameTest(hDevice);
		getProcessName(hDevice, processId);
	}
	if (param2 == "HideWindow") {
		// 打开设备
		hDevice = CreateFile(
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
		std::string param3 = argv[3];

		//readToDevice(hDevice);
		//writeToDevice(hDevice);
		//writeToDeviceStucture(hDevice);
		//getProcessNameTest(hDevice);
		controlHideProcess(hDevice, param3.data());
	}
	if (param2 == "HideFileRecovery") {

		// 打开设备
		hDevice = CreateFile(
			MINIFILTER_USER_DEVICES_LINK_NAME,
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

		SendIoctl_UnHideObjectPacket(hDevice, FsFileObject);
	}
	if (param2 == "HideFile") {

		// 打开设备
		hDevice = CreateFile(
			MINIFILTER_USER_DEVICES_LINK_NAME,
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

		std::string path = argv[3];
		int wideStrLength = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, NULL, 0);
		wchar_t* wideStrBuffer = new wchar_t[wideStrLength];
		MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wideStrBuffer, wideStrLength);
		SendIoctl_HideObjectPacket(hDevice, wideStrBuffer, FsFileObject);
	}

	if (param2 == "HideDirRecovery") {

		// 打开设备
		hDevice = CreateFile(
			MINIFILTER_USER_DEVICES_LINK_NAME,
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

		SendIoctl_UnHideObjectPacket(hDevice, FsDirObject);
	}
	if (param2 == "HideDir") {

		// 打开设备
		hDevice = CreateFile(
			MINIFILTER_USER_DEVICES_LINK_NAME,
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

		std::string path = argv[3];
		int wideStrLength = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, NULL, 0);
		wchar_t* wideStrBuffer = new wchar_t[wideStrLength];
		MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wideStrBuffer, wideStrLength);
		SendIoctl_HideObjectPacket(hDevice, wideStrBuffer, FsDirObject);
	}

	// 关闭设备句柄
	CloseHandle(hDevice);

	return 0;
}