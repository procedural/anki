/*
 * Copyright (c) 2019 ARM Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdexcept>

#pragma once

#define HWCPIPE_TAG "HWCPipe"

#if defined(__ANDROID__)
#	include <android/log.h>

#	define HWCPIPE_LOG(...) __android_log_print(ANDROID_LOG_VERBOSE, HWCPIPE_TAG, __VA_ARGS__)
#else
#	define HWCPIPE_LOG(...)                              \
		{                                                 \
			fprintf(stdout, "%s [INFO] : ", HWCPIPE_TAG); \
			fprintf(stdout, __VA_ARGS__);                 \
			fprintf(stdout, "\n");                        \
		}
#endif

namespace std
{
	struct runtime_error_anki
	{
		runtime_error_anki(::std::string s)
        {
            HWCPIPE_LOG("%s", s.c_str());
			abort();
        }
	};
}

#define throw
#define runtime_error runtime_error_anki
#define try if(true)
#define catch(x) if(false)
