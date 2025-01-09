#include <UESDK.hpp>
#include <private/OffsetFinder.hpp>

namespace SDK
{
    Status Init()
    {
        if (!OffsetFinder::FindFMemoryRealloc())
            return Status::Failed_FMemoryRealloc;

        if (!OffsetFinder::FindGObjects())
            return Status::Failed_GObjects;

        if (!OffsetFinder::FindFNameConstructorNarrow())
            return Status::Failed_NarrowFNameConstructor;

        if (!OffsetFinder::FindFNameConstructorWide())
            return Status::Failed_WideFNameConstructor;

        if (!OffsetFinder::FindAppendString())
            return Status::Failed_AppendString;

        if (const auto Status = OffsetFinder::SetupMemberOffsets(); Status != Status::Success)
            return Status;

        if (!OffsetFinder::FindProcessEventIdx())
            return Status::Failed_ProcessEvent;

        return Status::Success;
    }
}
