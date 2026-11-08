// Tools/DataTests/GameRulesTest.cpp
// The game's rules (Game/src/Rules: BattleRules, LootRules, NpcRules, ShopRules; OccluderRules, PartyRules, QuestRules, RouteRules) on fixed numbers: no scene, no data
// files, every random draw given. Built into DataRoundTripTest by scripts/test_data.sh.

#include "hypch.h"
#include "Rules/BattleRules.h"
#include "Rules/LootRules.h"
#include "Rules/OccluderRules.h"
#include "Rules/PartyRules.h"
#include "Rules/QuestRules.h"
#include "Rules/RouteRules.h"
#include "Rules/ShopRules.h"
#include "Rules/NpcRules.h"

#include <cstdio>
#include <deque>
#include <map>

namespace Hydro
{
	static int s_RuleFailures = 0;

	static void RuleCheck(bool condition, const char* what, int line, int& checks)
	{
		checks++;
		if (condition == false)
		{
			s_RuleFailures++;
			std::printf("FAIL  GameRulesTest.cpp line %d: %s\n", line, what);
		}
	}

#define RULE_CHECK(condition) RuleCheck((condition), #condition, __LINE__, checks)

	// Hands out the given rolls in order, then 0.
	static std::function<float()> Rolls(std::deque<float> values)
	{
		auto queue = std::make_shared<std::deque<float>>(std::move(values));
		return [queue]() -> float
		{
			if (queue->empty())
			{
				return 0.0f;
			}
			const float value = queue->front();
			queue->pop_front();
			return value;
		};
	}

	int TestGameRules(int& checks)
	{
		std::printf("Game rules: battle and loot\n");

		EnemyDefinition slime;
		slime.Weak = { DamageType::Pierce, DamageType::Fire };
		slime.Resist = { DamageType::Blunt };

		// stat 10 x power 1, spread roll 0.5 (x1.0), no critical (roll 0.5).
		BattleRules::HitResult plain = BattleRules::ResolveHit(slime, DamageType::Slash, 10.0f, 1.0f, false, 0.5f, 0.5f);
		RULE_CHECK(plain.Damage == 10 && plain.Weak == false && plain.Resisted == false && plain.Critical == false && plain.WeakSlot == -1);
		BattleRules::HitResult weak = BattleRules::ResolveHit(slime, DamageType::Fire, 10.0f, 1.0f, false, 0.5f, 0.5f);
		RULE_CHECK(weak.Damage == 15 && weak.Weak && weak.WeakSlot == 1);
		BattleRules::HitResult resisted = BattleRules::ResolveHit(slime, DamageType::Blunt, 10.0f, 1.0f, false, 0.5f, 0.5f);
		RULE_CHECK(resisted.Damage == 5 && resisted.Resisted);
		BattleRules::HitResult critical = BattleRules::ResolveHit(slime, DamageType::Slash, 10.0f, 1.0f, false, 0.0f, 0.5f);
		RULE_CHECK(critical.Critical && critical.Damage == 15);
		BattleRules::HitResult broken = BattleRules::ResolveHit(slime, DamageType::Fire, 10.0f, 1.0f, true, 0.5f, 0.5f);
		RULE_CHECK(broken.Damage == 30);
		RULE_CHECK(BattleRules::ResolveHit(slime, DamageType::Blunt, 0.1f, 1.0f, false, 0.5f, 0.0f).Damage == 1); // at least 1

		RULE_CHECK(BattleRules::ResolveHeal(1.5f, 10, 1.0f, 0.5f) == 15);
		RULE_CHECK(BattleRules::EnemyAttackDamage(10, 1.0f, false, 0.375f) == 10);
		RULE_CHECK(BattleRules::EnemyAttackDamage(10, 1.0f, true, 0.375f) == 5);
		RULE_CHECK(BattleRules::EnemyAttackDamage(1, 0.1f, true, 0.0f) == 1);

		RULE_CHECK(BattleRules::IsBroken(3, 3) && BattleRules::IsBroken(3, 4) == false && BattleRules::IsBroken(-1, 0) == false);

		// Speeds 9, 4, (fallen), 7 with no luck: 0, 3, 1. Luck can swap close ones: 4 + 3 beats 6 + 0.
		RULE_CHECK((BattleRules::TurnOrder({ 9, 4, -1, 7 }, Rolls({ 0.0f, 0.0f, 0.0f })) == std::vector<int>{ 0, 3, 1 }));
		RULE_CHECK((BattleRules::TurnOrder({ 6, 4 }, Rolls({ 0.0f, 1.0f })) == std::vector<int>{ 1, 0 }));
		// A full field: four heroes and three enemies, one fallen; the order covers everyone standing, fastest first.
		const std::vector<int> crowd = BattleRules::TurnOrder({ 9, 11, 5, 7, 4, -1, 10 }, Rolls({}));
		RULE_CHECK((crowd == std::vector<int>{ 1, 6, 0, 3, 2, 4 }));

		// Boosts, momentum and picks.
		RULE_CHECK(BattleRules::AttackHits(0) == 1 && BattleRules::AttackHits(3) == 4);
		RULE_CHECK(std::abs(BattleRules::BoostMultiplier(2) - 2.2f) < 1.0e-5f && BattleRules::BoostMultiplier(-1) == 1.0f);
		RULE_CHECK(BattleRules::SkillStat(DamageType::Fire, 10, 14) == 14.0f && BattleRules::SkillStat(DamageType::Slash, 10, 14) == 10.0f);
		RULE_CHECK(BattleRules::SkillStat(DamageType::Light, 12, 4) == 12.0f);
		RULE_CHECK(BattleRules::RegainMomentum(1, false, 1, 5) == 1);  // not in the first round
		RULE_CHECK(BattleRules::RegainMomentum(1, false, 2, 5) == 2);
		RULE_CHECK(BattleRules::RegainMomentum(3, true, 2, 5) == 3);   // boosted in the round before
		RULE_CHECK(BattleRules::RegainMomentum(5, false, 4, 5) == 5);  // full
		RULE_CHECK(BattleRules::PickByRoll(0, 0.5f) == -1 && BattleRules::PickByRoll(4, 0.0f) == 0);
		RULE_CHECK(BattleRules::PickByRoll(4, 0.99f) == 3 && BattleRules::PickByRoll(4, 1.0f) == 3 && BattleRules::PickByRoll(3, 0.5f) == 1);
		RULE_CHECK(BattleRules::ShareOf(15, 0.5f) == 8 && BattleRules::ShareOf(15, 0.0f) == 0 && BattleRules::ShareOf(15, 2.0f) == 15);

		EnemyDefinition caster;
		caster.BasicAttack = DataRowRef{ 1, "Attack" };
		caster.Abilities = { EnemyAbility{ DataRowRef{ 1, "Spark" }, 3.0f } };
		RULE_CHECK(BattleRules::ChooseEnemyAction(caster, 0.1f).Row == "Attack"); // 0.4 of 4 is the attack's 1
		RULE_CHECK(BattleRules::ChooseEnemyAction(caster, 0.9f).Row == "Spark");
		RULE_CHECK(BattleRules::ChooseEnemyAction(slime, 0.9f).IsSet() == false); // nothing set: the (empty) plain attack

		const std::vector<int> curve = { 30, 75, 135 };
		RULE_CHECK(BattleRules::LevelFor(curve, 0) == 1 && BattleRules::LevelFor(curve, 29) == 1 && BattleRules::LevelFor(curve, 30) == 2);
		RULE_CHECK(BattleRules::LevelFor(curve, 75) == 3 && BattleRules::LevelFor(curve, 9999) == 4); // the cap: 3 marks + 1

		HeroDefinition miko;
		miko.Base = CombatStats{ 90, 32, 10, 8, 9 };
		miko.GrowthPerLevel = CombatStats{ 9, 3, 2, 2, 0 };
		const CombatStats level3 = BattleRules::StatsAt(miko, 3);
		RULE_CHECK(level3.MaxHP == 108 && level3.MaxMP == 38 && level3.Attack == 14 && level3.Magic == 12 && level3.Speed == 9);
		RULE_CHECK(BattleRules::StatsAt(miko, 1).MaxHP == 90 && BattleRules::StatsAt(miko, 0).MaxHP == 90);

		// Loot: a rarity by share, then an item of it, then a count.
		LootTableDefinition table;
		table.DropChance = 50.0f;
		table.Rolls = 3;
		table.Rarities = { RarityChance{ ItemRarity::Common, 75.0f }, RarityChance{ ItemRarity::Rare, 25.0f }, RarityChance{ ItemRarity::Epic, 50.0f } };
		table.Items = { LootEntry{ DataRowRef{ 7, "Potion" }, 1, 3 }, LootEntry{ DataRowRef{ 7, "Herb" }, 1, 1 }, LootEntry{ DataRowRef{ 7, "Feather" }, 1, 1 } };
		const LootRules::RarityLookup rarity = [](const DataRowRef& item) { return item.Row == "Feather" ? ItemRarity::Rare : ItemRarity::Common; };
		// Roll 1: 0.2 drops; rarity 0.5 of 100 (Epic has no items, so Common 75 + Rare 25) -> Common; item 0.0 -> Potion;
		// count 0.99 -> 3. Roll 2: 0.9 doesn't drop. Roll 3: 0.1 drops; rarity 0.9 -> Rare; item 0.0 -> Feather.
		const std::vector<LootRules::Drop> drops = LootRules::Roll(table, Rolls({ 0.2f, 0.5f, 0.0f, 0.99f, 0.9f, 0.1f, 0.9f, 0.0f, 0.0f }), rarity);
		RULE_CHECK(drops.size() == 2);
		RULE_CHECK(drops.size() == 2 && drops[0].Item.Row == "Potion" && drops[0].Count == 3 && drops[1].Item.Row == "Feather" && drops[1].Count == 1);
		// The same item twice adds up.
		table.Rolls = 2;
		table.DropChance = 100.0f;
		const std::vector<LootRules::Drop> twice = LootRules::Roll(table, Rolls({ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }), rarity);
		RULE_CHECK(twice.size() == 1 && twice[0].Item.Row == "Potion" && twice[0].Count == 2);
		// No rarities: any item.
		table.Rarities.clear();
		table.Rolls = 1;
		RULE_CHECK(LootRules::Roll(table, Rolls({ 0.0f, 0.99f, 0.0f }), rarity)[0].Item.Row == "Feather");
		// Money (a chest's coins): none by default, then MinMoney to MaxMoney.
		RULE_CHECK(LootRules::RollMoney(table, Rolls({ 0.5f })) == 0);
		table.MinMoney = 20;
		table.MaxMoney = 40;
		RULE_CHECK(LootRules::RollMoney(table, Rolls({ 0.0f })) == 20 && LootRules::RollMoney(table, Rolls({ 0.999f })) == 40);
		table.MaxMoney = 20;
		RULE_CHECK(LootRules::RollMoney(table, Rolls({ 0.7f })) == 20);

		// Equipment (the Menu plan's M1): slots, swapping with the inventory.
		std::printf("Game rules: equipment and ability slots\n");
		RULE_CHECK(PartyRules::Fits(EquipSlot::Accessory, EquipPosition::Accessory2) && PartyRules::Fits(EquipSlot::Weapon, EquipPosition::Head) == false);
		RULE_CHECK(PartyRules::Fits(EquipSlot::None, EquipPosition::Weapon) == false);
		std::map<std::string, int> held = { { "Tekko", 1 }, { "Sword", 1 } };
		auto take = [&held](const DataRowRef& item) { if (held[item.Row] <= 0) { return false; } held[item.Row]--; return true; };
		auto give = [&held](const DataRowRef& item) { held[item.Row]++; };
		std::array<DataRowRef, 6> worn{};
		const DataRowRef tekko{ 7, "Tekko" };
		const DataRowRef sword{ 7, "Sword" };
		RULE_CHECK(PartyRules::Equip(worn, EquipPosition::Weapon, tekko, EquipSlot::Weapon, take, give) && worn[0] == tekko && held["Tekko"] == 0);
		RULE_CHECK(PartyRules::Equip(worn, EquipPosition::Head, sword, EquipSlot::Weapon, take, give) == false && held["Sword"] == 1); // doesn't fit
		RULE_CHECK(PartyRules::Equip(worn, EquipPosition::Weapon, sword, EquipSlot::Weapon, take, give) && worn[0] == sword && held["Tekko"] == 1 && held["Sword"] == 0);
		RULE_CHECK(PartyRules::Equip(worn, EquipPosition::Weapon, sword, EquipSlot::Weapon, take, give) && held["Sword"] == 0); // already worn
		RULE_CHECK(PartyRules::Equip(worn, EquipPosition::Weapon, DataRowRef{ 7, "Axe" }, EquipSlot::Weapon, take, give) == false && worn[0] == sword); // none held
		PartyRules::Unequip(worn, EquipPosition::Weapon, give);
		RULE_CHECK(worn[0].IsSet() == false && held["Sword"] == 1);
		const CombatStats sum = PartyRules::Add(CombatStats{ 90, 32, 10, 8, 9 }, CombatStats{ 20, 4, 3, 0, -1 });
		RULE_CHECK(sum.MaxHP == 110 && sum.MaxMP == 36 && sum.Attack == 13 && sum.Magic == 8 && sum.Speed == 8);

		// Ability slots: known ones only; a move swaps.
		const DataRowRef kick{ 1, "FlameKick" }, palm{ 1, "HolyPalm" }, lance{ 1, "SpiritLance" };
		std::array<DataRowRef, 8> slots{};
		slots[0] = kick;
		slots[1] = palm;
		const std::vector<DataRowRef> known = { kick, palm };
		RULE_CHECK(PartyRules::SetAbilitySlot(slots, known, 2, lance) == false && slots[2].IsSet() == false);
		RULE_CHECK(PartyRules::SetAbilitySlot(slots, known, 0, palm) && slots[0] == palm && slots[1] == kick);
		RULE_CHECK(PartyRules::SetAbilitySlot(slots, known, 5, kick) && slots[5] == kick && slots[1].IsSet() == false);
		RULE_CHECK(PartyRules::SetAbilitySlot(slots, known, 5, DataRowRef()) && slots[5].IsSet() == false);

		std::printf("Game rules: side quests\n");
		struct TestQuest
		{
			DataRowRef Definition;
			int Stage = 0;
			int Progress = 0;
			int Goal = 0;
			bool Done = false;
		};
		QuestDefinition hunt;
		hunt.Kind = QuestKind::Defeat;
		hunt.Enemy = DataRowRef{ 3, "Slime" };
		QuestDefinition bring;
		bring.Kind = QuestKind::Collect;
		bring.Item = DataRowRef{ 4, "SlimeJelly" };
		QuestDefinition story;
		std::map<std::string, const QuestDefinition*> book = { { "Hunt", &hunt }, { "Bring", &bring }, { "Story", &story } };
		auto lookup = [&book](const DataRowRef& row) -> const QuestDefinition*
		{
			auto found = book.find(row.Row);
			return found == book.end() ? nullptr : found->second;
		};
		std::vector<TestQuest> quests(3);
		quests[0].Definition = DataRowRef{ 9, "Hunt" };
		quests[0].Goal = 2;
		quests[1].Definition = DataRowRef{ 9, "Bring" };
		quests[1].Goal = 3;
		quests[2].Definition = DataRowRef{ 9, "Story" };
		RULE_CHECK(QuestRules::RecordDefeat(quests, "Wisp", lookup) == false && quests[0].Progress == 0);
		RULE_CHECK(QuestRules::RecordDefeat(quests, "Slime", lookup) && quests[0].Progress == 1 && quests[0].Stage == 0);
		RULE_CHECK(QuestRules::RecordDefeat(quests, "Slime", lookup) && quests[0].Progress == 2 && quests[0].Stage == 1 && QuestRules::IsReady(quests[0]));
		RULE_CHECK(QuestRules::RecordDefeat(quests, "Slime", lookup) == false && quests[0].Progress == 2); // no more than the goal
		int jelly = 2;
		auto countOf = [&jelly](const DataRowRef& item) { return item.Row == "SlimeJelly" ? jelly : 0; };
		RULE_CHECK(QuestRules::RefreshCollect(quests, lookup, countOf) && quests[1].Progress == 2 && quests[1].Stage == 0);
		jelly = 5;
		RULE_CHECK(QuestRules::RefreshCollect(quests, lookup, countOf) && quests[1].Progress == 3 && quests[1].Stage == 1 && QuestRules::IsReady(quests[1]));
		jelly = 1; // used meanwhile: back to collecting
		RULE_CHECK(QuestRules::RefreshCollect(quests, lookup, countOf) && quests[1].Progress == 1 && quests[1].Stage == 0);
		RULE_CHECK(QuestRules::RefreshCollect(quests, lookup, countOf) == false);
		quests[0].Done = true;
		auto find = [&quests](const std::string& row) -> const TestQuest*
		{
			for (const TestQuest& quest : quests)
			{
				if (quest.Definition.Row == row)
				{
					return &quest;
				}
			}
			return nullptr;
		};
		RULE_CHECK(QuestRules::CurrentFor({ "Hunt", "Bring" }, find) == "Bring");
		RULE_CHECK(QuestRules::CurrentFor({ "Hunt", "Later" }, find) == "Later");
		RULE_CHECK(QuestRules::CurrentFor({ "Hunt" }, find).empty());

		std::printf("Game rules: occluder fade and routes\n");
		// The camera south of and above the player: a box between them hides her, the ground she stands on, a box
		// behind her or beside the line and a wall right behind her (camera side) don't; a gate she stands under does.
		OccluderRules::Sight sight;
		sight.Camera = { 0.0f, -10.0f, 12.0f };
		sight.Feet = { 0.0f, 10.0f, 0.0f };
		RULE_CHECK(OccluderRules::Hides(sight, { -3.0f, -2.0f, 0.0f }, { 3.0f, 2.0f, 8.0f }));
		RULE_CHECK(OccluderRules::Hides(sight, { -30.0f, -30.0f, -1.0f }, { 30.0f, 30.0f, 0.0f }) == false);
		RULE_CHECK(OccluderRules::Hides(sight, { -3.0f, 12.0f, 0.0f }, { 3.0f, 16.0f, 8.0f }) == false);
		RULE_CHECK(OccluderRules::Hides(sight, { 5.0f, -2.0f, 0.0f }, { 8.0f, 2.0f, 8.0f }) == false);
		RULE_CHECK(OccluderRules::Hides(sight, { -3.0f, 10.3f, 0.0f }, { 3.0f, 11.0f, 8.0f }) == false);
		RULE_CHECK(OccluderRules::Hides(sight, { -3.0f, 8.0f, 0.0f }, { 3.0f, 12.0f, 6.0f })); // under a gate
		OccluderRules::Sight wallWalk; // the slice's wall walk behind the inn
		wallWalk.Camera = { 0.0f, 24.0f, 20.0f };
		wallWalk.Feet = { 0.0f, 36.0f, 11.5f };
		RULE_CHECK(OccluderRules::Hides(wallWalk, { -5.0f, 17.0f, 7.0f }, { 5.0f, 33.0f, 17.0f }));
		RULE_CHECK(OccluderRules::Hides(wallWalk, { -3.0f, 33.0f, 7.0f }, { 3.0f, 40.0f, 13.1f }) == false);    // the next wall section along the walk
		RULE_CHECK(OccluderRules::HidesCard(sight, { 0.0f, 7.0f, 0.0f }, 4.0f, 6.0f));            // a tree in front
		RULE_CHECK(OccluderRules::HidesCard(sight, { 0.0f, 13.0f, 0.0f }, 4.0f, 6.0f) == false);  // behind her
		RULE_CHECK(OccluderRules::HidesCard(sight, { 5.0f, 7.0f, 0.0f }, 2.0f, 6.0f) == false);   // beside the line
		RULE_CHECK(std::abs(OccluderRules::Approach(0.3f, 1.0f, 4.0f, 0.1f) - 0.7f) < 1e-4f);
		RULE_CHECK(OccluderRules::Approach(1.0f, 0.3f, 4.0f, 1.0f) == 0.3f);

		RouteRules::Step step = RouteRules::Next(2, 1, 3, RouteRules::Mode::Loop);
		RULE_CHECK(step.Index == 0 && step.Direction == 1);
		step = RouteRules::Next(2, 1, 3, RouteRules::Mode::PingPong);
		RULE_CHECK(step.Index == 1 && step.Direction == -1);
		step = RouteRules::Next(0, -1, 3, RouteRules::Mode::PingPong);
		RULE_CHECK(step.Index == 1 && step.Direction == 1);
		step = RouteRules::Next(2, 1, 3, RouteRules::Mode::Once);
		RULE_CHECK(step.Finished && step.Index == 2);
		step = RouteRules::Next(2, 1, 3, RouteRules::Mode::Restart);
		RULE_CHECK(step.Teleport && step.Index == 0);
		RULE_CHECK(RouteRules::Next(0, 1, 1, RouteRules::Mode::Loop).Finished);
		RULE_CHECK(RouteRules::ParseMode("PingPong") == RouteRules::Mode::PingPong && RouteRules::ParseMode("Bogus") == RouteRules::Mode::Loop);
		RULE_CHECK(RouteRules::FacingOf({ 1.0f, 0.2f }, "Down") == "Right" && RouteRules::FacingOf({ 0.0f, -1.0f }, "Up") == "Down");
		RULE_CHECK(RouteRules::FacingOf({ 0.0f, 0.0f }, "Left") == "Left");
		RULE_CHECK(RouteRules::RollWait(2.0f, 6.0f, 0.5f) == 4.0f && RouteRules::RollWait(6.0f, 2.0f, 0.0f) == 2.0f);
		RULE_CHECK(RouteRules::RollStop(1.0f, 0.99f) && RouteRules::RollStop(0.0f, 0.0f) == false);
		RULE_CHECK(RouteRules::IsInTheWay({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.8f, 0.1f, 0.0f }, 0.9f));
		RULE_CHECK(RouteRules::IsInTheWay({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { -0.8f, 0.0f, 0.0f }, 0.9f) == false);

		std::printf("Game rules: shop buy and sell\n");
		auto makeItem = [](int price, ItemKind kind)
		{
			ItemDefinition def{};
			def.Price = price;
			def.Kind = kind;
			return def;
		};

		DataRowRef potion{ 1, "Potion" };
		DataRowRef keyItem{ 1, "KeyRing" };
		DataRowRef ether{ 1, "Ether" };
		InventoryDefinition stock{};
		stock.Items.push_back(InventoryEntry{ potion, 1 });

		// Buy: exactly enough money -> money becomes 0.
		{
			Inventory inv{};
			inv.Money = 10;
			RULE_CHECK(ShopRules::Buy(inv, stock, potion, makeItem(5, ItemKind::Consumable), 2) == ShopRules::BuyResult::Bought && inv.Money == 0);
		}
		// Buy: not enough money -> nothing changes.
		{
			Inventory inv{};
			inv.Money = 9;
			RULE_CHECK(ShopRules::Buy(inv, stock, potion, makeItem(5, ItemKind::Consumable), 2) == ShopRules::BuyResult::NotEnoughMoney && inv.Money == 9);
		}
		// Buy: price 0 -> NotForSale.
		{
			Inventory inv{};
			inv.Money = 100;
			RULE_CHECK(ShopRules::Buy(inv, stock, potion, makeItem(0, ItemKind::Consumable), 1) == ShopRules::BuyResult::NotForSale);
		}
		// Buy: count < 1 -> BadCount.
		{
			Inventory inv{};
			inv.Money = 100;
			RULE_CHECK(ShopRules::Buy(inv, stock, potion, makeItem(5, ItemKind::Consumable), 0) == ShopRules::BuyResult::BadCount);
		}
		// Buy: already in the bag -> count grows.
		{
			Inventory inv{};
			inv.Money = 10;
			inv.Add(potion, 3);
			ShopRules::Buy(inv, stock, potion, makeItem(5, ItemKind::Consumable), 2);
			RULE_CHECK(inv.CountOf(potion) == 5 && inv.Money == 0);
		}

		// Sell: the last one.
		{
			Inventory inv{};
			inv.Money = 0;
			inv.Add(potion, 1);
			RULE_CHECK(ShopRules::Sell(inv, potion, makeItem(10, ItemKind::Consumable), 1) == ShopRules::SellResult::Sold && inv.Money == 5 && inv.CountOf(potion) == 0);
		}
		// Sell: more than you have -> NoneLeft, nothing changes.
		{
			Inventory inv{};
			inv.Money = 0;
			inv.Add(potion, 1);
			RULE_CHECK(ShopRules::Sell(inv, potion, makeItem(10, ItemKind::Consumable), 5) == ShopRules::SellResult::NoneLeft && inv.CountOf(potion) == 1 && inv.Money == 0);
		}
		// Sell: key item -> NotSellable.
		{
			Inventory inv{};
			inv.Money = 0;
			inv.Add(keyItem, 3);
			RULE_CHECK(ShopRules::Sell(inv, keyItem, makeItem(50, ItemKind::KeyItem), 1) == ShopRules::SellResult::NotSellable && inv.Money == 0);
		}
		// Sell: odd price (7) -> sells for 3 (rounded down).
		{
			Inventory inv{};
			inv.Money = 0;
			inv.Add(potion, 1);
			RULE_CHECK(ShopRules::Sell(inv, potion, makeItem(7, ItemKind::Consumable), 1) == ShopRules::SellResult::Sold && inv.Money == 3);
		}
		// Buy: more than a stack at once -> BadCount.
		{
			Inventory inv{};
			inv.Money = 100000;
			RULE_CHECK(ShopRules::Buy(inv, stock, potion, makeItem(5, ItemKind::Consumable), ShopRules::MaxStack + 1) == ShopRules::BuyResult::BadCount && inv.Money == 100000);
		}
		// Buy: would go past a full stack -> BagFull, nothing changes; up to exactly a full stack is fine.
		{
			Inventory inv{};
			inv.Money = 100;
			inv.Add(potion, ShopRules::MaxStack - 1);
			RULE_CHECK(ShopRules::Buy(inv, stock, potion, makeItem(5, ItemKind::Consumable), 2) == ShopRules::BuyResult::BagFull && inv.Money == 100);
			RULE_CHECK(inv.CountOf(potion) == ShopRules::MaxStack - 1);
			RULE_CHECK(ShopRules::Buy(inv, stock, potion, makeItem(5, ItemKind::Consumable), 1) == ShopRules::BuyResult::Bought && inv.CountOf(potion) == ShopRules::MaxStack);
		}
		// Sell: price 1 -> still sells for 1, not 0.
		{
			Inventory inv{};
			inv.Money = 0;
			inv.Add(potion, 2);
			RULE_CHECK(ShopRules::Sell(inv, potion, makeItem(1, ItemKind::Consumable), 2) == ShopRules::SellResult::Sold && inv.Money == 2);
		}
		// Sell: price 0 -> NotSellable.
		{
			Inventory inv{};
			inv.Money = 0;
			inv.Add(potion, 1);
			RULE_CHECK(ShopRules::Sell(inv, potion, makeItem(0, ItemKind::Consumable), 1) == ShopRules::SellResult::NotSellable && inv.Money == 0);
		}

		// Buy: not in this shop's stock -> NotForSale, nothing changes.
		{
			Inventory inv{};
			inv.Money = 100;
			RULE_CHECK(ShopRules::Buy(inv, stock, ether, makeItem(5, ItemKind::Consumable), 1) == ShopRules::BuyResult::NotForSale);
			RULE_CHECK(inv.Money == 100 && inv.CountOf(ether) == 0);
			RULE_CHECK(ShopRules::InStock(stock, potion) && ShopRules::InStock(stock, ether) == false);
			RULE_CHECK(ShopRules::InStock(InventoryDefinition{}, potion) == false);
		}
		// CanSell: priced and not a key item; Sell agrees with it.
		RULE_CHECK(ShopRules::CanSell(makeItem(10, ItemKind::Consumable)) && ShopRules::CanSell(makeItem(10, ItemKind::Equipment)));
		RULE_CHECK(ShopRules::CanSell(makeItem(0, ItemKind::Material)) == false && ShopRules::CanSell(makeItem(50, ItemKind::KeyItem)) == false);

		std::printf("Game rules: NPC choices\n");
		{
			RULE_CHECK(NpcRules::Options(false, false, false).empty());
			const std::vector<NpcOption> talk = NpcRules::Options(true, false, false);
			RULE_CHECK(talk.size() == 1 && talk[0] == NpcOption::Talk);
			const std::vector<NpcOption> quest = NpcRules::Options(false, true, false);
			RULE_CHECK(quest.size() == 1 && quest[0] == NpcOption::Quest);
			const std::vector<NpcOption> shop = NpcRules::Options(false, false, true);
			RULE_CHECK(shop.size() == 1 && shop[0] == NpcOption::Shop);
			const std::vector<NpcOption> talkShop = NpcRules::Options(true, false, true);
			RULE_CHECK(talkShop == std::vector<NpcOption>({ NpcOption::Talk, NpcOption::Shop, NpcOption::Leave }));
			const std::vector<NpcOption> all = NpcRules::Options(true, true, true);
			RULE_CHECK(all == std::vector<NpcOption>({ NpcOption::Talk, NpcOption::Quest, NpcOption::Shop, NpcOption::Leave }));
			const std::vector<NpcOption> questShop = NpcRules::Options(false, true, true);
			RULE_CHECK(questShop == std::vector<NpcOption>({ NpcOption::Quest, NpcOption::Shop, NpcOption::Leave }));
		}

		return s_RuleFailures;
	}
}
