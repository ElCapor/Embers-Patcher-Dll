#pragma once
#include <uesdk/FastSearch.hpp>
#include <uesdk/State.hpp>
#include <uesdk/UnrealObjects.hpp>
#include <algorithm>
#include <array>
#include <format>

namespace SDK
{
    template <typename T>
    struct is_writable_pointer
    {
        static constexpr bool value = std::is_pointer_v<T> && !std::is_const_v<std::remove_pointer_t<T>> && !std::is_volatile_v<std::remove_pointer_t<T>>;
    };

    template <StringLiteral ClassName, StringLiteral FunctionName, typename ReturnType, typename... Args>
    ReturnType UObject::Call(UFunction* Function, Args&&... args)
    {
        constexpr size_t NumArgs = sizeof...(Args);
        static std::array<ArgInfo, (NumArgs > 0 ? NumArgs : 1)> ArgOffsets = { 0 };

        static size_t ParmsSize = 0;
        static int32_t ReturnValueOffset = 0;
        static int32_t ReturnValueSize = 0;
        static bool HasReturnValue = false;

        // If there are no arguments and no return type, we can simply call ProcessEvent.
        if constexpr (std::is_void_v<ReturnType> && !NumArgs) {
            ProcessEventAsNative(Function, nullptr);
            return;
        }

        static bool Setup = false;
        if (!Setup) {
            InitializeArgInfo(Function, ArgOffsets, ParmsSize, ReturnValueOffset, ReturnValueSize, HasReturnValue);
            Setup = true;
        }

        // Allocate memory on the stack for the parameters.
        uint8_t* Parms = static_cast<uint8_t*>(_malloca(ParmsSize));
        if (!Parms)
            throw std::bad_alloc();

        memset(Parms, 0, ParmsSize);

        // Write input arguments to the parameter buffer.
        {
            auto WriteInputArg = [Parms](auto& Arg, ArgInfo& Info) {
                using ArgType = std::decay_t<decltype(Arg)>;
                memcpy(Parms + Info.Offset, &Arg, std::min<int32_t>(Info.Size, sizeof(ArgType)));
            };

            int ArgIndex = 0;
            ((ArgOffsets[ArgIndex].IsOutParm ? void() : WriteInputArg(args, ArgOffsets[ArgIndex]), ++ArgIndex), ...);
        }

        ProcessEventAsNative(Function, Parms);

        // Write output arguments.
        {
            auto WriteOutputArg = [Parms](auto& Arg, ArgInfo& Info) {
                using ArgType = std::decay_t<decltype(Arg)>;

                constexpr bool IsWritable = is_writable_pointer<ArgType>::value;
                constexpr bool IsVoidPtr = std::is_pointer_v<ArgType> && !std::is_void_v<std::remove_pointer_t<ArgType>>;

                if constexpr (!IsWritable) {
                    throw std::invalid_argument("Output argument type is not writable!");
                    return;
                }
                if constexpr (!IsVoidPtr) {
                    throw std::invalid_argument("Output arguments are only supported via pointers!");
                    return;
                }

                if constexpr (IsWritable && IsVoidPtr) {
                    if (Arg) {
                        std::memcpy(Arg, Parms + Info.Offset, sizeof(*Arg));
                    }
                }
            };

            int ArgIndex = 0;
            ((ArgOffsets[ArgIndex].IsOutParm ? WriteOutputArg(args, ArgOffsets[ArgIndex]) : void(), ++ArgIndex), ...);
        }

        if constexpr (!std::is_void_v<ReturnType>) {
            if (!HasReturnValue)
                throw std::logic_error("Function template specifies return type, but UFunction does not contain a return value!");

            ReturnType Return = {};
            memcpy(&Return, Parms + ReturnValueOffset, std::min<int32_t>(ReturnValueSize, sizeof(ReturnType)));
            return Return;
        }
    }

    template <StringLiteral ClassName, StringLiteral FunctionName, typename ReturnType, typename... Args>
    ReturnType UObject::CallAuto(Args&&... args)
    {
        static UFunction* Function = nullptr;

        // If there is no function, search for it.
        if (!Function) {
            if (!FastSearchSingle(FSUFunction(ClassName.c_str(), FunctionName.c_str(), &Function))) {
                throw std::invalid_argument("Failed to automatically find UFunction!");
            }
        }

        return Call<ClassName, FunctionName, ReturnType, Args...>(Function, std::forward<Args>(args)...);
    }

    template <int N>
    void UObject::InitializeArgInfo(UFunction* Function, std::array<ArgInfo, N>& ArgOffsets, size_t& ParmsSize, int32_t& ReturnValueOffset, int32_t& ReturnValueSize, bool& HasReturnValue) const
    {
        ParmsSize = Function->ParmsSize();
        ReturnValueOffset = Function->ReturnValueOffset();
        HasReturnValue = ReturnValueOffset != UINT16_MAX;

        int ArgIndex = 0;
        if (State::UsesFProperty) {
            for (FField* Field = Function->ChildProperties(); Field; Field = Field->Next) {
                if (!Field->HasTypeFlag(CASTCLASS_FProperty))
                    continue;

                FProperty* Property = static_cast<FProperty*>(Field);
                if (Property->HasPropertyFlag(CPF_ReturnParm)) {
                    if (Property->Offset == ReturnValueOffset)
                        ReturnValueSize = Property->ElementSize;

                    continue;
                }

                ArgOffsets[ArgIndex] = { Property->Offset, Property->ElementSize, Property->HasPropertyFlag(CPF_OutParm) };
                ArgIndex++;
            }
        }
        else {
            for (UField* Child = Function->Children(); Child; Child = Child->Next()) {
                if (!Child->HasTypeFlag(CASTCLASS_FProperty))
                    continue;

                UProperty* Property = static_cast<UProperty*>(Child);
                if (Property->HasPropertyFlag(CPF_ReturnParm)) {
                    if (Property->Offset() == ReturnValueOffset)
                        ReturnValueSize = Property->ElementSize();

                    continue;
                }

                ArgOffsets[ArgIndex] = { Property->Offset(), Property->ElementSize(), Property->HasPropertyFlag(CPF_OutParm) };
                ArgIndex++;
            }
        }
    }

    template <StringLiteral ClassName, StringLiteral MemberName, typename MemberType>
    MemberType UObject::GetMember()
    {
        static int32_t Offset = OFFSET_NOT_FOUND;
        if (Offset == OFFSET_NOT_FOUND && !FastSearchSingle(FSProperty(ClassName.c_str(), MemberName.c_str(), &Offset, nullptr))) {
            throw std::runtime_error("Failed to find member offset!");
        }
        return *(MemberType*)((uintptr_t)this + Offset);
    }

    template <StringLiteral ClassName, StringLiteral MemberName, typename MemberType>
    MemberType* UObject::GetMemberPtr()
    {
        static int32_t Offset = OFFSET_NOT_FOUND;
        if (Offset == OFFSET_NOT_FOUND && !FastSearchSingle(FSProperty(ClassName.c_str(), MemberName.c_str(), &Offset, nullptr))) {
            throw std::runtime_error("Failed to find member offset!");
        }
        return (MemberType*)((uintptr_t)this + Offset);
    }

    template <StringLiteral ClassName, StringLiteral MemberName, typename MemberType>
    void UObject::SetMember(MemberType Value)
    {
        static int32_t Offset = OFFSET_NOT_FOUND;
        if (Offset == OFFSET_NOT_FOUND && !FastSearchSingle(FSProperty(ClassName.c_str(), MemberName.c_str(), &Offset, nullptr))) {
            throw std::runtime_error("Failed to find member offset!");
        }
        *(MemberType*)((uintptr_t)this + Offset) = Value;
    }
}
