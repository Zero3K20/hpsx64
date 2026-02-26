/* windows.h interceptor for Linux builds of hpsx64
 * This file replaces the Windows headers on Linux/Steam Deck.
 * The actual compatibility layer is in windows_compat.h (force-included).
 */
#pragma once

#ifndef _WIN32
/* Already force-included by Makefile -include flag, but include directly if not */
#include "windows_compat.h"
#endif
