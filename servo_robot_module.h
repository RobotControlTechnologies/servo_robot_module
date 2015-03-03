#ifndef SERVO_ROBOT_MODULE_H
#define	SERVO_ROBOT_MODULE_H

class ServoRobot : public Robot {
	private:
#ifdef _DEBUG
		bool checkingSerial;
		unsigned int tid;
		HANDLE thread_handle;
		CRITICAL_SECTION cheking_mutex;
#endif
		Serial *SP;
		int count_axis;

    public: 
		bool is_aviable;
		ServoRobot::ServoRobot(Serial *SP, int count_axis) : is_aviable(true), SP(SP), count_axis(count_axis) {}
		FunctionResult* executeFunction(regval command_index, regval *args);
		void axisControl(regval axis_index, regval value);
#ifdef _DEBUG
		void startCheckingSerial();
		void checkSerial();
#endif
        ~ServoRobot();
};
typedef std::vector<ServoRobot*> v_connections;
typedef v_connections::iterator v_connections_i;

class ServoRobotModule : public RobotModule {
	v_connections aviable_connections;
	FunctionData **robot_functions;
	AxisData **robot_axis;
	bool is_error_init;
	CSimpleIniA ini;

	int COUNT_AXIS;

	public:
		ServoRobotModule();
		const char *getUID();
		void prepare(colorPrintf_t *colorPrintf_p, colorPrintfVA_t *colorPrintfVA_p);
		int init();
		FunctionData** getFunctions(int *count_functions);
		AxisData** getAxis(int *count_axis);
		Robot* robotRequire();
		void robotFree(Robot *robot);
		void final();
		void destroy();
		~ServoRobotModule() {};
};

#define ADD_ROBOT_FUNCTION(FUNCTION_NAME, COUNT_PARAMS, GIVE_EXCEPTION) \
	robot_functions[function_id] = new FunctionData; \
	robot_functions[function_id]->command_index = function_id + 1; \
	robot_functions[function_id]->count_params = COUNT_PARAMS; \
	robot_functions[function_id]->give_exception = GIVE_EXCEPTION; \
	robot_functions[function_id]->name = FUNCTION_NAME; \
	++function_id;

#define ADD_ROBOT_AXIS(AXIS_NAME, UPPER_VALUE, LOWER_VALUE) \
	robot_axis[axis_id] = new AxisData; \
	robot_axis[axis_id]->axis_index = axis_id + 1; \
	robot_axis[axis_id]->upper_value = UPPER_VALUE; \
	robot_axis[axis_id]->lower_value = LOWER_VALUE; \
	robot_axis[axis_id]->name = AXIS_NAME; \
	++axis_id;

#endif	/* SERVO_ROBOT_MODULE_H */