

#include "main.h"

#include <API.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "flywheel.h"
#include "utils.h"

typedef void(*Handler)(char const *);
typedef void(*FlywheelFloatAcceptor)(Flywheel *, float);
typedef void(*FlywheelBoolAcceptor)(Flywheel *, bool);

typedef struct HandlerMap
{
	char key[16];
	Handler handler;
}
HandlerMap;



void stdinHandler();
void handleRequest(const HandlerMap *api, const size_t apiSize, char const *request);
void handleSet(char const *request);
void handleSetFlywheelFloat(char const *request, FlywheelFloatAcceptor accept);
void handleSetFlywheelBool(char const *request, FlywheelBoolAcceptor accept);
void handleSetTarget(char const *request);
void handleSetController(char const *request);
void handleSetSmoothing(char const *request);
void handleSetPidKp(char const *request);
void handleSetPidKi(char const *request);
void handleSetPidKd(char const *request);
void handleSetTbhGain(char const *request);
void handleSetTbhApprox(char const *request);
void handleSetAllowReadify(char const *request);

HandlerMap methods[1] =
{
	{ "Set", handleSet }
};

#define METHODS_API_SIZE 1

HandlerMap setters[9] =
{
	{ "target", handleSetTarget },
	{ "controller", handleSetController },
	{ "smoothing", handleSetSmoothing },
	{ "PID.Kp", handleSetPidKp },
	{ "PID.Ki", handleSetPidKi },
	{ "PID.Kd", handleSetPidKd },
	{ "TBH.gain", handleSetTbhGain },
	{ "TBH.approx", handleSetTbhApprox },
	{ "allow-readify", handleSetAllowReadify }
};

#define SETTERS_API_SIZE 9




void stdinHandlerRun()
{
	taskCreate(stdinHandler, TASK_DEFAULT_STACK_SIZE, NULL, TASK_PRIORITY_DEFAULT);
}

void stdinHandler()
{
	char request[128];
	while (1)
	{
		//printf("Debug I am pending input from YOU.\nDebug   ^_^    \n");
		fgets(request, 128, stdin);
		//printf("Debug Hi I got a request. Lets see what it is...\n");
		//printf("Debug Request: '%s'\n", request);
		handleRequest(methods, METHODS_API_SIZE, request);
		delay(40);
	}
}



void handleRequest(const HandlerMap *api, const size_t apiSize, char const *request)
{
	size_t requestLength = strlen(request);
	size_t keyLength;
	for (int i = 0; i < apiSize; i++)
	{
		keyLength = strlen(api[i].key);
		if (!(stringStartsWith(api[i].key, request) && requestLength > keyLength))
		{
			//printf("Debug Not a %s request.\n", api[i].key);
			continue;
		}
		if (!isspace((unsigned char)request[keyLength]))
		{
			//printf("Debug Not a %s request.\n", api[i].key);
			continue;
		}
		//printf("Debug %s request.\n", api[i].key);
		int spaces = 0;
		while (isspace((unsigned char)request[keyLength + spaces]))
		{
			++spaces;
		}
		//printf("Debug Gottit, passing the request down.\n");
		api[i].handler(request + keyLength + spaces);
		break;
	}
}

void handleSet(char const *request)
{
	handleRequest(setters, SETTERS_API_SIZE, request);
}


void handleSetFlywheelFloat(char const *request, FlywheelFloatAcceptor accept)
{
	float value = stringToFloat(request);
	//printf("Debug String to float:\n");
	//printf("Debug %s --> %f\n", request, value);
	accept(flywheel, value);//TODO check request
}

void handleSetFlywheelBool(char const *request, FlywheelBoolAcceptor accept)
{
	if (stringStartsWith("true", request))
	{
		accept(flywheel, true);
	}
	else if (stringStartsWith("false", request))
	{
		accept(flywheel, false);
	}
}

void handleSetTarget(char const *request)
{
	handleSetFlywheelFloat(request, flywheelSet);
}

void handleSetController(char const *request)
{
	ControllerType controllerType;
	if (stringStartsWith("PID", request))
	{
		controllerType = CONTROLLER_TYPE_PID;
	}
	else if (stringStartsWith("TBH", request))
	{
		controllerType = CONTROLLER_TYPE_TBH;
	}
	else if (stringStartsWith("Bang-bang", request))
	{
		controllerType = CONTROLLER_TYPE_BANG_BANG;
	}
	else
	{
		return;
	}
	flywheelSetController(flywheel, controllerType);
}

void handleSetSmoothing(char const *request)
{
	handleSetFlywheelFloat(request, flywheelSetSmoothing);
}

void handleSetPidKp(char const *request)
{
	handleSetFlywheelFloat(request, flywheelSetPidKp);
}

void handleSetPidKi(char const *request)
{
	handleSetFlywheelFloat(request, flywheelSetPidKi);
}

void handleSetPidKd(char const *request)
{
	handleSetFlywheelFloat(request, flywheelSetPidKd);
}

void handleSetTbhGain(char const *request)
{
	handleSetFlywheelFloat(request, flywheelSetTbhGain);
}

void handleSetTbhApprox(char const *request)
{
	handleSetFlywheelFloat(request, flywheelSetTbhApprox);
}

void handleSetAllowReadify(char const *request)
{
	handleSetFlywheelBool(request, flywheelSetAllowReadify);
}