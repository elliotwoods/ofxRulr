#pragma once

#include "ofConstants.h"

#ifdef TARGET_WIN32
#	if defined(PLUGIN_SCRAP_EXPORT_ENABLED)
#		define PLUGIN_SCRAP_API_ENTRY __declspec(dllexport)
#	else
#		define PLUGIN_SCRAP_API_ENTRY __declspec(dllimport)
#	endif
#else
#	define PLUGIN_SCRAP_API_ENTRY
#endif