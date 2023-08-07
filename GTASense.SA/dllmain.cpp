// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <string>
#include "IniReader.h"
#include "winsock2.h"
#pragma comment(lib,"WS2_32")

enum WeaponType
{
    WEAPONTYPE_UNARMED,
    WEAPONTYPE_KNUCKLE,
    WEAPONTYPE_GOLF,
    WEAPONTYPE_STICK,
    WEAPONTYPE_KNIFE,
    WEAPONTYPE_BAT,
    WEAPONTYPE_SHOVEL,
    WEAPONTYPE_CUE,
    WEAPONTYPE_KATANA,
    WEAPONTYPE_CHAINSAW,
    WEAPONTYPE_PURPLEDILDO,
    WEAPONTYPE_WHITEDILDO,
    WEAPONTYPE_LONGVIBRATOR,
    WEAPONTYPE_SHORTVIBRATOR,
    WEAPONTYPE_FLOWERS,
    WEAPONTYPE_CANE,
    WEAPONTYPE_GRENADE,
    WEAPONTYPE_GAS,
    WEAPONTYPE_MOLOTOV,
    WEAPONTYPE_COLT = 22,
    WEAPONTYPE_SILENCED,
    WEAPONTYPE_DEAGLE,
    WEAPONTYPE_SHOTGUN,
    WEAPONTYPE_SAWNOFF,
    WEAPONTYPE_SHOTGSPA,
    WEAPONTYPE_UZI,
    WEAPONTYPE_MP5,
    WEAPONTYPE_AK47,
    WEAPONTYPE_M4,
    WEAPONTYPE_TEC9,
    WEAPONTYPE_RIFLE,
    WEAPONTYPE_SNIPER,
    WEAPONTYPE_ROCKETLA,
    WEAPONTYPE_HEATSEEK,
    WEAPONTYPE_FLAME,
    WEAPONTYPE_MINIGUN,
    WEAPONTYPE_SATCHEL,
    WEAPONTYPE_DETONATOR,
    WEAPONTYPE_SPRAYCAN,
    WEAPONTYPE_FIREEX,
    WEAPONTYPE_CAMERA,
    WEAPONTYPE_NVGOOGLES,
    WEAPONTYPE_IRGOOGLES,
    WEAPONTYPE_PARACHUTE
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
    int weapon_status = 0;
    bool wanted_odd = false;
    bool can_shoot = true;
    bool in_air_water = false;

    std::string getJson()
    {
        std::string json = std::string("{\"instructions\": [");
        // Right trigger
        json.append("{\"type\": 1, \"parameters\": [");
        json.append(std::to_string(controller_index));
        if (can_shoot && !in_air_water)
        {
            if (weapon_type == WEAPONTYPE_COLT || weapon_type == WEAPONTYPE_SILENCED || weapon_type == WEAPONTYPE_DEAGLE || weapon_type == WEAPONTYPE_DETONATOR || weapon_type == WEAPONTYPE_CAMERA) // Soft trigger
                json.append(", 2, " + std::to_string(TRIGGERMODE_SOFT));
            else if (weapon_type == WEAPONTYPE_SHOTGUN || weapon_type == WEAPONTYPE_SAWNOFF || weapon_type == WEAPONTYPE_SHOTGSPA || weapon_type == WEAPONTYPE_RIFLE || weapon_type == WEAPONTYPE_SNIPER || weapon_type == WEAPONTYPE_HEATSEEK || weapon_type == WEAPONTYPE_ROCKETLA) // Hard trigger
                json.append(", 2, " + std::to_string(TRIGGERMODE_HARD));
            else if ((weapon_type == WEAPONTYPE_TEC9 || weapon_type == WEAPONTYPE_UZI || weapon_type == WEAPONTYPE_MP5) && (weapon_status == 0 || weapon_status == 1)) // Vibrate trigger (power 15)
                json.append(", 2, " + std::to_string(TRIGGERMODE_VIBRATETRIGGER) + ", 15");
            else if ((weapon_type == WEAPONTYPE_M4 || weapon_type == WEAPONTYPE_AK47 || weapon_type == WEAPONTYPE_MINIGUN) && (weapon_status == 0 || weapon_status == 1)) // Vibrate trigger (power 20)
                json.append(", 2, " + std::to_string(TRIGGERMODE_VIBRATETRIGGER) + ", 20");
            else
                json.append(", 2, " + std::to_string(TRIGGERMODE_NORMAL)); // Normal
        }
        else
        {
            json.append(", 2, " + std::to_string(TRIGGERMODE_NORMAL));
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
            while (1)
            {
                int* player_data = reinterpret_cast<int*>(0xB7CD98);
                if (*player_data)
                {
                    int* player_ped = reinterpret_cast<int*>(*player_data);
                    std::to_string(*player_ped);
                    if (*player_ped)
                    {
                        if (enableWantedIndicator) 
                        {
                            int* CWanted = reinterpret_cast<int*>(0xB7CD98 + 0x4);
                            int wanted_level = *reinterpret_cast<uint8_t*>(*CWanted + 0x2C);
                            end_time = std::chrono::steady_clock::now();
                            if (duration_cast<std::chrono::milliseconds>(end_time - start_time).count() >= 500)
                            {
                                state.wanted_odd = !state.wanted_odd;
                                start_time = std::chrono::steady_clock::now();
                            }
                            state.wanted = ((int)wanted_level > 0) ? true : false;
                        }
                        if (enableAdaptiveTriggers)
                        {
                            int player_status = *reinterpret_cast<uint8_t*>(*player_data + 0x530);
                            int player_check = *reinterpret_cast<uint8_t*>(*player_data + 0x46C);
                            int weapon_slot = *reinterpret_cast<uint8_t*>(*player_data + 0x718);
                            int weapon_type = *reinterpret_cast<uint8_t*>(*player_data + 0x5A0 + (weapon_slot * 0x1C));
                            int weapon_status = *reinterpret_cast<uint8_t*>(*player_data + 0x5A0 + (weapon_slot * 0x1C) + 4);
                            state.can_shoot = (player_status == 1) ? true : false;
                            state.weapon_type = weapon_type;
                            state.weapon_status = weapon_status;
                            state.in_air_water = (player_check == 0 || player_check == 8) ? true : false;
                        }
                        std::string to_send = state.getJson();
                        sendto(s, to_send.c_str(), strlen(to_send.c_str()), 0, (struct sockaddr*)&si_other, slen);
                    }
                }
            }
        }
    }
}

BOOL APIENTRY DllMain(HMODULE hModule,
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

