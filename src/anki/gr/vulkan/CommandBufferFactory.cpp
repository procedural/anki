// Copyright (C) 2009-2017, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <anki/gr/vulkan/CommandBufferFactory.h>
#include <anki/core/Trace.h>

namespace anki
{

void MicroCommandBuffer::destroy()
{
	reset();

	if(m_handle)
	{
		vkFreeCommandBuffers(m_threadAlloc->m_factory->m_dev, m_threadAlloc->m_pool, 1, &m_handle);
		m_handle = {};
	}
}

void MicroCommandBuffer::reset()
{
	ANKI_ASSERT(m_refcount.load() == 0);
	ANKI_ASSERT(!m_fence.isCreated() || m_fence->done());

	m_objectRefs.destroy(m_fastAlloc);
	m_objectRefCount = 0;

	m_fastAlloc.getMemoryPool().reset();

	m_fence = {};
}

Error CommandBufferThreadAllocator::init()
{
	VkCommandPoolCreateInfo ci = {};
	ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	ci.queueFamilyIndex = m_factory->m_queueFamily;

	ANKI_VK_CHECK(vkCreateCommandPool(m_factory->m_dev, &ci, nullptr, &m_pool));

	return ErrorCode::NONE;
}

void CommandBufferThreadAllocator::destroyList(IntrusiveList<MicroCommandBuffer>& list)
{
	while(!list.isEmpty())
	{
		MicroCommandBuffer* ptr = &list.getFront();
		list.popFront();
		ptr->destroy();
		getAllocator().deleteInstance(ptr);
	}
}

void CommandBufferThreadAllocator::destroy()
{
	for(U i = 0; i < 2; ++i)
	{
		for(U j = 0; j < 2; ++j)
		{
			CmdbType& type = m_types[i][j];

			destroyList(type.m_readyCmdbs);
			destroyList(type.m_inUseCmdbs);
			destroyList(type.m_deletedCmdbs);
		}
	}

	if(m_pool)
	{
		vkDestroyCommandPool(m_factory->m_dev, m_pool, nullptr);
	}
}

Error CommandBufferThreadAllocator::newCommandBuffer(CommandBufferFlag cmdbFlags, MicroCommandBufferPtr& outPtr)
{
	cmdbFlags = cmdbFlags & (CommandBufferFlag::SECOND_LEVEL | CommandBufferFlag::SMALL_BATCH);

	Bool secondLevel = !!(cmdbFlags & CommandBufferFlag::SECOND_LEVEL);
	Bool smallBatch = !!(cmdbFlags & CommandBufferFlag::SMALL_BATCH);
	CmdbType& type = m_types[secondLevel][smallBatch];

	// Move the deleted to (possibly) in-use
	{
		LockGuard<Mutex> lock(type.m_deletedMtx);

		while(!type.m_deletedCmdbs.isEmpty())
		{
			MicroCommandBuffer* ptr = &type.m_deletedCmdbs.getFront();
			type.m_deletedCmdbs.popFront();

			if(secondLevel)
			{
				type.m_readyCmdbs.pushBack(ptr);
			}
			else
			{
				type.m_inUseCmdbs.pushBack(ptr);
			}
		}
	}

	// Reset the in-use command buffers and try to get one available
	MicroCommandBuffer* out = nullptr;
	if(!secondLevel)
	{
		// Primary

		IntrusiveList<MicroCommandBuffer> inUseCmdbs; // Push to temporary

		while(!type.m_inUseCmdbs.isEmpty())
		{
			MicroCommandBuffer* mcmdb = &type.m_inUseCmdbs.getFront();
			type.m_inUseCmdbs.popFront();

			if(!mcmdb->m_fence.isCreated() || mcmdb->m_fence->done())
			{
				// Can re-use it
				if(out)
				{
					type.m_readyCmdbs.pushBack(mcmdb);
				}
				else
				{
					out = mcmdb;
				}

				mcmdb->reset();
			}
			else
			{
				inUseCmdbs.pushBack(mcmdb);
			}
		}

		ANKI_ASSERT(type.m_inUseCmdbs.isEmpty());
		type.m_inUseCmdbs = std::move(inUseCmdbs);
	}
	else
	{
		ANKI_ASSERT(type.m_inUseCmdbs.isEmpty());

		if(!type.m_readyCmdbs.isEmpty())
		{
			out = &type.m_readyCmdbs.getFront();
			type.m_readyCmdbs.popFront();
		}
	}

	if(ANKI_UNLIKELY(out == nullptr))
	{
		// Create a new one

		VkCommandBufferAllocateInfo ci = {};
		ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		ci.commandPool = m_pool;
		ci.level = (secondLevel) ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		ci.commandBufferCount = 1;

		ANKI_TRACE_INC_COUNTER(VK_CMD_BUFFER_CREATE, 1);
		VkCommandBuffer cmdb;
		ANKI_VK_CHECK(vkAllocateCommandBuffers(m_factory->m_dev, &ci, &cmdb));

		MicroCommandBuffer* newCmdb = getAllocator().newInstance<MicroCommandBuffer>(this);
		newCmdb->m_fastAlloc = StackAllocator<U8>(m_factory->m_alloc.getMemoryPool().getAllocationCallback(),
			m_factory->m_alloc.getMemoryPool().getAllocationCallbackUserData(),
			(smallBatch) ? (1 * 1024) : (10 * 1024),
			2.0f);

		newCmdb->m_handle = cmdb;
		newCmdb->m_flags = cmdbFlags;

		out = newCmdb;
	}

	ANKI_ASSERT(out && out->m_refcount.load() == 0);
	outPtr.reset(out);
	return ErrorCode::NONE;
}

void CommandBufferThreadAllocator::deleteCommandBuffer(MicroCommandBuffer* ptr)
{
	ANKI_ASSERT(ptr);

	Bool secondLevel = !!(ptr->m_flags & CommandBufferFlag::SECOND_LEVEL);
	Bool smallBatch = !!(ptr->m_flags & CommandBufferFlag::SMALL_BATCH);

	if(secondLevel)
	{
		// We can safely reset the 2nd level cmdbs early
		ptr->reset();
	}

	CmdbType& type = m_types[secondLevel][smallBatch];

	LockGuard<Mutex> lock(type.m_deletedMtx);
	type.m_deletedCmdbs.pushBack(ptr);
}

Error CommandBufferFactory::init(GrAllocator<U8> alloc, VkDevice dev, uint32_t queueFamily)
{
	ANKI_ASSERT(dev);

	m_alloc = alloc;
	m_dev = dev;
	m_queueFamily = queueFamily;
	return ErrorCode::NONE;
}

void CommandBufferFactory::destroy()
{
	for(CommandBufferThreadAllocator* alloc : m_threadAllocs)
	{
		alloc->destroy();
		m_alloc.deleteInstance(alloc);
	}

	m_threadAllocs.destroy(m_alloc);
}

Error CommandBufferFactory::newCommandBuffer(ThreadId tid, CommandBufferFlag cmdbFlags, MicroCommandBufferPtr& ptr)
{
	CommandBufferThreadAllocator* alloc = nullptr;

	{
		LockGuard<SpinLock> lock(m_threadAllocMtx);

		class Comp
		{
		public:
			Bool operator()(const CommandBufferThreadAllocator* a, ThreadId tid) const
			{
				return a->m_tid < tid;
			}

			Bool operator()(ThreadId tid, const CommandBufferThreadAllocator* a) const
			{
				return tid < a->m_tid;
			}
		};

		// Find using binary search
		auto it = binarySearch(m_threadAllocs.getBegin(), m_threadAllocs.getEnd(), tid, Comp());

		if(it != m_threadAllocs.getEnd())
		{
			ANKI_ASSERT((*it)->m_tid == tid);
			alloc = *it;
		}
		else
		{
			alloc = m_alloc.newInstance<CommandBufferThreadAllocator>(this, tid);

			m_threadAllocs.resize(m_alloc, m_threadAllocs.getSize() + 1);
			m_threadAllocs[m_threadAllocs.getSize() - 1] = alloc;

			// Sort for fast find
			std::sort(m_threadAllocs.getBegin(),
				m_threadAllocs.getEnd(),
				[](const CommandBufferThreadAllocator* a, const CommandBufferThreadAllocator* b) {
					return a->m_tid < b->m_tid;
				});

			ANKI_CHECK(alloc->init());
		}
	}

	ANKI_ASSERT(alloc);
	ANKI_CHECK(alloc->newCommandBuffer(cmdbFlags, ptr));

	return ErrorCode::NONE;
}

} // end namespace anki