// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Core/NativeWindow.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android_native_app_glue.h>

namespace anki
{

/// Native window implementation for Android
class NativeWindowImpl
{
public:
	ANativeWindow* m_nativeWindow = nullptr;
};

} // end namespace anki
