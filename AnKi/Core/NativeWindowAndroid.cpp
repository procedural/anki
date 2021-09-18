// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Core/NativeWindowAndroid.h>

namespace anki
{

Error NativeWindow::init(NativeWindowInitInfo& init, HeapAllocator<U8>& alloc)
{
	ANKI_CORE_LOGI("Initializing Android window");

	m_alloc = alloc;
	m_impl = m_alloc.newInstance<NativeWindowImpl>();

	// Loop until the window is ready
	while(g_androidApp->window == nullptr)
	{
		int ident;
		int events;
		android_poll_source* source;

		const int timeoutMs = 5;
		while((ident = ALooper_pollAll(timeoutMs, nullptr, &events, reinterpret_cast<void**>(&source))) >= 0)
		{
			if(source != nullptr)
			{
				source->process(g_androidApp, source);
			}
		}
	}

	m_impl->m_nativeWindow = g_androidApp->window;

	// Set some stuff
	m_width = ANativeWindow_getWidth(g_androidApp->window);
	m_height = ANativeWindow_getHeight(g_androidApp->window);

	return Error::NONE;
}

void NativeWindow::destroy()
{
	ANKI_CORE_LOGI("Destroying Android window");
	ANativeActivity_finish(g_androidApp->activity);

	// Loop until destroyRequested is set
	while(!g_androidApp->destroyRequested)
	{
		int ident;
		int events;
		android_poll_source* source;

		while((ident = ALooper_pollAll(0, nullptr, &events, reinterpret_cast<void**>(&source))) >= 0)
		{
			if(source != nullptr)
			{
				source->process(g_androidApp, source);
			}
		}
	}

	m_alloc.deleteInstance(m_impl);
}

void NativeWindow::setWindowTitle(CString title)
{
	// Nothing
}

} // end namespace anki
