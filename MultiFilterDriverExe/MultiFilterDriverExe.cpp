#include "FileMiniFilter.h"
#include "Control.h"
#include "WfpControl.h"

/*
 * MultiFilterDriverExe.exe Minifilter
 *
 * MultiFilterDriverExe.exe WFP Port 433
 * MultiFilterDriverExe.exe WFP Port 80
 * MultiFilterDriverExe.exe WFP RemoveRule
 *
 * MultiFilterDriverExe.exe Control CoverProcess [ProcessId]
 *
 */

int main(int argc, CHAR* argv[]) {
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