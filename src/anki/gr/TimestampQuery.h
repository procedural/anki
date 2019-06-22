// Copyright (C) 2009-2019, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <anki/gr/GrObject.h>

namespace anki
{

/// @addtogroup graphics
/// @{

/// Timestamp query.
class TimestampQuery : public GrObject
{
	ANKI_GR_OBJECT

public:
	static const GrObjectType CLASS_TYPE = GrObjectType::TIMESTAMP_QUERY;

protected:
	/// Construct.
	TimestampQuery(GrManager* manager, CString name)
		: GrObject(manager, CLASS_TYPE, name)
	{
	}

	/// Destroy.
	~TimestampQuery()
	{
	}

	/// Get the result if it's available. It won't block.
	TimestampQueryResult getResult(U64& timestamp) const;

private:
	/// Allocate and initialize new instance.
	static ANKI_USE_RESULT TimestampQuery* newInstance(GrManager* manager);
};
/// @}

} // end namespace anki
