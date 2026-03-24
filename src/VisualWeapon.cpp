/*
** Made by Rochet2(Eluna)
** Rewritten by Poszer & Talamortis https://github.com/poszer/ & https://github.com/talamortis/
** AzerothCore 2019 http://www.azerothcore.org/
** Cleaned and made into a module by Micrah https://github.com/milestorme/
** Modified by GlacierWoW: Spanish translation, per-enchant gold cost, timed preview system
*/

#include "ScriptMgr.h"
#include "Player.h"
#include "Chat.h"
#include "ScriptedGossip.h"
#include "Config.h"
#include "EventProcessor.h"

using namespace std;

enum VisualWeaponsGossip
{
    VIS_DEFAULT_MESSAGE             = 907,
    VIS_GOSSIP_MAIN_MENU_ACTION     = 100,
    VIS_GOSSIP_MAIN_HAND_ACTION     = 200,
    VIS_GOSSIP_OFF_HAND_ACTION      = 300,
    VIS_GOSSIP_CLOSE_ACTION         = 400,
    VIS_GOSSIP_REMOVE_MAIN_ACTION   = 500,
    VIS_GOSSIP_REMOVE_OFF_ACTION    = 501,
    VIS_GOSSIP_CONFIRM_ACTION       = 600,
    VIS_GOSSIP_CANCEL_ACTION        = 700
};

enum EnchantTier
{
    TIER_COMMON   = 0,
    TIER_UNCOMMON = 1,
    TIER_RARE     = 2,
    TIER_EPIC     = 3
};

struct VisualData
{
    uint32 Menu;
    uint32 Submenu;
    uint32 Icon;
    uint32 Id;
    string Name;
    uint8 Tier;
};

static string GOLD_ICON = "|TInterface/MoneyFrame/UI-GoldIcon:12:12:2:0|t";

static uint32 GetEnchantCostGold(uint32 enchantId, uint8 tier)
{
    string configKey = "VisualWeapon.Cost.Enchant." + to_string(enchantId);
    int32 override_cost = sConfigMgr->GetOption<int32>(configKey, -1);
    if (override_cost >= 0)
        return static_cast<uint32>(override_cost);

    switch (tier)
    {
        case TIER_COMMON:   return sConfigMgr->GetOption<uint32>("VisualWeapon.Cost.Common", 10);
        case TIER_UNCOMMON: return sConfigMgr->GetOption<uint32>("VisualWeapon.Cost.Uncommon", 30);
        case TIER_RARE:     return sConfigMgr->GetOption<uint32>("VisualWeapon.Cost.Rare", 75);
        case TIER_EPIC:     return sConfigMgr->GetOption<uint32>("VisualWeapon.Cost.Epic", 150);
        default:            return 50;
    }
}

static uint32 GetEnchantCostCopper(uint32 enchantId, uint8 tier)
{
    return GetEnchantCostGold(enchantId, tier) * 10000;
}

static string GetTierColor(uint8 tier)
{
    switch (tier)
    {
        case TIER_COMMON:   return "|cff9d9d9d";
        case TIER_UNCOMMON: return "|cff1eff00";
        case TIER_RARE:     return "|cff0070dd";
        case TIER_EPIC:     return "|cffa335ee";
        default:            return "|cffffffff";
    }
}

VisualData vData[] =
{
    // Page 1
    { 1, VIS_GOSSIP_MAIN_MENU_ACTION, GOSSIP_ICON_TALK, 0, "Volver..", 0 },
    { 1, 2, GOSSIP_ICON_INTERACT_1, 0, "Siguiente..", 0 },
    { 1, 0, GOSSIP_ICON_BATTLE, 3789, "Rabiar",                     TIER_EPIC },
    { 1, 0, GOSSIP_ICON_BATTLE, 3854, "Poder Arcano",               TIER_RARE },
    { 1, 0, GOSSIP_ICON_BATTLE, 3273, "Escarcha Mortal",            TIER_EPIC },
    { 1, 0, GOSSIP_ICON_BATTLE, 3225, "Verdugo",                    TIER_EPIC },
    { 1, 0, GOSSIP_ICON_BATTLE, 3870, "Drenar Sangre",              TIER_RARE },
    { 1, 0, GOSSIP_ICON_BATTLE, 1899, "Arma Profana",               TIER_RARE },
    { 1, 0, GOSSIP_ICON_BATTLE, 2674, "Oleada Arcana",              TIER_UNCOMMON },
    { 1, 0, GOSSIP_ICON_BATTLE, 2675, "Maestro de Batalla",         TIER_UNCOMMON },
    { 1, 0, GOSSIP_ICON_BATTLE, 2671, "Poder Arcano y Fuego",       TIER_RARE },
    { 1, 0, GOSSIP_ICON_BATTLE, 2672, "Poder de Sombras y Escarcha",TIER_RARE },
    { 1, 0, GOSSIP_ICON_BATTLE, 3365, "Runa de Rompe-espadas",      TIER_UNCOMMON },
    { 1, 0, GOSSIP_ICON_BATTLE, 2673, "Mangosta",                   TIER_EPIC },
    { 1, 0, GOSSIP_ICON_BATTLE, 2343, "Poder con Hechizos",         TIER_UNCOMMON },

    // Page 2
    { 2, VIS_GOSSIP_MAIN_MENU_ACTION, GOSSIP_ICON_TALK, 0, "Volver..", 0 },
    { 2, 3, GOSSIP_ICON_INTERACT_1, 0, "Siguiente..", 0 },
    { 2, 1, GOSSIP_ICON_INTERACT_1, 0, "Anterior..", 0 },
    { 2, 0, GOSSIP_ICON_BATTLE, 425,  "Templo Oscuro",              TIER_EPIC },
    { 2, 0, GOSSIP_ICON_BATTLE, 3855, "Poder con Hechizos III",     TIER_RARE },
    { 2, 0, GOSSIP_ICON_BATTLE, 1894, "Arma Helada",                TIER_UNCOMMON },
    { 2, 0, GOSSIP_ICON_BATTLE, 1103, "Agilidad",                   TIER_COMMON },
    { 2, 0, GOSSIP_ICON_BATTLE, 1898, "Robar Vida",                 TIER_RARE },
    { 2, 0, GOSSIP_ICON_BATTLE, 3345, "Tierra Viva",                TIER_UNCOMMON },
    { 2, 0, GOSSIP_ICON_BATTLE, 1743, "Hoja Espectral",             TIER_COMMON },
    { 2, 0, GOSSIP_ICON_BATTLE, 3093, "Poder vs No-muertos",        TIER_UNCOMMON },
    { 2, 0, GOSSIP_ICON_BATTLE, 1900, "Cruzado",                    TIER_EPIC },
    { 2, 0, GOSSIP_ICON_BATTLE, 3846, "Poder con Hechizos II",      TIER_RARE },
    { 2, 0, GOSSIP_ICON_BATTLE, 1606, "Poder de Ataque",            TIER_COMMON },
    { 2, 0, GOSSIP_ICON_BATTLE, 283,  "Furia de Viento",            TIER_UNCOMMON },
    { 2, 0, GOSSIP_ICON_BATTLE, 1,    "Golpe de Roca",              TIER_COMMON },

    // Page 3
    { 3, VIS_GOSSIP_MAIN_MENU_ACTION, GOSSIP_ICON_TALK, 0, "Volver..", 0 },
    { 3, 2, GOSSIP_ICON_INTERACT_1, 0, "Anterior..", 0 },
    { 3, 0, GOSSIP_ICON_BATTLE, 3265, "Aceite Bendito",             TIER_UNCOMMON },
    { 3, 0, GOSSIP_ICON_BATTLE, 2,    "Marca de Escarcha",          TIER_COMMON },
    { 3, 0, GOSSIP_ICON_BATTLE, 3,    "Lengua de Fuego",            TIER_COMMON },
    { 3, 0, GOSSIP_ICON_BATTLE, 3266, "Aceite Sagrado",             TIER_UNCOMMON },
    { 3, 0, GOSSIP_ICON_BATTLE, 1903, "Espiritu",                   TIER_COMMON },
    { 3, 0, GOSSIP_ICON_BATTLE, 13,   "Afilado",                    TIER_COMMON },
    { 3, 0, GOSSIP_ICON_BATTLE, 26,   "Aceite de Escarcha",         TIER_COMMON },
    { 3, 0, GOSSIP_ICON_BATTLE, 7,    "Veneno Mortal",              TIER_UNCOMMON },
    { 3, 0, GOSSIP_ICON_BATTLE, 803,  "Arma Ardiente",              TIER_RARE },
    { 3, 0, GOSSIP_ICON_BATTLE, 1896, "Dano de Arma",               TIER_COMMON },
    { 3, 0, GOSSIP_ICON_BATTLE, 2666, "Intelecto",                  TIER_COMMON },
    { 3, 0, GOSSIP_ICON_BATTLE, 25,   "Aceite de Sombras",          TIER_COMMON },
};

// Per-player preview state
struct PreviewState
{
    uint32 VisualId;
    uint32 EnchantId;
    uint32 OriginalVisual;
    string EnchantName;
    uint8 Tier;
    bool MainHand;
    bool Active;
    bool Confirmed;
    uint32 PreviewToken; // to match timed revert with current preview
};

static std::unordered_map<uint64, PreviewState> playerPreviewState;
static uint32 previewTokenCounter = 0;

static void RevertPreview(Player* player)
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

// Timed event to auto-revert preview
class PreviewRevertEvent : public BasicEvent
{
public:
    PreviewRevertEvent(ObjectGuid playerGuid, uint32 token)
        : _playerGuid(playerGuid), _token(token) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        Player* player = ObjectAccessor::FindPlayer(_playerGuid);
        if (!player)
            return true;

        uint64 guid = player->GetGUID().GetRawValue();
        auto it = playerPreviewState.find(guid);
        if (it == playerPreviewState.end())
            return true;

        PreviewState& state = it->second;

        // Only revert if this token matches (hasn't been replaced by another preview)
        // and hasn't been confirmed
        if (state.Active && state.PreviewToken == _token && !state.Confirmed)
        {
            uint8 slot = state.MainHand ? EQUIPMENT_SLOT_MAINHAND : EQUIPMENT_SLOT_OFFHAND;
            Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);

            if (item)
                player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (item->GetSlot() * 2), 0, state.OriginalVisual);

            state.Active = false;

            ChatHandler(player->GetSession()).SendSysMessage("|cffAAAAAA[Preview expirado] El efecto visual fue revertido.|r");
        }

        return true;
    }

private:
    ObjectGuid _playerGuid;
    uint32 _token;
};

class VisualWeaponNPC : public CreatureScript
{
public:
    VisualWeaponNPC() : CreatureScript("npc_visualweapon") { }

    bool MainHand;

    bool IsValidWeapon(Player* player, uint8 slot)
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            return false;

        const ItemTemplate* itemTemplate = item->GetTemplate();
        if (itemTemplate->Class != ITEM_CLASS_WEAPON)
            return false;

        if (itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_BOW ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_GUN ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_obsolete ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_THROWN ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_SPEAR ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_CROSSBOW ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_WAND ||
            itemTemplate->SubClass == ITEM_SUBCLASS_WEAPON_FISHING_POLE)
            return false;

        return true;
    }

    void ApplyPreview(Player* player, uint32 visual_id, uint32 enchantId, uint8 tier, const string& enchantName)
    {
        uint8 slot = MainHand ? EQUIPMENT_SLOT_MAINHAND : EQUIPMENT_SLOT_OFFHAND;

        if (!IsValidWeapon(player, slot))
        {
            string msg = "|cffFF0000No tienes un arma valida en la " + string(MainHand ? "mano principal" : "mano secundaria") + ".|r";
            ChatHandler(player->GetSession()).SendSysMessage(msg.c_str());
            return;
        }

        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);

        uint64 guid = player->GetGUID().GetRawValue();
        PreviewState& state = playerPreviewState[guid];

        // If switching preview, restore original first
        if (state.Active)
        {
            uint8 prevSlot = state.MainHand ? EQUIPMENT_SLOT_MAINHAND : EQUIPMENT_SLOT_OFFHAND;
            Item* prevItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, prevSlot);
            if (prevItem)
                player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (prevItem->GetSlot() * 2), 0, state.OriginalVisual);
        }

        // Save original visual
        state.OriginalVisual = player->GetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (item->GetSlot() * 2), 0);
        state.VisualId = visual_id;
        state.EnchantId = enchantId;
        state.MainHand = MainHand;
        state.Tier = tier;
        state.EnchantName = enchantName;
        state.Active = true;
        state.Confirmed = false;
        state.PreviewToken = ++previewTokenCounter;

        // Apply visual preview
        player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (item->GetSlot() * 2), 0, visual_id);

        // Schedule auto-revert after 15 seconds
        uint32 previewDuration = sConfigMgr->GetOption<uint32>("VisualWeapon.PreviewDuration", 15) * IN_MILLISECONDS;
        player->m_Events.AddEvent(new PreviewRevertEvent(player->GetGUID(), state.PreviewToken), player->m_Events.CalculateTime(previewDuration));
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

        uint32 cost = GetEnchantCostCopper(state.EnchantId, state.Tier);
        uint32 costGold = GetEnchantCostGold(state.EnchantId, state.Tier);

        if (cost > 0 && !player->HasEnoughMoney(cost))
        {
            string msg = "|cffFF0000No tienes suficiente oro. Necesitas " + to_string(costGold) + GOLD_ICON + ".|r";
            ChatHandler(player->GetSession()).SendSysMessage(msg.c_str());
            player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (item->GetSlot() * 2), 0, state.OriginalVisual);
            state.Active = false;
            return;
        }

        if (cost > 0)
            player->ModifyMoney(-static_cast<int32>(cost));

        // Mark as confirmed so the timed revert won't touch it
        state.Confirmed = true;
        state.Active = false;

        CharacterDatabase.Execute("REPLACE into `mod_weapon_visual_effect` (`item_guid`, `enchant_visual_id`) VALUES ('{}', '{}')", item->GetGUID().GetCounter(), state.VisualId);

        string msg = "|cff00FF00Efecto visual aplicado! Costo: " + to_string(costGold) + GOLD_ICON + "|r";
        ChatHandler(player->GetSession()).SendSysMessage(msg.c_str());
    }

    void RemoveVisual(Player* player, bool isMainHand)
    {
        uint8 slot = isMainHand ? EQUIPMENT_SLOT_MAINHAND : EQUIPMENT_SLOT_OFFHAND;
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        string handName = isMainHand ? "mano principal" : "mano secundaria";

        if (!item)
        {
            string msg = "|cffFF0000No tienes un arma equipada en la " + handName + ".|r";
            ChatHandler(player->GetSession()).SendSysMessage(msg.c_str());
            return;
        }

        player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (item->GetSlot() * 2), 0, 0);
        CharacterDatabase.Execute("DELETE FROM `mod_weapon_visual_effect` WHERE `item_guid` = '{}'", item->GetGUID().GetCounter());

        string msg = "|cff00FF00Efecto visual de " + handName + " removido.|r";
        ChatHandler(player->GetSession()).SendSysMessage(msg.c_str());
    }

    void GetMenu(Player* player, Creature* creature, uint32 menuId)
    {
        string handName = MainHand ? "Mano Principal" : "Mano Secundaria";
        uint32 previewSec = sConfigMgr->GetOption<uint32>("VisualWeapon.PreviewDuration", 15);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|cff00CCFF" + handName + "|r  |cff9d9d9dComun|r |cff1eff00Poco comun|r |cff0070ddRaro|r |cffa335eeEpico|r", GOSSIP_SENDER_MAIN, VIS_GOSSIP_MAIN_MENU_ACTION);

        for (uint8 i = 0; i < (sizeof(vData) / sizeof(*vData)); i++)
        {
            if (vData[i].Menu == menuId)
            {
                if (vData[i].Submenu == 0 && vData[i].Id != 0)
                {
                    uint32 costGold = GetEnchantCostGold(vData[i].Id, vData[i].Tier);
                    string tierColor = GetTierColor(vData[i].Tier);
                    string entry = tierColor + vData[i].Name + "|r - " + to_string(costGold) + GOLD_ICON;
                    AddGossipItemFor(player, vData[i].Icon, entry, GOSSIP_SENDER_MAIN, i);
                }
                else
                    AddGossipItemFor(player, vData[i].Icon, vData[i].Name, GOSSIP_SENDER_MAIN, i);
            }
        }

        player->PlayerTalkClass->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void GetConfirmMenu(Player* player, Creature* creature)
    {
        uint64 guid = player->GetGUID().GetRawValue();
        auto it = playerPreviewState.find(guid);
        if (it == playerPreviewState.end() || !it->second.Active)
            return;

        PreviewState& state = it->second;
        uint32 costGold = GetEnchantCostGold(state.EnchantId, state.Tier);
        string tierColor = GetTierColor(state.Tier);
        string handName = state.MainHand ? "Mano Principal" : "Mano Secundaria";
        uint32 previewSec = sConfigMgr->GetOption<uint32>("VisualWeapon.PreviewDuration", 15);

        // Info header
        string previewInfo = "|cffFFFF00[Preview " + to_string(previewSec) + "s]|r " + tierColor + state.EnchantName + "|r - " + handName;
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, previewInfo, GOSSIP_SENDER_MAIN, VIS_GOSSIP_CANCEL_ACTION);

        // Confirm
        string confirmText = "|TInterface/ICONS/Spell_ChargePositive:20:20|t |cff00FF00Confirmar|r - " + to_string(costGold) + GOLD_ICON;
        AddGossipItemFor(player, GOSSIP_ICON_BATTLE, confirmText, GOSSIP_SENDER_MAIN, VIS_GOSSIP_CONFIRM_ACTION);

        // Cancel
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/Spell_ChargeNegative:20:20|t |cffFF0000Cancelar|r", GOSSIP_SENDER_MAIN, VIS_GOSSIP_CANCEL_ACTION);

        player->PlayerTalkClass->SendGossipMenu(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }

    void GetMainMenu(Player* player, Creature* creature)
    {
        bool hasMainHand = IsValidWeapon(player, EQUIPMENT_SLOT_MAINHAND);
        bool hasOffHand = IsValidWeapon(player, EQUIPMENT_SLOT_OFFHAND);

        if (hasMainHand)
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|TInterface/PaperDoll/UI-PaperDoll-Slot-MainHand:40:40:-18|tMano Principal", GOSSIP_SENDER_MAIN, VIS_GOSSIP_MAIN_HAND_ACTION);
        else
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|TInterface/PaperDoll/UI-PaperDoll-Slot-MainHand:40:40:-18|t|cff9d9d9dMano Principal (sin arma)|r", GOSSIP_SENDER_MAIN, VIS_GOSSIP_CLOSE_ACTION);

        if (hasOffHand)
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|TInterface/PaperDoll/UI-PaperDoll-Slot-SecondaryHand:40:40:-18|tMano Secundaria", GOSSIP_SENDER_MAIN, VIS_GOSSIP_OFF_HAND_ACTION);
        else
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|TInterface/PaperDoll/UI-PaperDoll-Slot-SecondaryHand:40:40:-18|t|cff9d9d9dMano Secundaria (sin arma)|r", GOSSIP_SENDER_MAIN, VIS_GOSSIP_CLOSE_ACTION);

        if (hasMainHand)
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|TInterface/ICONS/INV_Enchant_Disenchant:20:20:-4|t|cffFF4444Quitar efecto - Mano Principal|r", GOSSIP_SENDER_MAIN, VIS_GOSSIP_REMOVE_MAIN_ACTION);
        if (hasOffHand)
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|TInterface/ICONS/INV_Enchant_Disenchant:20:20:-4|t|cffFF4444Quitar efecto - Mano Secundaria|r", GOSSIP_SENDER_MAIN, VIS_GOSSIP_REMOVE_OFF_ACTION);

        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|TInterface/PaperDollInfoFrame/UI-GearManager-Undo:40:40:-18|tCerrar", GOSSIP_SENDER_MAIN, VIS_GOSSIP_CLOSE_ACTION);

        player->PlayerTalkClass->SendGossipMenu(VIS_DEFAULT_MESSAGE, creature->GetGUID());
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        RevertPreview(player);
        GetMainMenu(player, creature);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();

        switch (action)
        {
            case VIS_GOSSIP_MAIN_HAND_ACTION:
                RevertPreview(player);
                MainHand = true;
                GetMenu(player, creature, 1);
                return true;

            case VIS_GOSSIP_OFF_HAND_ACTION:
                RevertPreview(player);
                MainHand = false;
                GetMenu(player, creature, 1);
                return true;

            case VIS_GOSSIP_CLOSE_ACTION:
                RevertPreview(player);
                CloseGossipMenuFor(player);
                return false;

            case VIS_GOSSIP_REMOVE_MAIN_ACTION:
                RevertPreview(player);
                RemoveVisual(player, true);
                CloseGossipMenuFor(player);
                return true;

            case VIS_GOSSIP_REMOVE_OFF_ACTION:
                RevertPreview(player);
                RemoveVisual(player, false);
                CloseGossipMenuFor(player);
                return true;

            case VIS_GOSSIP_CONFIRM_ACTION:
                ConfirmVisual(player);
                CloseGossipMenuFor(player);
                return true;

            case VIS_GOSSIP_CANCEL_ACTION:
                RevertPreview(player);
                GetMainMenu(player, creature);
                return true;
        }

        uint32 menuData = vData[action].Submenu;

        if (menuData == VIS_GOSSIP_MAIN_MENU_ACTION)
        {
            RevertPreview(player);
            GetMainMenu(player, creature);
            return true;
        }
        else if (menuData == 0)
        {
            ApplyPreview(player, vData[action].Id, vData[action].Id, vData[action].Tier, vData[action].Name);
            GetConfirmMenu(player, creature);
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
        CharacterDatabase.Execute("DELETE FROM `mod_weapon_visual_effect` WHERE NOT EXISTS(SELECT 1 FROM item_instance WHERE `mod_weapon_visual_effect`.item_guid = item_instance.guid)");
    }

    void GetVisual(Player* player)
    {
        if (!player)
            return;

        Item* pItem;
        QueryResult result = CharacterDatabase.Query("SELECT item_guid, enchant_visual_id FROM `mod_weapon_visual_effect` WHERE item_guid IN(SELECT guid FROM item_instance WHERE owner_guid = '{}')", player->GetGUID().GetCounter());

        if (!result)
            return;

        do
        {
            Field* fields = result->Fetch();
            uint32 item_guid = fields[0].Get<uint32>();
            uint32 visual = fields[1].Get<uint32>();

            for (int i = EQUIPMENT_SLOT_MAINHAND; i <= EQUIPMENT_SLOT_OFFHAND; ++i)
            {
                pItem = player->GetItemByPos(255, i);
                if (pItem && pItem->GetGUID().GetCounter() == item_guid)
                    player->SetUInt16Value(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + (pItem->GetSlot() * 2), 0, visual);
            }
        } while (result->NextRow());
    }

    void OnPlayerEquip(Player* player, Item* /*item*/, uint8 /*bag*/, uint8 /*slot*/, bool /*update*/) override
    {
        RevertPreview(player);
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
        uint64 guid = player->GetGUID().GetRawValue();
        playerPreviewState.erase(guid);
    }

    void OnPlayerUpdateZone(Player* player, uint32 /*newZone*/, uint32 /*newArea*/) override
    {
        RevertPreview(player);
        GetVisual(player);
    }
};

class VisualWeaponWorld : public WorldScript
{
public:
    VisualWeaponWorld() : WorldScript("VisualWeaponWorld") {}

    void OnStartup() override
    {
        CharacterDatabase.DirectExecute("DELETE FROM `mod_weapon_visual_effect` WHERE NOT EXISTS(SELECT 1 FROM item_instance WHERE `mod_weapon_visual_effect`.item_guid = item_instance.guid)");
    }
};

void AddVisualWeaponScripts()
{
    new VisualWeaponPlayer();
    new VisualWeaponWorld();
    new VisualWeaponNPC();
}
