﻿#include "ntifs.h"
UNICODE_STRING GetDeviceObjectName(PDEVICE_OBJECT deviceObject);
PCHAR ProcessVisibleBytes(PVOID data, SIZE_T dataSize);