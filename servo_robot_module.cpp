
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <stdint.h>
#include <unistd.h>
#include <cstdarg>
#include <cstddef>
#include <fcntl.h>
#include <dlfcn.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "module.h"
#include "robot_module.h"

#include "error.h"

#include "SerialClass.h"
#include "SimpleIni.h"

#include "servo_robot_module.h"

#ifdef _WIN32
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#endif
/* GLOBALS CONFIG */
#define IID "RCT.Servo_robot_module_v100"
#define ERROR_VALUE -1
#define ADDITIONAL_AXIS_COUNT 1  // add lock axis

#define ADD_ROBOT_AXIS(AXIS_NAME, UPPER_VALUE, LOWER_VALUE) \
  robot_axis[axis_id] = new AxisData;                       \
  robot_axis[axis_id]->axis_index = axis_id + 1;            \
  robot_axis[axis_id]->upper_value = UPPER_VALUE;           \
  robot_axis[axis_id]->lower_value = LOWER_VALUE;           \
  robot_axis[axis_id]->name = AXIS_NAME;                    \
  ++axis_id;
/////////////////////////////////////////////////

inline int getIniValueInt(CSimpleIniA *ini, const char *section_name,
                          const char *key_name) {
  int tmp = ini->GetLongValue(section_name, key_name, ERROR_VALUE);
  if (tmp == ERROR_VALUE) {
    throw new Error(ConsoleColor(ConsoleColor::red),
                    "Not specified value for \"%s\" in section \"%s\"!\n",
                    key_name, section_name);
  }
  return tmp;
}
inline const char *getIniValueChar(CSimpleIniA *ini, const char *section_name,
                                   const char *key_name) {
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

  is_prepare_failed = false;

  {
    robot_functions = new FunctionData *[count_functions];
    system_value function_id = 0;
    FunctionData::ParamTypes *pt;

    pt = new FunctionData::ParamTypes[2];
    pt[0] = FunctionData::ParamTypes::FLOAT;
    pt[1] = FunctionData::ParamTypes::FLOAT;
    robot_functions[function_id] =
        new FunctionData(function_id + 1, 2, pt, "move_servo");
    function_id++;
  }
}

const struct ModuleInfo &ServoRobotModule::getModuleInfo() { return *mi; }

int ServoRobotModule::init() {
  if (is_prepare_failed) {
    return 1;
  }
  return 0;
}

FunctionData **ServoRobotModule::getFunctions(unsigned int *_count_functions) {
  (*_count_functions) = this->count_functions;
  return robot_functions;
}

AxisData **ServoRobotModule::getAxis(unsigned int *count_axis) {
  (*count_axis) = this->count_axis;
  return robot_axis;
}

Robot *ServoRobotModule::robotRequire() {
  for (v_connections_i i = aviable_connections.begin();
       i != aviable_connections.end(); ++i) {
    if ((*i)->isAvaliable()) {
      try {
        (*i)->connect();
      } catch (Error *e) {
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
  ServoRobot *servo_robot = reinterpret_cast<ServoRobot *>(robot);

  for (v_connections_i i = aviable_connections.begin();
       i != aviable_connections.end(); ++i) {
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

  std::string ConfigPath = "";
#ifdef _WIN32
  WCHAR DllPath[MAX_PATH] = {0};
  GetModuleFileNameW((HINSTANCE)&__ImageBase, DllPath, (DWORD)MAX_PATH);

  WCHAR *tmp = wcsrchr(DllPath, L'\\');
  WCHAR wConfigPath[MAX_PATH] = {0};

  size_t path_len = tmp - DllPath;

  wcsncpy(wConfigPath, DllPath, path_len);
  wcscat(wConfigPath, L"\\config.ini");

  char c_ConfigPath[MAX_PATH] = {0};
  wcstombs(c_ConfigPath, wConfigPath, sizeof(c_ConfigPath));
  ConfigPath.append(c_ConfigPath);
#else
  Dl_info PathToSharedObject;
  void *pointer = reinterpret_cast<void *>(getRobotModuleObject);
  dladdr(pointer, &PathToSharedObject);
  std::string dltemp(PathToSharedObject.dli_fname);

  int dlfound = dltemp.find_last_of("/");

  dltemp = dltemp.substr(0, dlfound);
  dltemp += "/config.ini";

  ConfigPath.assign(dltemp.c_str());
#endif

  CSimpleIniA ini;
  ini.SetMultiKey(true);
  // Считали файл

  Error *init_error = new Error;
  try {
    if (ini.LoadFile(ConfigPath.c_str()) < 0) {
      throw new Error(ConsoleColor(ConsoleColor::red),
                      "Can't load '%s' file!\n", ConfigPath.c_str());
    }
    // Считали Оси
    int servo_count = getIniValueInt(&ini, "main", "servo_count");
    count_axis = servo_count + ADDITIONAL_AXIS_COUNT;

    // Считали по количеству осей граничные значения для серв
    std::vector<ServoLimits> servo_limits;
    for (int i = 1; i < servo_count + 1; ++i) {
      Error *_error = new Error;
      std::string str("servo_");
      char buffer[32];
      sprintf(buffer, "%d", i);
      str += std::string(buffer);
      int max;
      int min;
      int start_pos;
      int safe_pos;
      try {
        try {
          min = getIniValueInt(&ini, str.c_str(), "min");
        } catch (Error *e) {
          _error->append(e);
        }
        if (min < 0 || min > 180) {
          _error->append(new Error(
              ConsoleColor(ConsoleColor::red),
              "Wrong min limit '%d', should be in borders [0,180]", min));
        }

        try {
          max = getIniValueInt(&ini, str.c_str(), "max");
        } catch (Error *e) {
          _error->append(e);
        }

        if (max < 0 || max > 180) {
          _error->append(new Error(
              ConsoleColor(ConsoleColor::red),
              "Wrong max limit '%d', should be in borders [0,180]", max));
        }
        if (max < min) {
          _error->append(new Error(ConsoleColor(ConsoleColor::red),
                                   "min bigger than max"));
        }

        // Тепеь считываем начальные положения серв
        try {
          start_pos = getIniValueInt(&ini, "start_position", str.c_str());
        } catch (Error *e) {
          _error->append(e);
        }
        // Сравниваем
        if (start_pos < min || start_pos > max) {
          _error->append(new Error(ConsoleColor(ConsoleColor::red),
                                   "Start position is out of limits"));
        }

        // Тепеь считываем безопасное положение положения серв
        try {
          safe_pos = getIniValueInt(&ini, "safe_position", str.c_str());
        } catch (Error *e) {
          _error->append(e);
        }
        // Сравниваем
        if (safe_pos < min || safe_pos > max) {
          _error->append(new Error(ConsoleColor(ConsoleColor::red),
                                   "Safe position is out of limits"));
        }
        _error->checkSelf();
      } catch (Error *e) {
        init_error->append(new Error(e, ConsoleColor(ConsoleColor::red),
                                     "%s errors:", str.c_str()));
        continue;
      }

      axis_settings.push_back(AxisMinMax(min, max, str));
      servo_limits.push_back(ServoLimits(i, min, max, start_pos, safe_pos));
    }

    init_error->checkSelf();

    //считали количество роботов
    int robots_count = getIniValueInt(&ini, "main", "robots_count");

    // Теперь по количеству роботов читаем порты
    for (int i = 1; i <= robots_count; ++i) {
      std::string str("robot_port_");
      char buffer[32];
      sprintf(buffer, "%d", i);
      str += std::string(buffer);
      std::string port(getIniValueChar(&ini, "main", str.c_str()));

      ServoRobot *servo_robot = new ServoRobot(port, count_axis, servo_limits);
      aviable_connections.push_back(servo_robot);
    }

  } catch (Error *e) {
    colorPrintf(ConsoleColor(ConsoleColor::red), "Error(s): %s",
                e->emit().c_str());
    delete e;
    is_prepare_failed = true;
    count_axis = 0;
  }

  // ADD AXIS
  robot_axis = new AxisData *[count_axis];
  system_value axis_id = 0;

  for (int i = 0; i < axis_settings.size(); ++i) {
    ADD_ROBOT_AXIS(axis_settings[i].name.c_str(), axis_settings[i]._max,
                   axis_settings[i]._min);
  }
  ADD_ROBOT_AXIS("locked", 1, 0);
}

int ServoRobotModule::startProgram(int uniq_index) { return 0; }

int ServoRobotModule::endProgram(int uniq_index) { return 0; }

void *ServoRobotModule::writePC(unsigned int *buffer_length) {
  (*buffer_length) = 0;
  return NULL;
}

void ServoRobotModule::final() {
  for (v_connections_i i = aviable_connections.begin();
       i != aviable_connections.end(); ++i) {
    delete (*i);
  }
  aviable_connections.clear();
}

void ServoRobotModule::destroy() {
  for (int j = 0; j < count_functions; ++j) {
    delete robot_functions[j];
  }
  for (int j = 0; j < count_axis; ++j) {
    delete robot_axis[j];
  }
  delete[] robot_functions;

  delete[] robot_axis;
  delete this;
};

bool ServoRobot::isAvaliable() { return is_aviable; };

void ServoRobot::disconnect() {
  if (SP && SP->IsConnected()) {
    setSafePosition(Command::move_servo);
    SP->~Serial();
  }
  is_aviable = true;
};

void ServoRobot::setStartPosition() {
  unsigned char buffer[3];
  buffer[0] = Command::move_servo;

  for (unsigned char i = 0; i < servo_limits.size(); ++i) {
    buffer[1] = i + 1;
    buffer[2] = servo_limits[i].start_position;
    if (!SP->WriteData(buffer, 3)) {
      throw new Error(
          ConsoleColor(ConsoleColor::red),
          "Can't write '%d' to servo %d. Setup safe position failed!",
          buffer[2], buffer[1]);
    }
  }
};

void ServoRobot::setSafePosition(unsigned char command) {
  unsigned char buffer[3];
  buffer[0] = command;

  for (unsigned char i = 0; i < servo_limits.size(); ++i) {
    buffer[1] = i + 1;
    buffer[2] = servo_limits[i].safe_position;
    if (!SP->WriteData(buffer, 3)) {
      throw new Error(
          ConsoleColor(ConsoleColor::red),
          "Can't write '%d' to servo %d. Setup safe position failed!",
          buffer[2], buffer[1]);
    }
  }
};

void ServoRobot::connect() {
  if (SP) {
    SP->~Serial();
  }
  SP = new Serial((char *)port.c_str());
#ifdef _WIN32
  Sleep(150);
#else
  usleep(150);
#endif
  if (!SP->IsConnected()) {
    SP->~Serial();
    throw new Error(ConsoleColor(ConsoleColor::red), "Can't connect to robot");
  }

  try {
    setSafePosition(Command::write_value);  // only set safe values
#ifdef _WIN32
    Sleep(150);
#else
    usleep(150);
#endif
    setStartPosition();
  } catch (Error *e) {
    throw new Error(e, ConsoleColor(ConsoleColor::red), "Connection error:");
  }

  is_aviable = false;
};

FunctionResult *ServoRobot::executeFunction(CommandMode mode,
                                            system_value command_index,
                                            void **args) {
  if (command_index != 1) {
    return NULL;
  }

  FunctionResult *fr = NULL;
  try {
    switch (command_index) {
      case 1: {
        unsigned char input1 = (unsigned char)*(variable_value *)args[0];
        unsigned char input2 = (unsigned char)*(variable_value *)args[1];

        if (input1 > servo_limits.size()) {
          throw new Error(ConsoleColor(ConsoleColor::red),
                          "wrong axis number %d", input1);
        }

        int _min = servo_limits[input1 - 1]._min;
        int _max = servo_limits[input1 - 1]._max;
        if ((input2 > _max) || (input2 <= _min)) {
          throw new Error(ConsoleColor(ConsoleColor::red),
                          "wrong axis value %d, should be in borders [%d, %d]",
                          input1, _min, _max);
        }

        // Если прошел то разрешаем
        const int command_bytes = 3;
        unsigned char buffer[command_bytes];
        buffer[0] = Command::move_servo;
        buffer[1] = input1;
        buffer[2] = input2;

        if (!SP->WriteData(buffer, command_bytes)) {
          throw new Error(ConsoleColor(ConsoleColor::red),
                          "Can't write '%d' to servo %d. Sending data failed!",
                          input2, input1);
        }
        break;
      }
      default: { break; }
    }

    fr = new FunctionResult(FunctionResult::Types::VALUE, 0);
  } catch (Error *e) {
    colorPrintf(ConsoleColor(ConsoleColor::red), "Error: %s",
                e->emit().c_str());
    fr = new FunctionResult(FunctionResult::Types::EXCEPTION, 0);
    delete e;
  }

  return fr;
};

void ServoRobot::axisControl(system_value axis_index, variable_value value) {
  switch (axis_index) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6: {
      if (!is_locked) {
        const int command_bytes = 3;
        unsigned char buffer[command_bytes];
        buffer[0] = Command::move_servo;
        buffer[1] = axis_index;
        buffer[2] = value;
        SP->WriteData(buffer, command_bytes);
      }
      break;
    }
    case 7: {
      is_locked = !value;
    }
    default: { break; }
  }
  colorPrintf(ConsoleColor(ConsoleColor::green), "change axis value: %d = %f\n",
              axis_index, value);
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