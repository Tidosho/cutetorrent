#pragma once
#undef interface
#include <iostream>
#include <vector>
#include <map>
//#include <unordered_map>
// all interface IDs supported
typedef enum {
	//INTERFACE_STEAMCLIENT008,
	INTERFACE_STEAMUSER016,
	INTERFACE_STEAMUSERSTATS11,
	//INTERFACE_STEAMREMOTESTORAGE002,
	INTERFACE_STEAMUTILS005,
	//INTERFACE_STEAMNETWORKING003,
	INTERFACE_STEAMFRIENDS013,
	INTERFACE_STEAMMATCHMAKING009
	//INTERFACE_STEAMGAMESERVER009,
	//INTERFACE_STEAMMASTERSERVERUPDATER001
} SteamInterface_t;

struct SteamInterface2_s
{
	SteamInterface_t interface;
	void* instance;
};

#define interface __STRUCT__

// result type
typedef struct SteamAPIResult_s
{
	void* data;
	int size;
	int type;
	SteamAPICall_t call;
} SteamAPIResult_t;

// basic class
class CSteamBase {
private:
	static std::vector<SteamInterface2_s> _instances;
	static std::map<SteamAPICall_t, bool> _calls;
	static std::map<SteamAPICall_t, CCallbackBase*> _resultHandlers;
	static std::vector<SteamAPIResult_t> _results;
	static std::vector<CCallbackBase*> _callbacks;

	static int _callID;
private:
	static void* CreateInterface(SteamInterface_t interfaceID);
public:
	// get interface instance from identifier
	static void* GetInterface(SteamInterface_t interfaceID);

	// run callbacks
	static void RunCallbacks();

	// register a global callback
	static void RegisterCallback(CCallbackBase* handler, int callback);

	// register a call result
	static void RegisterCallResult(SteamAPICall_t call, CCallbackBase* result);

	// register a call
	static SteamAPICall_t RegisterCall();

	// return a callback
	static void ReturnCall(void* data, int size, int type, SteamAPICall_t call);	
};