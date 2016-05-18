#ifndef SERVO_ROBOT_MODULE_H
#define SERVO_ROBOT_MODULE_H

struct ServoLimits {
  int servo_number;
  int _min;
  int _max;
  int start_position;
  int safe_position;
  bool increment;
  int current_position; 
  ServoLimits(int number, int _min, int _max, int start_position,
              int safe_position, bool increment);
};

enum Command { write_value = 0xFE, move_servo = 0xFF };

class ServoRobot : public Robot {
 private:
  colorPrintfRobotVA_t *colorPrintf_p;
  Serial *SP;
  int count_axis;

  std::vector<ServoLimits> servo_data;
  std::string port;
  bool is_aviable;
  bool is_locked;

  void colorPrintf(ConsoleColor colors, const char *mask, ...);
  void setStartPosition();
  void setSafePosition(unsigned char command);
 public:
  ServoRobot(std::string port, int count_axis,
             std::vector<ServoLimits> servo_data)
      : colorPrintf_p(NULL),
        SP(NULL),
        count_axis(count_axis),
        servo_data(servo_data),
        port(port),
        is_aviable(true),
        is_locked(true) {}
  void prepare(colorPrintfRobot_t *colorPrintf_p,
               colorPrintfRobotVA_t *colorPrintfVA_p);
  FunctionResult *executeFunction(CommandMode mode, system_value command_index,
                                  void **args);
  void axisControl(system_value axis_index, variable_value value);
  ~ServoRobot(){};
  void connect();
  void disconnect();
  bool isAvaliable();
};
typedef std::vector<ServoRobot *> v_connections;
typedef v_connections::iterator v_connections_i;

struct AxisMinMax {
  int _min;
  int _max;
  std::string name;
  AxisMinMax(int _min, int _max, std::string name)
      : _min(_min), _max(_max), name(name) {}
};

class ServoRobotModule : public RobotModule {
  v_connections aviable_connections;
  FunctionData **robot_functions;
  AxisData **robot_axis;

  std::vector<AxisMinMax> axis_settings;
  int count_axis;
  const int count_functions = 1;

  bool is_prepare_failed;
  colorPrintfModuleVA_t *colorPrintf_p;
  ModuleInfo *mi;

  void colorPrintf(ConsoleColor colors, const char *mask, ...);

 public:
  ServoRobotModule();
  const struct ModuleInfo &getModuleInfo();
  void prepare(colorPrintfModule_t *colorPrintf_p,
               colorPrintfModuleVA_t *colorPrintfVA_p);

  FunctionData **getFunctions(unsigned int *_count_functions);
  AxisData **getAxis(unsigned int *count_axis);
  void *writePC(unsigned int *buffer_length);

  int init();
  Robot *robotRequire();
  void robotFree(Robot *robot);
  void final();

  void readPC(void *buffer, unsigned int buffer_length){};

  int startProgram(int uniq_index);
  int endProgram(int uniq_index);

  void destroy();
  ~ServoRobotModule(){};
};

#endif /* SERVO_ROBOT_MODULE_H */