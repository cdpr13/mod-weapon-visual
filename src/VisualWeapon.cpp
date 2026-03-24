/*
** Made by Rochet2(Eluna)
** Rewritten by Poszer & Talamortis https://github.com/poszer/ & https://github.com/talamortis/
** AzerothCore 2019 http://www.azerothcore.org/
** Cleaned and made into a module by Micrah https://github.com/milestorme/
** Modified by GlacierWoW: Spanish translation, gold cost, preview system
*/

#include "ScriptMgr.h"
#include "Player.h"
#include "Chat.h"
#include "ScriptedGossip.h"
#include "Config.h"

using namespace std;

enum VisualWeaponsGossip
{
    VIS_DEFAULT_MESSAGE         = 907,
    VIS_GOSSIP_MAIN_MENU_ACTION = 100,
    VIS_GOSSIP_MAIN_HAND_ACTION = 200,
    VIS_GOSSIP_OFF_HAND_ACTION  = 300,
    VIS_GOSSIP_CLOSE_ACTION     = 400,
    VIS_GOSSIP_REMOVE_ACTION    = 500,
    VIS_GOSSIP_CONFIRM_ACTION   = 600,
    VIS_GOSSIP_CANCEL_ACTION    = 700
};

struct VisualData
{
    uint32 Menu;
    uint32 Submenu;
    uint32 Icon;
    uint32 Id;
    string Name;
};

VisualData vData[] =
{
    { 1, VIS_GOSSIP_MAIN_MENU_ACTION, GOSSIP_ICON_TALK, 0, "Volver.." },
    { 1, 2, GOSSIP_ICON_INTERACT_1, 0, "Siguiente.." },
    { 1, 0, GOSSIP_ICON_BATTLE, 3789, "Rabiar" },
    { 1, 0, GOSSIP_ICON_BATTLE, 3854, "Poder Arcano" },
    { 1, 0, GOSSIP_ICON_BATTLE, 3273, "Escarcha Mortal" },
    { 1, 0, GOSSIP_ICON_BATTLE, 3225, "Verdugo" },
    { 1, 0, GOSSIP_ICON_BATTLE, 3870, "Drenar Sangre" },
    { 1, 0, GOSSIP_ICON_BATTLE, 1899, "Arma Profana" },
    { 1, 0, GOSSIP_ICON_BATTLE, 2674, "Oleada Arcana" },
    { 1, 0, GOSSIP_ICON_BATTLE, 2675, "Maestro de Batalla" },
    { 1, 0, GOSSIP_ICON_BATTLE, 2671, "Poder Arcano y Fuego" },
    { 1, 0, GOSSIP_ICON_BATTLE, 2672, "Poder de Sombras y Escarcha" },
    { 1, 0, GOSSIP_ICON_BATTLE, 3365, "Runa de Rompe-espadas" },
    { 1, 0, GOSSIP_ICON_BATTLE, 2673, "Mangosta" },
    { 1, 0, GOSSIP_ICON_BATTLE, 2343, "Poder con Hechizos" },

    { 2, VIS_GOSSIP_MAIN_MENU_ACTION, GOSSIP_ICON_TALK, 0, "Volver.." },
    { 2, 3, GOSSIP_ICON_INTERACT_1, 0, "Siguiente.." },
    { 2, 1, GOSSIP_ICON_INTERACT_1, 0, "Anterior.." },
    { 2, 0, GOSSIP_ICON_BATTLE, 425, "Templo Oscuro" },
    { 2, 0, GOSSIP_ICON_BATTLE, 3855, "Poder con Hechizos III" },
    { 2, 0, GOSSIP_ICON_BATTLE, 1894, "Arma Helada" },
    { 2, 0, GOSSIP_ICON_BATTLE, 1103, "Agilidad" },
    { 2, 0, GOSSIP_ICON_BATTLE, 1898, "Robar Vida" },
    { 2, 0, GOSSIP_ICON_BATTLE, 3345, "Tierra Viva" },
    { 2, 0, GOSSIP_ICON_BATTLE, 1743, "Hoja Espectral" },
    { 2, 0, GOSSIP_ICON_BATTLE, 3093, "Poder vs No-muertos" },
    { 2, 0, GOSSIP_ICON_BATTLE, 1900, "Cruzado" },
    { 2, 0, GOSSIP_ICON_BATTLE, 3846, "Poder con Hechizos II" },
    { 2, 0, GOSSIP_ICON_BATTLE, 1606, "Poder de Ataque" },
    { 2, 0, GOSSIP_ICON_BATTLE, 283, "Furia de Viento" },
    { 2, 0, GOSSIP_ICON_BATTLE, 1, "Golpe de Roca" },

    { 3, VIS_GOSSIP_MAIN_MENU_ACTION, GOSSIP_ICON_TALK, 0, "Volver.." },
    { 3, 2, GOSSIP_ICON_INTERACT_1, 0, "Anterior.." },
    { 3, 0, GOSSIP_ICON_BATTLE, 3265, "Aceite Bendito" },
    { 3, 0, GOSSIP_ICON_BATTLE, 2, "Marca de Escarcha" },
    { 3, 0, GOSSIP_ICON_BATTLE, 3, "Lengua de Fuego" },
    { 3, 0, GOSSIP_ICON_BATTLE, 3266, "Aceite Sagrado" },
    { 3, 0, GOSSIP_ICON_BATTLE, 1903, "Espiritu" },
    { 3, 0, GOSSIP_ICON_BATTLE, 13, "Afilado" },
    { 3, 0, GOSSIP_ICON_BATTLE, 26, "Aceite de Escarcha" },
    { 3, 0, GOSSIP_ICON_BATTLE, 7, "Veneno Mortal" },
    { 3, 0, GOSSIP_ICON_BATTLE, 803, "Arma Ardiente" },
    { 3, 0, GOSSIP_ICON_BATTLE, 1896, "Dano de Arma" },
    { 3, 0, GOSSIP_ICON_BATTLE, 2666, "Intelecto" },
    { 3, 0, GOSSIP_ICON_BATTLE, 25, "Aceite de Sombras" },
};

// Per-player preview state
struct PreviewState
{
    uint32 VisualId;
    uint32 OriginalVisual;
    bool MainHand;
    bool Active;
};

static std::unordered_map<uint64, PreviewState> playerPreviewState;

class VisualWeaponNPC : public CreatureScript
{
public:
    VisualWeaponNPC() : CreatureScript("npc_visualweapon") { }

    bool MainHand;

    uint32 GetCost()
    {
        return sConfigMgr->GetOption<uint32>("VisualWeapon.Cost", 50) * 10000; // Config in gold, convert to copper
    }

    uint32 GetCostGold()
    {
        return sConfigMgr->GetOption<uint32>("VisualWeapon.Cost", 50);
    }

    void ApplyPreview(Player* player, uint32 visual_id)
    {
        uint8 slot = MainHand ? EQUIPMENT_SLOT_MAINHAND : EQUIPMENT_SLOT_OFFHAND;
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);

        if (!item)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000No tienes un arma equipada en esa mano.|r");
            return;
        }

        const ItemTemplate* itemTemplate = item->GetTemplate();
        if (itemTemplate->Class != ITEM_CLASS_WEAPON)
            return;

        if (itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_BOW ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_GUN ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_obsolete ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_THROWN ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_SPEAR ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_CROSSBOW ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_WAND ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_FISHING_POLE)
            return;

        // Save original visual for cancel
        uint64 guid = player->GetGUID().GetRawValue();
        PreviewState& state = playerPreviewState[guid];
        if (!state.Active)
            state.OriginalVisual = player->GetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (item->GetSlot() * 2), 0);

        state.VisualId = visual_id;
        state.MainHand = MainHand;
        state.Active = true;

        // Apply visual preview (only visual, not saved to DB)
        player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (item->GetSlot() * 2), 0, visual_id);
    }

    void ConfirmVisual(Player* player)
    {
        uint64 guid = player->GetGUID().GetRawValue();
        auto it = playerPreviewState.find(guid);
        if (it == playerPreviewState.end() || !it->second.Active)
            return;

        PreviewState& state = it->second;
        uint8 slot = state.MainHand ? EQUIPMENT_SLOT_MAINHAND : EQUIPMENT_SLOT_OFFHAND;
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);

        if (!item)
            return;

        uint32 cost = GetCost();

        if (cost > 0 && !player->HasEnoughMoney(cost))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000No tienes suficiente oro. Necesitas %u oro.|r", GetCostGold());
            // Revert visual
            player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (item->GetSlot() * 2), 0, state.OriginalVisual);
            state.Active = false;
            return;
        }

        if (cost > 0)
            player->ModifyMoney(-static_cast<int32>(cost));

        // Save to DB
        CharacterDatabase.Execute("REPLACE into `mod_weapon_visual_effect` (`item_guid`, `enchant_visual_id`) VALUES ('{}', '{}')", item->GetGUID().GetCounter(), state.VisualId);

        ChatHandler(player->GetSession()).PSendSysMessage("|cff00FF00Efecto visual aplicado exitosamente!|r");
        state.Active = false;
    }

    void CancelPreview(Player* player)
    {
        uint64 guid = player->GetGUID().GetRawValue();
        auto it = playerPreviewState.find(guid);
        if (it == playerPreviewState.end() || !it->second.Active)
            return;

        PreviewState& state = it->second;
        uint8 slot = state.MainHand ? EQUIPMENT_SLOT_MAINHAND : EQUIPMENT_SLOT_OFFHAND;
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);

        if (item)
            player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (item->GetSlot() * 2), 0, state.OriginalVisual);

        state.Active = false;
    }

    void RemoveVisual(Player* player)
    {
        uint8 slot = MainHand ? EQUIPMENT_SLOT_MAINHAND : EQUIPMENT_SLOT_OFFHAND;
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);

        if (!item)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("|cffFF0000No tienes un arma equipada en esa mano.|r");
            return;
        }

        player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (item->GetSlot() * 2), 0, 0);
        CharacterDatabase.Execute("DELETE FROM `mod_weapon_visual_effect` WHERE `item_guid` = '{}'", item->GetGUID().GetCounter());

        ChatHandler(player->GetSession()).PSendSysMessage("|cff00FF00Efecto visual removido.|r");
    }

    void GetMenu(Player* player, Creature* creature, uint32 menuId)
    {
        uint32 costGold = GetCostGold();
        for (uint8 i = 0; i < (sizeof(vData) / sizeof(*vData)); i++)
        {
            if (vData[i].Menu == menuId)
            {
                // Show cost next to enchant names (not navigation items)
                if (vData[i].Submenu == 0 && vData[i].Id != 0 && costGold > 0)
                {
                    string nameWithCost = vData[i].Name + " |cffFFD700[" + to_string(costGold) + " oro]|r";
                    AddGossipItemFor(player, vData[i].Icon, nameWithCost, GOSSIP_SENDER_MAIN, i);
                }
                else
                    AddGossipItemFor(player, vData[i].Icon, vData[i].Name, GOSSIP_SENDER_MAIN, i);
            }
        }

        player->PlayerTalkClass->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void GetConfirmMenu(Player* player, Creature* creature, string enchantName)
    {
        uint32 costGold = GetCostGold();
        string confirmText;
        if (costGold > 0)
            confirmText = "|TInterface/ICONS/Spell_ChargePositive:20:20|t |cff00FF00Confirmar|r - " + enchantName + " |cffFFD700(" + to_string(costGold) + " oro)|r";
        else
            confirmText = "|TInterface/ICONS/Spell_ChargePositive:20:20|t |cff00FF00Confirmar|r - " + enchantName;

        AddGossipItemFor(player, GOSSIP_ICON_BATTLE, confirmText, GOSSIP_SENDER_MAIN, VIS_GOSSIP_CONFIRM_ACTION);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/Spell_ChargeNegative:20:20|t |cffFF0000Cancelar|r (revertir preview)", GOSSIP_SENDER_MAIN, VIS_GOSSIP_CANCEL_ACTION);

        player->PlayerTalkClass->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void GetMainMenu(Player* player, Creature* creature)
    {
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|TInterface/PaperDoll/UI-PaperDoll-Slot-MainHand:40:40:-18|tMano Principal", GOSSIP_SENDER_MAIN, VIS_GOSSIP_MAIN_HAND_ACTION);
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|TInterface/PaperDoll/UI-PaperDoll-Slot-SecondaryHand:40:40:-18|tMano Secundaria", GOSSIP_SENDER_MAIN, VIS_GOSSIP_OFF_HAND_ACTION);
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|TInterface/ICONS/INV_Enchant_Disenchant:40:40:-18|t|cffFF4444Quitar Efecto Visual|r", GOSSIP_SENDER_MAIN, VIS_GOSSIP_REMOVE_ACTION);
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|TInterface/PaperDollInfoFrame/UI-GearManager-Undo:40:40:-18|tCerrar", GOSSIP_SENDER_MAIN, VIS_GOSSIP_CLOSE_ACTION);

        player->PlayerTalkClass->SendGossipMenu(VIS_DEFAULT_MESSAGE, creature->GetGUID());
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        // Cancel any active preview when reopening menu
        CancelPreview(player);
        GetMainMenu(player, creature);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();

        switch (action)
        {
            case VIS_GOSSIP_MAIN_HAND_ACTION:
                CancelPreview(player);
                MainHand = true;
                GetMenu(player, creature, 1);
                return true;

            case VIS_GOSSIP_OFF_HAND_ACTION:
                CancelPreview(player);
                MainHand = false;
                GetMenu(player, creature, 1);
                return true;

            case VIS_GOSSIP_CLOSE_ACTION:
                CancelPreview(player);
                CloseGossipMenuFor(player);
                return false;

            case VIS_GOSSIP_REMOVE_ACTION:
                CancelPreview(player);
                RemoveVisual(player);
                CloseGossipMenuFor(player);
                return true;

            case VIS_GOSSIP_CONFIRM_ACTION:
                ConfirmVisual(player);
                CloseGossipMenuFor(player);
                return true;

            case VIS_GOSSIP_CANCEL_ACTION:
                CancelPreview(player);
                GetMainMenu(player, creature);
                return true;
        }

        uint32 menuData = vData[action].Submenu;

        if (menuData == VIS_GOSSIP_MAIN_MENU_ACTION)
        {
            CancelPreview(player);
            GetMainMenu(player, creature);
            return true;
        }
        else if (menuData == 0)
        {
            // Preview: apply visual temporarily, show confirm menu
            ApplyPreview(player, vData[action].Id);
            GetConfirmMenu(player, creature, vData[action].Name);
            return true;
        }

        GetMenu(player, creature, menuData);
        return true;
    }
};

class VisualWeaponPlayer : public PlayerScript
{
public:
    VisualWeaponPlayer() : PlayerScript("VisualWeaponPlayer")
    {
        // Delete unused rows from DB table
        CharacterDatabase.Execute("DELETE FROM `mod_weapon_visual_effect` WHERE NOT EXISTS(SELECT 1 FROM item_instance WHERE `mod_weapon_visual_effect`.item_guid = item_instance.guid)");
    }

    void GetVisual(Player* player)
    {
        if (!player)
            return;

        Item* pItem;

        // We need to query the DB to get item
        QueryResult result = CharacterDatabase.Query("SELECT item_guid, enchant_visual_id FROM `mod_weapon_visual_effect` WHERE item_guid IN(SELECT guid FROM item_instance WHERE owner_guid = '{}')", player->GetGUID().GetCounter());

        if (!result)
            return;

        // Now we have query the DB we need to get the fields.
        do
        {
            Field* fields = result->Fetch();
            uint32 item_guid = fields[0].Get<uint32>();
            uint32 visual = fields[1].Get<uint32>();

            // Lets loop to check item by pos
            for (int i = EQUIPMENT_SLOT_MAINHAND; i <= EQUIPMENT_SLOT_OFFHAND; ++i)
            {
                pItem = player->GetItemByPos(255, i);

                if (pItem && pItem->GetGUID().GetCounter() == item_guid)
                {
                    player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (pItem->GetSlot() * 2), 0, visual);
                }
            }
        } while (result->NextRow());
    }

    // if Player has item in bag and re-equip it lets check for enchant
    void OnPlayerEquip(Player* player, Item* /*item*/, uint8 /*bag*/, uint8 /*slot*/, bool /*update*/) override
    {
        GetVisual(player);
    }

    void OnPlayerLogin(Player* player) override
    {
        GetVisual(player);

        if(sConfigMgr->GetOption<bool>("VisualWeapon.AnnounceEnable", true))
            ChatHandler(player->GetSession()).SendSysMessage("|cff4CFF00[GlacierWoW]|r Efectos visuales de armas disponibles. Busca a |cffFF8000Espectro|r en la casa de subastas.");
    }

    void OnPlayerLogout(Player* player) override
    {
        // Clean up preview state on logout
        uint64 guid = player->GetGUID().GetRawValue();
        playerPreviewState.erase(guid);
    }
};

class VisualWeaponWorld : public WorldScript
{
public:
    VisualWeaponWorld() : WorldScript("VisualWeaponWorld") {}

    void OnStartup() override
    {
        // Delete unused rows from DB table
        CharacterDatabase.DirectExecute("DELETE FROM `mod_weapon_visual_effect` WHERE NOT EXISTS(SELECT 1 FROM item_instance WHERE `mod_weapon_visual_effect`.item_guid = item_instance.guid)");
    }

};

void AddVisualWeaponScripts()
{
    new VisualWeaponPlayer();
    new VisualWeaponWorld();
    new VisualWeaponNPC();
}
