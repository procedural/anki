// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <Tests/Framework/Framework.h>
#include <AnKi/Util/BuddyAllocatorBuilder.h>

namespace anki {

ANKI_TEST(Util, BuddyAllocatorBuilder)
{
	HeapAllocator<U8> alloc(allocAligned, nullptr);

	// Simple
	{
		BuddyAllocatorBuilder<4> buddy(alloc, 4);

		Array<U32, 2> addr;
		Bool success = buddy.allocate(1, addr[0]);
		success = buddy.allocate(3, addr[1]);
		(void)success;

		// buddy.debugPrint();

		buddy.free(addr[0], 1);
		buddy.free(addr[1], 3);

		// printf("\n");
		// buddy.debugPrint();
	}

	// Fuzzy
	{
		BuddyAllocatorBuilder<32> buddy(alloc, 32);
		std::vector<std::pair<U32, U32>> allocations;
		for(U32 it = 0; it < 1000; ++it)
		{
			if((getRandom() % 2) == 0)
			{
				// Do an allocation
				U32 addr;
				const U32 size = max<U32>(getRandom() % 512, 1);
				const Bool success = buddy.allocate(size, addr);
				if(success)
				{
					allocations.push_back({addr, size});
				}
			}
			else
			{
				// Do some deallocation
				if(allocations.size())
				{
					const PtrSize randPos = getRandom() % allocations.size();
					buddy.free(allocations[randPos].first, allocations[randPos].second);

					allocations.erase(allocations.begin() + randPos);
				}
			}
		}

		// Remove the remaining
		for(const auto& pair : allocations)
		{
			buddy.free(pair.first, pair.second);
		}
	}
}

} // end namespace anki