#pragma once

#ifdef BUILD_Plugin_ArUco
#define PLUGIN_ARUCO_EXPORTS __declspec(dllexport)
#else
#define PLUGIN_ARUCO_EXPORTS
#endif