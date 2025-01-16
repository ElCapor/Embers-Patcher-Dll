#include <Windows.h>
#ifdef DEBUG
#include <console.hpp>
#endif
#include <oxorany.hpp>
#include <detours.h>
#include <UESDK.hpp>
#include <private/Offsets.hpp>
#include <unordered_set>
#include <thread>
#include <chrono>

#ifdef DEBUG
// Ceux la servaient juste pour pas qu'ils spamment ma console
std::unordered_set<std::string> bannedEvents = {
    oxorany("Tick"),
    oxorany("ReceiveTick"),
    oxorany("BlueprintModifyPostProcess"),
    oxorany("BlueprintModifyCamera"),
    oxorany("OnMouseMove"),
    oxorany("OnMouseLeave"),
    oxorany("OnMouseEnter"),
    oxorany("OnKeyDown"),
    oxorany("OnKeyUp"),
    oxorany("OnPreviewKeyDown"),
    oxorany("Mouse Actived"),
    oxorany("GetTextValue"),
    oxorany("GetMIDIPath"),
    oxorany("RefreshLerps"),
    oxorany("SetRenderTransform"),
    oxorany("SetRenderOpacity"),
    oxorany("OnAnimationFinished"),
    oxorany("SetPadding"),
    oxorany("SetColorAndOpacity"),
    oxorany("OnAnimationStarted"),
    oxorany("Construct"),
    oxorany("PreConstruct"),
    oxorany("OnCloseEvent"),
    oxorany("Destruct"),
    oxorany("RefreshEvent"),
    oxorany("OnInitialized"),
    oxorany("OnFocusLost"),
    oxorany("OnMouseButtonDown"),
    oxorany("OnPreviewMouseButtonDown"),
    oxorany("On Pressed_Event"),
    oxorany("On Released_Event"),
    oxorany("On Hovered_Event"),
    oxorany("On Unhovered_Event"),
    oxorany("OnMouseButtonUp"),
    oxorany("OnFocusReceived"),
    oxorany("CustomEvent"),
    oxorany("OnRemovedFromFocusPath"),
    oxorany("OnAddedToFocusPath"),
    oxorany("CustomEvent_5"),
    oxorany("SetMinimum"),
    oxorany("SetMaximum"),
    oxorany("SetAlignment"),
    oxorany("OnEditColors_Event"),
    oxorany("Update Display"),
    oxorany("ReceiveBeginPlay")
};

#endif

std::unordered_set<std::string> blockedEvents = {
    oxorany("Tier Update"),
    oxorany("User Update Event"),
    oxorany("UserRetrieved_Event"),
    oxorany("ExecuteUbergraph_BP_DiscordMan"),
};

using ProcessEvent_t = void (*)(SDK::UObject*, SDK::UFunction*, void*);
ProcessEvent_t oProcessEvent = nullptr;
void ProcessEventHook(SDK::UObject* obj, SDK::UFunction* func, void* params)
{ 
    #ifdef DEBUG
    if (bannedEvents.find(func->GetName()) == bannedEvents.end())
    {
        Console::log(oxorany("Process Event Called : "), func->GetName());
    }
    #endif
    if (blockedEvents.find(func->GetName()) != blockedEvents.end())
    {
        #ifdef DEBUG
        Console::log(oxorany("Blocked Event -> "), func->GetName());
        #endif
        return;
    }
    return oProcessEvent(obj, func, params);
}

DWORD WINAPI MainThread()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    #ifdef DEBUG
    Console::Open();
    Console::log(oxorany("Starting..."));
    #endif
    if (SDK::Init() != SDK::Status::Success)
    {
        #ifdef DEBUG
        Console::log(oxorany("Error..."));
        Console::Wait();
        #endif
        return 0;
    }

    SDK::UClass* EmberInstanceClass = nullptr;
    if (!SDK::FastSearchSingle(FSUClass(oxorany("EmberInstance_C"), &EmberInstanceClass)))
    {
        #ifdef DEBUG
        Console::log(oxorany("Error searching class..."));
        Console::Wait();
        #endif DEBUG
        return 0;
    }

    for (int i = 0; i < SDK::GObjects->Num(); i++) {
        SDK::UObject* Obj = SDK::GObjects->GetByIndex(i);
        
        // Check if the object is valid, not a default object and is or inherits from EmberInstance
        if (!Obj || Obj->IsDefaultObject() || !Obj->IsA(EmberInstanceClass))
            continue;

        #ifdef DEBUG
        Console::log(oxorany("Object located...."));
        #endif
        
        SDK::UFunction* Function = nullptr;

        // If there is no function, search for it.
        if (!Function) {
            if (!SDK::FastSearchSingle(SDK::FSUFunction(oxorany("EmberInstance_C"), oxorany("Discord Tier Update"), &Function))) {
                return 0;
            }
        }
        Obj->Call<"","",void,uint8_t>(Function, 3);
        static int32_t Offset = OFFSET_NOT_FOUND;
        if (Offset == OFFSET_NOT_FOUND && !SDK::FastSearchSingle(SDK::FSProperty(oxorany("EmberInstance_C"), oxorany("EmbersVersion"), &Offset, nullptr))) {
            return 0;
        }
        *(SDK::FString*)((uintptr_t)Obj + Offset) = SDK::FString(oxorany(L"2.3"));

        Offset = OFFSET_NOT_FOUND;
        if (Offset == OFFSET_NOT_FOUND && !SDK::FastSearchSingle(SDK::FSProperty(oxorany("EmberInstance_C"), oxorany("DiscordID"), &Offset, nullptr))) {
            return 0;
        }
        #ifdef DEBUG
        SDK::FString old_id = *(SDK::FString*)((uintptr_t)Obj + Offset);
        std::wcout << old_id.CStr() << std::endl;
        #endif
	// spoof your discord id
        *(SDK::FString*)((uintptr_t)Obj + Offset) = SDK::FString(oxorany(L""));

        oProcessEvent = reinterpret_cast<ProcessEvent_t>(Obj->VFT[SDK::Offsets::UObject::ProcessEventIdx]);

        #ifdef DEBUG
        Console::log(oxorany("ProcessEvent located..."));
        #endif

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)oProcessEvent, ProcessEventHook);
        DetourTransactionCommit();

        #ifdef DEBUG
        Console::log(oxorany("ProcessEvent Hooked"));
        #endif

    }

    #ifdef DEBUG
    Console::Wait();
    #endif
    return 0;
}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpvReserved )  // reserved
{
    // Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
         // Initialize once for each new process.
         // Return FALSE to fail DLL load.
         CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MainThread, NULL, NULL, NULL);
            break;
    }

    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}


template <typename Return, typename... Args>
using MethodPointer = Return(__fastcall*)(Args...);

template <typename Return, typename... Args>
auto Cast(void* function) -> MethodPointer<Return, Args...> {
    if (function) return static_cast<MethodPointer<Return, Args...>>(function);
    return nullptr;
}

#define METHOD(name, ret, ...) \
using t_##name = MethodPointer<ret, __VA_ARGS__>; \
t_##name o##name; \

#define METHOD2(name, ret, ...) __declspec(dllexport) ret name(__VA_ARGS__){

METHOD(GFSDK_Aftermath_DX11_CreateContextHandle, int, int, int);
METHOD(GFSDK_Aftermath_DX11_Initialize, int, int, int, int);
METHOD(GFSDK_Aftermath_DX12_CreateContextHandle, int, int, int);
METHOD(GFSDK_Aftermath_DX12_Initialize, int, int, int, int);
METHOD(GFSDK_Aftermath_GetData, int, int, int, int);
METHOD(GFSDK_Aftermath_GetDeviceStatus, int, int);
METHOD(GFSDK_Aftermath_GetPageFaultInformation, int, int);
METHOD(GFSDK_Aftermath_ReleaseContextHandle, int, int);
METHOD(GFSDK_Aftermath_SetEventMarker, int, int, int);

extern "C"
{
METHOD2(GFSDK_Aftermath_DX11_Initialize, int, int a1, int a2, int a3) 
    return oGFSDK_Aftermath_DX11_Initialize(a1,a2,a3);
}
METHOD2(GFSDK_Aftermath_DX11_CreateContextHandle,int, int a1, int a2)
    return oGFSDK_Aftermath_DX11_CreateContextHandle(a1,a2);
}
METHOD2(GFSDK_Aftermath_DX12_CreateContextHandle, int, int a1, int a2)
    return oGFSDK_Aftermath_DX12_CreateContextHandle(a1, a2);
}
METHOD2(GFSDK_Aftermath_DX12_Initialize, int, int a1, int a2, int a3)
    return oGFSDK_Aftermath_DX12_Initialize(a1, a2, a3);
}
METHOD2(GFSDK_Aftermath_GetData, int, int a1, int a2, int a3)
    return oGFSDK_Aftermath_GetData(a1, a2, a3);
}
METHOD2(GFSDK_Aftermath_GetDeviceStatus, int, int a1)
    return oGFSDK_Aftermath_GetDeviceStatus(a1);
}
METHOD2(GFSDK_Aftermath_GetPageFaultInformation, int, int a1)
    return oGFSDK_Aftermath_GetPageFaultInformation(a1);
}

METHOD2(GFSDK_Aftermath_SetEventMarker, int, int a1, int a2)
    return oGFSDK_Aftermath_SetEventMarker(a1, a2);
}


METHOD2(GFSDK_Aftermath_ReleaseContextHandle, int, int a1)

    return oGFSDK_Aftermath_ReleaseContextHandle(a1);
}

}
