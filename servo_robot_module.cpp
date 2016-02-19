#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <process.h>

#include "module.h"
#include "robot_module.h"

#include "error.h"

#include <SerialClass.h>
#include "SimpleIni.h"

#include "servo_robot_module.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

/* GLOBALS CONFIG */
#define IID "RCT.Servo_robot_module_v100"
const int COUNT_FUNCTIONS = 1;
#define BUILD_NUMBER 2


#define ADD_ROBOT_AXIS(AXIS_NAME, UPPER_VALUE, LOWER_VALUE) \
	robot_axis[axis_id] = new AxisData; \
	robot_axis[axis_id]->axis_index = axis_id + 1; \
	robot_axis[axis_id]->upper_value = UPPER_VALUE; \
	robot_axis[axis_id]->lower_value = LOWER_VALUE; \
	robot_axis[axis_id]->name = AXIS_NAME; \
	++axis_id;

#define ADD_2F_FUNCTION(name)                         \
  pt = new FunctionData::ParamTypes[2];               \
  pt[0] = FunctionData::ParamTypes::FLOAT;            \
  pt[1] = FunctionData::ParamTypes::FLOAT;            \
  robot_functions[function_id] =                      \
      new FunctionData(function_id + 1, 2, pt, name); \
  function_id++;

#define DEFINE_ALL_FUNCTIONS \
	ADD_2F_FUNCTION("move_servo"); /*number, angle*/

#define DEFINE_ALL_AXIS \
	/*ADD_ROBOT_AXIS("locked", 1, 0)\
	ADD_ROBOT_AXIS("straight", 2, 0)\
	ADD_ROBOT_AXIS("rotation", 2, 0)*/

/////////////////////////////////////////////////

inline int getIniValueInt(CSimpleIniA *ini, const char *section_name, const char *key_name) {
	const char *tmp = ini->GetValue(section_name, key_name, NULL);
	if (!tmp) {
		throw new Error(ConsoleColor(ConsoleColor::red),
            		"Not specified value for \"%s\" in section \"%s\"!\n",
            		 key_name, section_name);
	}
	return strtol(tmp, NULL, 0);
}
inline const char* getIniValueChar(CSimpleIniA *ini, const char *section_name, const char *key_name) {
	const char *result = ini->GetValue(section_name, key_name, NULL);
	if (!result) {
		throw new Error(ConsoleColor(ConsoleColor::red),
            		"Not specified value for \"%s\" in section \"%s\"!\n",
            		 key_name, section_name);
	}
	return result;
}

ServoRobotModule::ServoRobotModule() {
	mi = new ModuleInfo;
	mi->uid = IID;
	mi->mode = ModuleInfo::Modes::PROD;
	mi->version = BUILD_NUMBER;
	mi->digest = NULL;

	{
		robot_functions = new FunctionData *[COUNT_FUNCTIONS];
		system_value function_id = 0;
		FunctionData::ParamTypes *pt;
		DEFINE_ALL_FUNCTIONS;
	}
	/*{
		robot_axis = new AxisData*[COUNT_AXIS];
		system_value axis_id = 0;
		DEFINE_ALL_AXIS
	}*/
}

const struct ModuleInfo &ServoRobotModule::getModuleInfo() { return *mi; }

int ServoRobotModule::init() {

	WCHAR DllPath[MAX_PATH] = {0};
	GetModuleFileNameW((HINSTANCE)&__ImageBase, DllPath, _countof(DllPath));

	WCHAR *tmp = wcsrchr(DllPath, L'\\');
	WCHAR ConfigPath[MAX_PATH] = {0};
	
	size_t path_len = tmp - DllPath;

	wcsncpy(ConfigPath, DllPath, path_len);
	wcscat(ConfigPath, L"\\config.ini");

	CSimpleIniA ini;
	ini.SetMultiKey(true);
	// Считали файл

	Error *init_error = new Error;
	try{
		if (ini.LoadFile(ConfigPath) < 0) {
			throw new Error(ConsoleColor(ConsoleColor::red),
							"Can't load '%s' file!\n", ConfigPath);
		}
		// Считали Оси
		COUNT_AXIS = getIniValueInt(&ini, "main", "servo_count");

		// Считали по количеству осей граничные значения для серв
		std::vector<ServoLimits> servo_limits;
		std::vector<unsigned char> start_position;
		std::vector<unsigned char> safe_position;
		for (int i = 1; i < COUNT_AXIS+1; ++i)
		{
			Error *_error = new Error;
			std::string str("servo_");
			str += std::to_string(i);
			int max;
			int min;
			int start_pos;
			int safe_pos;
			try {
				
				try{
					min = getIniValueInt(&ini, str.c_str(), "min");
				} catch (Error *e){
					_error->append(e);
				}
				if (min < 0 || min > 180)
				{
					_error->append(new Error(ConsoleColor(ConsoleColor::red),
		                    		"Wrong min limit '%d', should be in borders [0,180]",
		                    		 min));
				}
				
				try{
					max = getIniValueInt(&ini, str.c_str(), "max");
				} catch (Error *e){
					_error->append(e);
				}

				if (max < 0 || max > 180)
				{
					_error->append(new Error(ConsoleColor(ConsoleColor::red),
		                    		"Wrong max limit '%d', should be in borders [0,180]",
		                    		 max));
				}
				if (max < min) {
					_error->append(new Error(ConsoleColor(ConsoleColor::red),
		                    		"min bigger than max"));
				}
		
				// Тепеь считываем начальные положения серв
				try{
					start_pos = getIniValueInt(&ini, "start_position", str.c_str());
				} catch (Error *e){
					_error->append(e);
				}
				// Сравниваем
				if (start_pos < min || start_pos > max)
				{
					_error->append(new Error(ConsoleColor(ConsoleColor::red),
		                    		"Start position is out of limits"));
				}

				// Тепеь считываем безопасное положение положения серв
				try{
					safe_pos = getIniValueInt(&ini, "safe_position", str.c_str());
				} catch (Error *e){
					_error->append(e);
				}
				// Сравниваем
				if (safe_pos < min || safe_pos > max)
				{
					_error->append(new Error(ConsoleColor(ConsoleColor::red),
		                    		"Safe position is out of limits"));
				}
				_error->checkSelf();
			} catch (Error *e){
				init_error->append(new Error(e, ConsoleColor(ConsoleColor::red), "%s errors:",
	                    str.c_str()));
				continue;
			}

			servo_limits.push_back(ServoLimits(i, min, max));
			start_position.push_back(start_pos);
			safe_position.push_back(safe_pos);
		}

		init_error->checkSelf();
	
		//считали количество роботов
		int robots_count = getIniValueInt(&ini, "main", "robots_count");
		
		// Теперь по количеству роботов читаем порты
		for (int i = 1; i <= robots_count; ++i) {
			std::string str("robot_port_");
			str += std::to_string(i);
			std::string port(getIniValueChar(&ini, "main", str.c_str()));

			ServoRobot *servo_robot 
				= new ServoRobot(port, COUNT_AXIS, servo_limits, 
								 start_position, safe_position);
			aviable_connections.push_back(servo_robot);
		}
	} catch(Error *e) {
		colorPrintf(ConsoleColor(ConsoleColor::red), "Error(s): %s",
                    e->emit().c_str());
		printf("%s\n", e->emit().c_str());
	    delete e;
		return 1;
	}
	return 0;
}

FunctionData** ServoRobotModule::getFunctions(unsigned int *count_functions) {
	(*count_functions) = COUNT_FUNCTIONS;
	return robot_functions;
}

AxisData** ServoRobotModule::getAxis(unsigned int *count_axis) {
	//(*count_axis) = COUNT_AXIS;
	//return robot_axis;
	(*count_axis) = 0;
	return NULL;
}

Robot* ServoRobotModule::robotRequire() {
	for (v_connections_i i = aviable_connections.begin(); i != aviable_connections.end(); ++i) {
		if ((*i)->isAvaliable()) {
			try {
				(*i)->connect();
			} catch (Error *e){
				colorPrintf(ConsoleColor(ConsoleColor::red), "Error: %s",
                    e->emit().c_str());
		        delete e;
		        return NULL;
			}
			return (*i);
		}
	}
	return NULL;
}

void ServoRobotModule::robotFree(Robot *robot) {
	ServoRobot *servo_robot = reinterpret_cast<ServoRobot*>(robot);

	for (v_connections_i i = aviable_connections.begin(); i != aviable_connections.end(); ++i) {
		if ((*i) == servo_robot) {
			servo_robot->disconnect();
			return;
		}
	}
}

void ServoRobot::prepare(colorPrintfRobot_t *colorPrintf_p,
                         colorPrintfRobotVA_t *colorPrintfVA_p) {
  this->colorPrintf_p = colorPrintfVA_p;
}

void ServoRobotModule::prepare(colorPrintfModule_t *colorPrintf_p,
                               colorPrintfModuleVA_t *colorPrintfVA_p) {
  this->colorPrintf_p = colorPrintfVA_p;
}

int ServoRobotModule::startProgram(int uniq_index) { return 0; }

int ServoRobotModule::endProgram(int uniq_index) { return 0; }

void *ServoRobotModule::writePC(unsigned int *buffer_length) {
  (*buffer_length) = 0;
  return NULL;
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
bool ServoRobot::isAvaliable(){
	return is_aviable;
};

void ServoRobot::disconnect(){
	setSafePosition(0xFF);
	is_aviable = true;
};

#define WRITE_POS_TO_SERVO(servo, position) \
	buffer[1] = servo; \
	buffer[2] = position[servo-1]; \
	if (!SP->WriteData(buffer, 3)) { \
		throw new Error(ConsoleColor(ConsoleColor::red), \
        		"Can't write '%d' to servo %d. Setup safe position failed!",  \
        		buffer[2], buffer[1]); \
	}

void ServoRobot::setStartPosition(){
	unsigned char buffer[3];
	buffer[0] = 0xFF;

	WRITE_POS_TO_SERVO(5, start_position);
	Sleep(25);
	WRITE_POS_TO_SERVO(3, start_position);
	Sleep(25);
	WRITE_POS_TO_SERVO(1, start_position);
	WRITE_POS_TO_SERVO(2, start_position);
	WRITE_POS_TO_SERVO(4, start_position);
	WRITE_POS_TO_SERVO(6, start_position);
};


void ServoRobot::setSafePosition(unsigned char command){
	unsigned char buffer[3];
	buffer[0] = command;

	for (unsigned char i = 0; i < safe_position.size(); ++i)
	{
		buffer[1] = i+1;
		buffer[2] = safe_position[i];
		if (!SP->WriteData(buffer, 3)) {
			throw new Error(ConsoleColor(ConsoleColor::red),
            		"Can't write '%d' to servo %d. Setup safe position failed!", 
            		buffer[2], buffer[1]);
		}
	}

};

void ServoRobot::connect(){
	printf("port %s\n",port.c_str());
	SP = new Serial((char *)port.c_str());
	Sleep(150);
	if (!SP->IsConnected()){
		throw new Error(ConsoleColor(ConsoleColor::red),
                    "Can't connect to robot");
	}

	try{
		setSafePosition(254); // only set safe values
		Sleep(150);
		setStartPosition();
	} catch(Error *e){
		throw e;
	}

	is_aviable = false;
};

FunctionResult *ServoRobot::executeFunction(CommandMode mode, system_value command_index,
                                void **args) {
	if (command_index != 1) {
		return NULL;
	}

	FunctionResult *fr = NULL;
	try {
		switch(command_index) {
			case 1:{
				unsigned char input1 =(unsigned char) *(variable_value *)args[0];
				unsigned char input2 =(unsigned char) *(variable_value *)args[1];
	
				if ((input1 > count_axis) || (input1 <= 0)) {
					throw new Error(ConsoleColor(ConsoleColor::red),
                    		"wrong axis number %d", input1);
				}

				int _min = servo_limits[input1-1]._min;
				int _max = servo_limits[input1-1]._max;
				if ((input2 > _max) || (input2 <= _min)) {
					throw new Error(ConsoleColor(ConsoleColor::red),
                    		"wrong axis value %d, should be in borders [%d, %d]",
                    		input1, _min, _max);
				}

				// Если прошел то разрешаем
				unsigned char buffer[3];
				buffer[0] = 0xFF;
				buffer[1] = input1;
				buffer[2] = input2;

				if (!SP->WriteData(buffer, 3)) {
					throw new Error(ConsoleColor(ConsoleColor::red),
                    		"Can't write '%d' to servo %d. Sending data failed!", input2, input1);
				}
				break;
			}
			default:{
				break;
			}
		}

		fr = new FunctionResult(FunctionResult::Types::VALUE, 0);
	} catch (Error *e){
		colorPrintf(ConsoleColor(ConsoleColor::red), "Error: %s",
                	e->emit().c_str());
	    fr = new FunctionResult(FunctionResult::Types::EXCEPTION, 0);
	    delete e;
	}

	return fr;
}

void ServoRobot::axisControl(system_value axis_index, variable_value value) {}

ServoRobot::~ServoRobot() {
	if (SP) {
		if (SP->IsConnected())
		{
			SP->~Serial();
		}
	}
}

void ServoRobot::colorPrintf(ConsoleColor colors, const char *mask, ...) {
  va_list args;
  va_start(args, mask);
  (*colorPrintf_p)(this, port.c_str(), colors, mask, args);
  va_end(args);
}

void ServoRobotModule::colorPrintf(ConsoleColor colors, const char *mask, ...) {
  va_list args;
  va_start(args, mask);
  (*colorPrintf_p)(this, colors, mask, args);
  va_end(args);
}


PREFIX_FUNC_DLL unsigned short getRobotModuleApiVersion() {
  return ROBOT_MODULE_API_VERSION;
};

PREFIX_FUNC_DLL RobotModule *getRobotModuleObject() {
  return new ServoRobotModule();
}