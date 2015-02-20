#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <process.h>

#include "../module_headers/module.h"
#include "../module_headers/robot_module.h"

#include "../../SerialClass/SerialClass.h"
#include "SimpleIni.h"

#include "servo_robot_module.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

/* GLOBALS CONFIG */
const int COUNT_FUNCTIONS = 1;

#define DEFINE_ALL_FUNCTIONS \
	ADD_ROBOT_FUNCTION("move_servo", 2, true)				/*number, angle*/

#define DEFINE_ALL_AXIS \
	/*ADD_ROBOT_AXIS("locked", 1, 0)\
	ADD_ROBOT_AXIS("straight", 2, 0)\
	ADD_ROBOT_AXIS("rotation", 2, 0)*/

/////////////////////////////////////////////////

inline int getIniValueInt(CSimpleIniA *ini, const char *section_name, const char *key_name) {
	const char *tmp = ini->GetValue(section_name, key_name, NULL);
	if (!tmp) {
		printf("Not specified value for \"%s\" in section \"%s\"!\n", key_name, section_name);
		throw;
	}
	return strtol(tmp, NULL, 0);
}
inline const char* getIniValueChar(CSimpleIniA *ini, const char *section_name, const char *key_name) {
	const char *result = ini->GetValue(section_name, key_name, NULL);
	if (!result) {
		printf("Not specified value for \"%s\" in section \"%s\"!\n", key_name, section_name);
		throw;
	}
	return result;
}

ServoRobotModule::ServoRobotModule() {
	{
		robot_functions = new FunctionData*[COUNT_FUNCTIONS];
		regval function_id = 0;
		DEFINE_ALL_FUNCTIONS
	}
	/*{
		robot_axis = new AxisData*[COUNT_AXIS];
		regval axis_id = 0;
		DEFINE_ALL_AXIS
	}*/

	WCHAR DllPath[MAX_PATH] = {0};
	GetModuleFileNameW((HINSTANCE)&__ImageBase, DllPath, _countof(DllPath));

	WCHAR *tmp = wcsrchr(DllPath, L'\\');
	WCHAR ConfigPath[MAX_PATH] = {0};
	
	size_t path_len = tmp - DllPath;

	wcsncpy(ConfigPath, DllPath, path_len);
	wcscat(ConfigPath, L"\\config.ini");

	ini.SetMultiKey(true);

	if (ini.LoadFile(ConfigPath) < 0) {
		printf("Can't load '%s' file!\n", ConfigPath);
		is_error_init = true;
		return;
	}

	try {
		COUNT_AXIS = getIniValueInt(&ini, "main", "count_servos");
	} catch(...) {
		is_error_init = true;
		return;
	}

	is_error_init = false;
}

const char *ServoRobotModule::getUID() {
	return "Servo robot module 1.00";
}

int ServoRobotModule::init() {
	if (is_error_init) {
		return 1;
	}

	try{
		int count_robots = getIniValueInt(&ini, "main", "count_robots");
	
		for (int i = 1; i <= count_robots; ++i) {
			std::string str("robot_port_");
			str += std::to_string(i);
			const char *port_name = getIniValueChar(&ini, "main", str.c_str());

			int len_p = 5 + strlen(port_name) + 1;
			char *p = new char[len_p]; 
			strcpy(p, "\\\\.\\");
			strcat(p, port_name);

			Serial* SP = new Serial(p);
			delete p;

			if (!SP->IsConnected()) {
				printf("Can't connected to the serial port %s!", port_name);
				continue;
			}

			ServoRobot *servo_robot = new ServoRobot(SP, COUNT_AXIS);
			printf("DLL: connected to %s robot %p\n", port_name, servo_robot);
			aviable_connections.push_back(servo_robot);

#ifdef _DEBUG
			servo_robot->startCheckingSerial();
#endif
		}
	} catch(...) {
		return 1;
	}

	return 0;
}

FunctionData** ServoRobotModule::getFunctions(int *count_functions) {
	(*count_functions) = COUNT_FUNCTIONS;
	return robot_functions;
}

AxisData** ServoRobotModule::getAxis(int *count_axis) {
	//(*count_axis) = COUNT_AXIS;
	//return robot_axis;
	(*count_axis) = 0;
	return NULL;
}

Robot* ServoRobotModule::robotRequire() {
	for (v_connections_i i = aviable_connections.begin(); i != aviable_connections.end(); ++i) {
		if ((*i)->is_aviable) {
			(*i)->is_aviable = false;
			return (*i);
		}
	}
	return NULL;
}

void ServoRobotModule::robotFree(Robot *robot) {
	ServoRobot *servo_robot = reinterpret_cast<ServoRobot*>(robot);

	for (v_connections_i i = aviable_connections.begin(); i != aviable_connections.end(); ++i) {
		if ((*i) == servo_robot) {
			servo_robot->is_aviable = true;
			return;
		}
	}
}

void ServoRobotModule::final() {
	for (v_connections_i i = aviable_connections.begin(); i != aviable_connections.end(); ++i) {
		delete (*i);
	}
	aviable_connections.clear();
}

void ServoRobotModule::destroy() {
	for (int j = 0; j < COUNT_FUNCTIONS; ++j) {
		delete robot_functions[j];
	}
	//for (int j = 0; j < COUNT_AXIS; ++j) {
	for (int j = 0; j < 0; ++j) {
		delete robot_axis[j];
	}
	delete[] robot_functions;
	//delete[] robot_axis;
	delete this;
}

FunctionResult* ServoRobot::executeFunction(regval command_index, regval *args) {
	if (command_index != 1) {
		return NULL;
	}

	if ((((*args)+1) > count_axis) || (((*args)+1) <= 0)) {
		return new FunctionResult(0);
	}
	if (((*args+1) > 180) || ((*args+1) <= 0)) {
		return new FunctionResult(0);
	}

	unsigned char *buffer = new unsigned char[3];
	buffer[0] = 0xFF;
	buffer[1] = *args;
	buffer[2] = *(args + 1);

#ifdef _DEBUG
	EnterCriticalSection(&cheking_mutex);
#endif
	bool have_error = !SP->WriteData(buffer, 3);
#ifdef _DEBUG
	LeaveCriticalSection(&cheking_mutex);
#endif

	delete[] buffer;

	if (have_error) {
		return new FunctionResult(0);
	}
	return new FunctionResult(1, 0);
}

void ServoRobot::axisControl(regval axis_index, regval value) {}

#ifdef _DEBUG
unsigned int WINAPI procCheckingSerial(void *arg) {
	ServoRobot *servo_robot = static_cast<ServoRobot*>(arg);
	servo_robot->checkSerial();
	return 0;
}

void ServoRobot::startCheckingSerial() {
	checkingSerial = true;
	InitializeCriticalSection(&cheking_mutex);
	thread_handle = (HANDLE) _beginthreadex(NULL, 0, procCheckingSerial, this, 0, &tid);
}

void ServoRobot::checkSerial() {
	const int dataLength = 256;
	char incomingData[dataLength] = "";
	int readResult = 0;

	EnterCriticalSection(&cheking_mutex);
	while(SP->IsConnected() && checkingSerial) {
		readResult = SP->ReadData(incomingData, dataLength);
		LeaveCriticalSection(&cheking_mutex);

		if (readResult != -1) {
			printf("Bytes read: %i\n", readResult);
			incomingData[readResult] = 0;
			printf("%s\n", incomingData);
		}
		Sleep(500);

		EnterCriticalSection(&cheking_mutex);
	}
	LeaveCriticalSection(&cheking_mutex);
}
#endif

ServoRobot::~ServoRobot() {
#ifdef _DEBUG
	EnterCriticalSection(&cheking_mutex);
	checkingSerial = false;
	LeaveCriticalSection(&cheking_mutex);

	WaitForSingleObject(thread_handle, INFINITE);
	CloseHandle(thread_handle);
	DeleteCriticalSection(&cheking_mutex);
#endif

	SP->~Serial();
}

__declspec(dllexport) RobotModule* getRobotModuleObject() {
	return new ServoRobotModule();
}
