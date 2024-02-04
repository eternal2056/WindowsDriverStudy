#include "FileMiniFilter.h"
#include "Control.h"
#include "WfpControl.h"
#include <string>
#include <fstream>


// 1. 配置文件
// 2. 注册表
// 3. 
void readAndPrintFile(const std::string& filePath, std::vector<std::string>& forbidAddressList) {
	// 打开文件
	std::ifstream inputFile(filePath);

	// 检查文件是否成功打开
	if (!inputFile.is_open()) {
		//readAndPrintFile("Error opening file: ", forbidAddressList);
		return;
	}

	// 逐行读取文件内容
	std::string line;
	while (std::getline(inputFile, line)) {
		// 处理每一行的数据
		forbidAddressList.push_back(line);
	}

	// 关闭文件
	inputFile.close();
}

/*
 * MultiFilterDriverExe.exe Minifilter
 *
 * MultiFilterDriverExe.exe WFP Port 433
 * MultiFilterDriverExe.exe WFP Port 80
 * MultiFilterDriverExe.exe WFP RemoveRule
 * MultiFilterDriverExe.exe WFP AddressListAdd
 *
 * MultiFilterDriverExe.exe Control CoverProcess [ProcessId]
 *
 */

int main(int argc, CHAR* argv[]) {
	char* buffer = new char[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, buffer);
	std::cout << L"Current working directory: " << buffer << std::endl;
	std::string bufferString = buffer;
	bufferString += "\\WfpConfig.txt";
	extern std::vector<std::string> forbidAddressListTemp;
	readAndPrintFile(bufferString, forbidAddressListTemp);
	//readAndPrintFile
	int processId;
	if (argc == 1) {
		std::cout << "Too few parameters." << std::endl;
		return 0;
	}
	std::string driverType = argv[1];
	if (driverType == "Minifilter") {
		FileMiniFilterMain();
	}
	if (driverType == "WFP") {
		WfpMain(argc, argv);
	}
	if (driverType == "Control") {
		ControlMain(argc, argv);
	}
}