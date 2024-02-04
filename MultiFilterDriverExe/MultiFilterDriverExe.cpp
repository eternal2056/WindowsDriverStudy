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
 * MultiFilterDriverExe.exe Minifilter AddExe CHROME.EXE
 * MultiFilterDriverExe.exe Minifilter AddExeList
 * MultiFilterDriverExe.exe Minifilter RemoveRule
 *
 * MultiFilterDriverExe.exe WFP RemotePort 433
 * MultiFilterDriverExe.exe WFP RemotePort 80
 * MultiFilterDriverExe.exe WFP RemoveRule
 * MultiFilterDriverExe.exe WFP AddressListAdd
 * MultiFilterDriverExe.exe WFP AddressAdd bilibili.com
 * MultiFilterDriverExe.exe WFP RemoteIp 192.168.1.4
 *
 * MultiFilterDriverExe.exe Control CoverProcess [ProcessId]
 *
 */

int main(int argc, CHAR* argv[]) {
	char* buffer = new char[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, buffer);
	std::cout << L"Current working directory: " << buffer << std::endl;
	std::string directoryPath = buffer;
	std::string WfpConfigPath = directoryPath + "\\WfpConfig.txt";
	std::string MiniFilterConfigPath = directoryPath + "\\MiniFilterConfig.txt";
	extern std::vector<std::string> forbidAddressListTemp;
	readAndPrintFile(WfpConfigPath, forbidAddressListTemp);
	extern std::vector<std::string> forbidExeListTemp;
	readAndPrintFile(MiniFilterConfigPath, forbidExeListTemp);
	//readAndPrintFile
	int processId;
	if (argc == 1) {
		std::cout << "Too few parameters." << std::endl;
		return 0;
	}
	std::string driverType = argv[1];
	if (driverType == "Minifilter") {
		FileMiniFilterMain(argc, argv);
	}
	if (driverType == "WFP") {
		WfpMain(argc, argv);
	}
	if (driverType == "Control") {
		ControlMain(argc, argv);
	}
}