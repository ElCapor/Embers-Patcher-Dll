#pragma once
#include <uesdk/UESDKStatus.hpp>

namespace SDK::OffsetFinder
{
    bool FindFMemoryRealloc();
    bool FindGObjects();
    bool FindFNameConstructorNarrow();
    bool FindFNameConstructorWide();
    bool FindAppendString();
    bool FindProcessEventIdx();

    Status SetupMemberOffsets();
}
