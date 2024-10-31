#pragma once

#define DEBUGLOG_DEFAULT_LOG_LEVEL_INFO
#define DEBUGLOG_DEFAULT_FILE_LEVEL_INFO
#define DEBUGLOG_ENABLE_FILE_LOGGER

#include <FS.h> // this header is needed, because for file logging FILE_WRITE has to be defined and FS.h defines FILE_WRITE properly
#include <DebugLog.h>
