#include "eiface.h"
#include "server_class.h"
#include "dt_common.h"
#include "iostream"

class NetvarDecompressor : public IServerPluginCallbacks
{
public:
	NetvarDecompressor() { };
	~NetvarDecompressor() { };
	virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);
	virtual void Unload(void) { };
	virtual void Pause(void) { };
	virtual void UnPause(void) { };
	virtual const char *GetPluginDescription(void);
	virtual void LevelInit(char const *pMapName) { };
	virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) { };
	virtual void GameFrame(bool simulating) { };
	virtual void LevelShutdown(void) { };
	virtual void ClientActive(edict_t *pEntity) { };
	virtual void ClientFullyConnect(edict_t *pEntity) { };
	virtual void ClientDisconnect(edict_t *pEntity) { };
	virtual void ClientPutInServer(edict_t *pEntity, char const *playername) { };
	virtual void SetCommandClient(int index) { };
	virtual void ClientSettingsChanged(edict_t *pEdict) { };
	virtual PLUGIN_RESULT ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen) { return PLUGIN_CONTINUE; };
	virtual PLUGIN_RESULT ClientCommand(edict_t *pEntity, const CCommand &args) { return PLUGIN_CONTINUE; };
	virtual PLUGIN_RESULT NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) { return PLUGIN_CONTINUE; };
	virtual void OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue) { };
	virtual void OnEdictAllocated(edict_t *edict) { };
	virtual void OnEdictFreed(const edict_t *edict) { };
	virtual bool BNetworkCryptKeyCheckRequired(uint32 unFromIP, uint16 usFromPort, uint32 unAccountIdProvidedByClient, bool bClientWantsToUseCryptKey) { return false; };
	virtual bool BNetworkCryptKeyValidate(uint32 unFromIP, uint16 usFromPort, uint32 unAccountIdProvidedByClient, int nEncryptionKeyIndexFromClient, int numEncryptedBytesFromClient, byte *pbEncryptedBufferFromClient, byte *pbPlainTextKeyForNetchan) { return false; };
};

ICvar *g_pCVar = NULL;

NetvarDecompressor g_NetvarDecompressor;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(NetvarDecompressor, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_NetvarDecompressor);

int g_PLID = 0;

IServerGameDLL *gamedll = NULL;

void CorrectProps(SendTable *table)
{
	CStandardSendProxies *sendproxies = gamedll->GetStandardSendProxies();
	int numProps = table->GetNumProps();
	for (int i = 0; i < numProps; i++)
	{
		SendProp *prop = table->GetProp(i);
		if (prop->GetDataTable() && prop->GetNumElements() > 0) 
		{
			if (std::string(prop->GetName()).substr(0, 1) == std::string("0")) continue;
			CorrectProps(prop->GetDataTable());
		}

		if (std::string(prop->GetName()) == std::string("m_fFlags"))
		{
			prop->m_nBits = 32;
			prop->SetProxyFn(sendproxies->m_Int32ToInt32);
			continue;
		}

		auto flags = prop->GetFlags();
		if (flags & SPROP_ENCODED_AGAINST_TICKCOUNT)
		{
			flags &= ~SPROP_ENCODED_AGAINST_TICKCOUNT;
			prop->SetFlags(flags);
		}

		switch (prop->GetType())
		{
			case DPT_Float:
			case DPT_Vector:
			case DPT_VectorXY:
				prop->SetFlags(SPROP_NOSCALE);
				prop->m_nBits = 32;
				break;
		}
	}
}

const char *NetvarDecompressor::GetPluginDescription(void)
{
	return "Sendprop fixes";
}

bool NetvarDecompressor::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
	gamedll = (IServerGameDLL *)gameServerFactory(INTERFACEVERSION_SERVERGAMEDLL, NULL);
	if (!gamedll)
	{
		Warning("Failed to get a pointer on ServerGameDLL.\n");
		return false;
	}

	g_pCVar = (ICvar *)interfaceFactory(CVAR_INTERFACE_VERSION, NULL);
	ConVar *sv_sendtables = g_pCVar->FindVar("sv_sendtables");
	sv_sendtables->SetValue(2);
	ServerClass *sc = gamedll->GetAllServerClasses();
	while (sc)
	{
		SendTable *table = sc->m_pTable;
		CorrectProps(table);
		sc = sc->m_pNext;
	}
	Msg("Netvars Decompressed.\n");

	return true;
}
