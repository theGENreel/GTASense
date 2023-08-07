// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <string>
#include <sstream>
#include "IniReader.h"
#include "winsock2.h"
#pragma comment(lib,"WS2_32")

enum WeaponType
{
    WEAPONTYPE_UNARMED,
    WEAPONTYPE_BAT,
    WEAPONTYPE_PISTOL,
    WEAPONTYPE_UZI,
    WEAPONTYPE_SHOTGUN,
    WEAPONTYPE_AK47,
    WEAPONTYPE_M16,
    WEAPONTYPE_SNIPERRIFLE,
    WEAPONTYPE_ROCKETLA,
    WEAPONTYPE_FLAMETHROWER,
    WEAPONTYPE_MOLOTOV,
    WEAPONTYPE_GRENADE,
    WEAPONTYPE_DETONATOR
};

enum TriggerMode
{
    TRIGGERMODE_NORMAL = 0,
    TRIGGERMODE_GAMECUBE = 1,
    TRIGGERMODE_VERYSOFT = 2,
    TRIGGERMODE_SOFT = 3,
    TRIGGERMODE_HARD = 4,
    TRIGGERMODE_VERYHARD = 5,
    TRIGGERMODE_HARDEST = 6,
    TRIGGERMODE_RIGID = 7,
    TRIGGERMODE_VIBRATETRIGGER = 8,
    TRIGGERMODE_CHOPPT = 9,
    TRIGGERMODE_MEDIUM = 10,
    TRIGGERMODE_VIBRATETRIGGERPULSE = 11,
    TRIGGERMODE_CUSTOMTRIGGERVALUE = 12,
    TRIGGERMODE_RESISTANCE = 13,
    TRIGGERMODE_BOW = 14,
    TRIGGERMODE_GALLOPING = 15,
    TRIGGERMODE_SEMIAUTOMATICGUN = 16,
    TRIGGERMODE_AUTOMATICGUN = 17,
    TRIGGERMODE_MACHINE = 18
};

class State
{
public:
    int controller_index = 0;
    bool wanted = false;
    int weapon_type = 0;
    bool wanted_odd = false;
    bool in_car = false;

    std::string getJson()
    {
        std::string json = std::string("{\"instructions\": [");
        // Right trigger
        json.append("{\"type\": 1, \"parameters\": [");
        json.append(std::to_string(controller_index));
        if (!in_car)
        {
            if (weapon_type == WEAPONTYPE_PISTOL || weapon_type == WEAPONTYPE_DETONATOR) // Soft trigger
                json.append(", 2, " + std::to_string(TRIGGERMODE_SOFT));
            else if (weapon_type == WEAPONTYPE_SHOTGUN || weapon_type == WEAPONTYPE_SNIPERRIFLE || weapon_type == WEAPONTYPE_ROCKETLA) // Hard trigger
                json.append(", 2, " + std::to_string(TRIGGERMODE_HARD));
            else if (weapon_type == WEAPONTYPE_UZI) // Vibrate trigger (power 15)
                json.append(", 2, " + std::to_string(TRIGGERMODE_VIBRATETRIGGER) + ", 15");
            else if (weapon_type == WEAPONTYPE_M16 || weapon_type == WEAPONTYPE_AK47) // Vibrate trigger (power 20)
                json.append(", 2, " + std::to_string(TRIGGERMODE_VIBRATETRIGGER) + ", 20");
            else
                json.append(", 2, " + std::to_string(TRIGGERMODE_NORMAL)); // Normal
        }
        else
        {
            json.append(", 2, " + std::to_string(TRIGGERMODE_NORMAL)); // In vehicle
        }
        json.append("]}, ");
        // Wanted indicator
        json.append("{\"type\": 2, \"parameters\": [");
        json.append(std::to_string(controller_index));
        if (wanted)
        {
            if (wanted_odd)
            {
                json.append(", 255, 0, 0");
            }
            else
            {
                json.append(", 0, 0, 255");
            }
        }
        else
            json.append(", 0, 0, 0");
        json.append("]}");
        //End
        json.append("]}");
        return json;
    }
};

void Thread()
{
    CIniReader iniReader("");
    bool enableAdaptiveTriggers = iniReader.ReadBoolean("MAIN", "EnableAdaptiveTriggers", true);
    bool enableWantedIndicator = iniReader.ReadBoolean("MAIN", "EnableWantedIndicator", true);

    State state = State();

    struct sockaddr_in si_other;
    int s, slen = sizeof(si_other);
    char buf[512];
    char message[512];
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0)
    {
        if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) != SOCKET_ERROR)
        {
            si_other.sin_family = AF_INET;
            si_other.sin_port = htons(6969);
            si_other.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
            std::chrono::steady_clock::time_point start_time;
            std::chrono::steady_clock::time_point end_time;
            int* player;
            if (*(DWORD*)0x5C1E75 == 0xB85548EC) // v1.0
                player = reinterpret_cast<int*>(0x006FB1C8);
            else if (*(DWORD*)0x5C2135 == 0xB85548EC) // v1.1
                player = reinterpret_cast<int*>(0x006FB1C8);
            else
                return;
            while (1)
            {
                if (*player)
                {
                    if (enableWantedIndicator)
                    {
                        int* CWanted = reinterpret_cast<int*>(*player + 0x53C);
                        uint32_t wanted_level = *reinterpret_cast<uint32_t*>(*CWanted + 0x18);
                        state.wanted = ((int)wanted_level > 0) ? true : false;
                        end_time = std::chrono::steady_clock::now();
                        if (duration_cast<std::chrono::milliseconds>(end_time - start_time).count() >= 500)
                        {
                            state.wanted_odd = !state.wanted_odd;
                            start_time = std::chrono::steady_clock::now();
                        }
                    }
                    if (enableAdaptiveTriggers)
                    {
                        uint8_t player_status = *reinterpret_cast<uint8_t*>(*player + 0x234);
                        int weapon_slot_num = *reinterpret_cast<unsigned char*>(*player + 0x554);
                        //MessageBoxA(0, std::to_string(weapon_slot_num).c_str(), "asd", 0);
                        state.weapon_type = weapon_slot_num;
                        state.in_car = (player_status == 3) ? true : false;
                    }

                    std::string to_send = state.getJson();
                    sendto(s, to_send.c_str(), strlen(to_send.c_str()), 0, (struct sockaddr*)&si_other, slen);
                }
            }
        }
    }
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(Thread), 0, 0, 0);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

