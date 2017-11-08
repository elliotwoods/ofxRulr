#pragma once

#ifdef BUILD_Plugin_Calibration
	#define PLUGIN_CALIBRATION_EXPORTS __declspec(dllexport)
#else
	#define PLUGIN_CALIBRATION_EXPORTS
#endif