﻿#include <ShlObj.h>

#include "f4se/PluginAPI.h"
#include "f4se_common/f4se_version.h"

#include "form_cache.hpp"
#include "injection_data.hpp"
#include "papyrus_lootman.hpp"
#include "properties.hpp"
#include "vendor_chest.hpp"

#ifdef _DEBUG
#include "papyrus_debug.hpp"
#endif

IDebugLog gLog;

PluginHandle pluginHandle = kPluginHandle_Invalid;
F4SEPapyrusInterface* papyrus = nullptr;
F4SEMessagingInterface* messaging = nullptr;

void OnMessaging(F4SEMessagingInterface::Message* msg)
{
    const auto prefix = "| SYSTEM     |";
    _MESSAGE("%s F4SE messaging: %d", prefix, msg->type);
    if (msg->type == F4SEMessagingInterface::kMessage_GameLoaded)
    {
        form_cache::Initialize();
        properties::Initialize();
        injection_data::LoadInjectionData();
        vendor_chest::Initialize();
    }
    else if (msg->type == F4SEMessagingInterface::kMessage_PreLoadGame)
    {
        papyrus_lootman::OnPreLoadGame();
    }
}

extern "C" {
bool F4SEPlugin_Query(const F4SEInterface* f4se, PluginInfo* info)
{
    gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\lootman.log");

    const auto prefix = "| INITIALIZE |";
    _MESSAGE("%s [ Start plugin query phase ]", prefix);

    info->infoVersion = PluginInfo::kInfoVersion;
    info->name = "LootMan";
    info->version = 11016310; // FO4 1.10.163-10

    if (f4se->isEditor)
    {
        _FATALERROR("%s   Loaded in editor, marking as incompatible", prefix);
        return false;
    }

    if (f4se->runtimeVersion != RUNTIME_VERSION_1_10_163)
    {
        _FATALERROR("%s   Unsupported runtime version: %08X", prefix, f4se->runtimeVersion);
        return false;
    }

    papyrus = static_cast<F4SEPapyrusInterface*>(f4se->QueryInterface(kInterface_Papyrus));
    if (!papyrus)
    {
        _FATALERROR("%s   Couldn't get papyrus interface", prefix);
        return false;
    }

    pluginHandle = f4se->GetPluginHandle();
    messaging = static_cast<F4SEMessagingInterface*>(f4se->QueryInterface(kInterface_Messaging));
    if (!messaging)
    {
        _FATALERROR("%s   Couldn't get message interface", prefix);
        return false;
    }

    _MESSAGE("%s   Complete the plugin query phase", prefix);
    return true;
}

bool F4SEPlugin_Load(const F4SEInterface* f4se)
{
    const auto prefix = "| INITIALIZE |";
    _MESSAGE("%s [ Start plugin load phase ]", prefix);

    if (!injection_data::Initialize())
    {
        _FATALERROR("%s   Failed to initialization for injection data", prefix);
        return false;
    }

    if (!papyrus->Register(papyrus_lootman::Register))
    {
        _FATALERROR("%s   Failed to register papyrus functions for lootman", prefix);
        return false;
    }

#ifdef _DEBUG
    if (!papyrus->Register(papyrus_debug::Register))
    {
        _FATALERROR("%s   Failed to register papyrus functions for debug", prefix);
        return false;
    }
#endif

    if (!messaging->RegisterListener(pluginHandle, "F4SE", OnMessaging))
    {
        _FATALERROR("%s   Failed to messaging functions", prefix);
        return false;
    }

    _MESSAGE("%s   Complete the plugin load phase", prefix);
    return true;
}
}
