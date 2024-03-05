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
 * MultiFilterDriverExe.exe Minifilter AddExe CHROME.EXE					# 拦截启动 CHROME.EXE
 * MultiFilterDriverExe.exe Minifilter AddExeList							# 拦截启动 一次性多个
 * MultiFilterDriverExe.exe Minifilter RemoveRule							# 恢复正常
 *
 * MultiFilterDriverExe.exe WFP RemotePort 433								# 拦截目标端口433的请求
 * MultiFilterDriverExe.exe WFP RemotePort 80								# 拦截目标端口 80的请求
 * MultiFilterDriverExe.exe WFP RemoveRule									# 恢复正常
 * MultiFilterDriverExe.exe WFP AddressListAdd								# 拦截URL 一次性多个
 * MultiFilterDriverExe.exe WFP AddressAdd bilibili.com						# 拦截URL bilibili
 * MultiFilterDriverExe.exe WFP RemoteIp 192.168.1.4						# 拦截目标IP为 192.168.1.4请求
 *
 * MultiFilterDriverExe.exe Control CoverProcess [ProcessId]				# 隐藏进程 通过PID
 * MultiFilterDriverExe.exe Control HideWindow [ClassName]					# 隐藏窗口 通过ClassName 阻挡录屏和截图
 * MultiFilterDriverExe.exe Control HideWindowByPid [ProcessId]					# 隐藏窗口 通过ClassName 阻挡录屏和截图
 * MultiFilterDriverExe.exe Control HideFile \??\C:\HOME\CodeTest\1.exe		# 隐藏文件
 * MultiFilterDriverExe.exe Control HideFileRecovery						# 恢复正常
 * MultiFilterDriverExe.exe Control HideDir \??\C:\HOME\CodeTest			# 隐藏文件夹
 * MultiFilterDriverExe.exe Control HideDirRecovery							# 恢复正常
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