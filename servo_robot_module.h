#ifndef SERVO_ROBOT_MODULE_H
#define	SERVO_ROBOT_MODULE_H

struct ServoLimits {
  int _min;
  int _max;
  int servo_number;
  ServoLimits(int number, int _min, int _max) : servo_number(number), _min(_min), _max(_max){};
};

class ServoRobot : public Robot {
	private:
		colorPrintfRobotVA_t *colorPrintf_p;
		Serial *SP;
		int count_axis;
		std::vector<ServoLimits> servo_limits;
		std::vector<unsigned char> start_position;
		std::vector<unsigned char> safe_position;
		std::string port;
		void colorPrintf(ConsoleColor colors, const char *mask, ...);
		void setStartPosition();
		void setSafePosition(unsigned char command);
		bool is_aviable;
    public: 
		
		ServoRobot(std::string port, int count_axis, 
				   std::vector<ServoLimits> servo_limits,
				   std::vector<unsigned char> start_position, 
				   std::vector<unsigned char> safe_position) 
			: is_aviable(true), port(port), count_axis(count_axis), 
			  servo_limits(servo_limits), start_position(start_position),
			  safe_position(safe_position), SP(NULL) {}
		void prepare(colorPrintfRobot_t *colorPrintf_p,
               		 colorPrintfRobotVA_t *colorPrintfVA_p);
		FunctionResult *executeFunction(CommandMode mode, system_value command_index,
                                		void **args);
		void axisControl(system_value axis_index, variable_value value);
        ~ServoRobot();
        void connect();
        void disconnect();
        bool isAvaliable();
};
typedef std::vector<ServoRobot*> v_connections;
typedef v_connections::iterator v_connections_i;

class ServoRobotModule : public RobotModule {
	v_connections aviable_connections;
	FunctionData **robot_functions;
	AxisData **robot_axis;
	
	int COUNT_AXIS;

	colorPrintfModuleVA_t *colorPrintf_p;
	ModuleInfo *mi;

	void colorPrintf(ConsoleColor colors, const char *mask, ...);

	public:
		ServoRobotModule();
		const struct ModuleInfo &getModuleInfo();
		void prepare(colorPrintfModule_t *colorPrintf_p,
		        	 colorPrintfModuleVA_t *colorPrintfVA_p);

		FunctionData **getFunctions(unsigned int *count_functions);
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
		~ServoRobotModule() {};
};

#endif	/* SERVO_ROBOT_MODULE_H */