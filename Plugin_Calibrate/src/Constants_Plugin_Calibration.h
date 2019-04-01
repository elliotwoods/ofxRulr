#pragma once

#ifdef BUILD_Plugin_Calibrate
	#define PLUGIN_CALIBRATE_EXPORTS __declspec(dllexport)
#else
	#define PLUGIN_CALIBRATE_EXPORTS
#endif