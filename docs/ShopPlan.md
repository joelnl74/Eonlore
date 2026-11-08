# Shop and NPC choices: plan

Talking to an NPC who has more than one thing to offer opens a small **choice menu** (Talk / Quest / Buy & Sell / Leave).
Buy & Sell opens a **shop screen** of its own. An NPC with only lines to say keeps today's behaviour: E opens the dialogue
directly.

**Status (Nov 9, 2026):** Q1–Q7, C1 and C2 are done (rules and tests pass, the views and director compile, the scenes and the
`FieldUI` prefab are rebuilt). Left: **C3**, testing in Play, and the data in §6.

## 0. What already exists (don't rebuild it)

| Thing | Where | Notes |
| --- | --- | --- |
| Buy/sell rules | `Rules/ShopRules.h` | `Buy(inventory, row, item, count)`, `Sell(inventory, row, item, count)`, `SellPrice(item)`, `MaxStack = 99`. Results: `Bought, NotForSale, NotEnoughMoney, BadCount, BagFull` / `Sold, NotSellable, NoneLeft, BadCount` |
| Rule tests | `Tools/Tests/GameRulesTest.cpp`, "Game rules: shop buy and sell" | 15 checks |
| Prices | `ItemDefinition::Price`, `Items.hytable` | Set for every item; key items 0 |
| A merchant's goods | `NpcDefinition::Shop` (`Data/FieldDefinitions.h`) → `Assets/Data/Shops/*.hydef` | Each is an `InventoryDefinition`: `Items` (`{Item, Count}`) and `Money`. Blacksmith, PotteryMerchant, SakeMerchant, PaperMerchant |
| Money | `Inventory::Money`, `CurrencyName` | Both in `State/Inventory.h`. Already saved |
| Equipped gear | `PartyMember::Equipment` | Not in the inventory, so it can never be sold by accident |
| Quest conversations | `QuestLog::Claim(npc)`, `Release`, `Talk(npc)`, `TakeTalk(npc)` (`State/QuestLog.h`) | A quest script claims an NPC; `Talk` hands the conversation to it; the quest script picks it up with `TakeTalk` |
| NPC interaction | `FieldDirectorScript::UpdateInteraction`, `case Kind::Npc` | Today: `QuestLog::Get().Talk(...)` first, else the NPC row's dialogue through `DialogueScript::Open` |
| Menu pattern to copy | `InventoryView` + `FieldDirectorScript::UseSelected` | The view turns UI events into a `Request`; the **director** calls the rule, changes state, then calls the view's `Present()` |
| Field UI text | `FieldLabel` + `GetFieldText` (top of `FieldDirectorScript.h`) | Add labels there; no strings in views |

## 1. Decisions (already made)

1. **The shop's stock never runs out** (v1). A shop file's `Count` and `Money` are ignored for shops. Limited stock would
   need saving per shop. It can come later.
2. **The shop is its own screen**, `Shop Canvas`, not a tab of the field menu. Adding a `Tab::Shop` would put a Shop tab in
   the menu's tab row (`FieldMenuView` loops over `TabCount`) and Esc would land in Journal/Inventory.
3. **Views never change state.** `ShopView` and `NpcChoiceView` only show things and return a `Request`. The director
   calls `ShopRules` and changes the inventory.
4. **Keyboard-first, like every other menu:** one list of rows, Enter acts on the selected row, Esc goes back or closes.
   No Buy/Sell buttons per row.
5. **The choice menu shows only what this NPC offers**, in this order:
   - **Talk:** when the NPC row has dialogue lines.
   - **Quest:** when a quest has claimed this NPC.
   - **Buy & Sell:** when the NPC row has a `Shop`.
   - **Leave:** always.

   With only one real option (no quest, no shop), there's no menu: E goes straight to it, as today.

## 2. Screens and their tags

Built by `scripts/game/ui/build_game_ui.py` into every field scene **and** the `FieldUI` prefab (run with
`field prefab`). Both are under the director, so their events reach `FieldDirectorScript::OnUIEvent`.

**Choice Canvas** (new; captures input; hidden by default; small panel near the bottom, like the dialogue box):

| Tag | Purpose |
| --- | --- |
| `Choice Canvas` | The screen |
| `Choice Title` | The NPC's name |
| `Choice 0` … `Choice 3` | Rows (buttons); unused rows hidden |
| `Choice 0 Name` … `Choice 3 Name` | The row's label (rows are `ribbon_row`s, which name their label "Name") |

**Shop Canvas** (new; captures input; hidden by default; the same frame art as the field menu pages):

| Tag | Purpose |
| --- | --- |
| `Shop Canvas` | The screen |
| `Shop Title` | The merchant's name |
| `Shop Mode Buy`, `Shop Mode Sell` | The two mode headers; the active one is highlighted |
| `Shop Money Label`, `Shop Money Amount` | `CurrencyName` and `Inventory::Money` |
| `Shop Row 0` … `Shop Row 9` | Rows (buttons) |
| `Shop Row <n> Name`, `Shop Row <n> Price`, `Shop Row <n> Count` | Item name; buy price or sell price; how many the party holds |
| `Shop Detail Name`, `Shop Detail Text` | The selected item's name and description |
| `Shop Help` | Key help ("Enter  Buy / sell one   Left / Right  Buy or sell   Esc  Leave"; the fonts have no arrow glyphs) |

## 3. How it plays

1. Miko walks to `Npc PotteryMerchant` and presses E.
2. The director builds the options (rule `NpcRules::Options`, step Q2) from: the NPC row has lines; `QuestLog::IsClaimed`
   (step Q1); the NPC row has a `Shop`.
3. One option: do it at once (today's behaviour). Two or more: open `Choice Canvas` with the NPC's name and the options.
4. The player picks:
   - **Talk:** open the dialogue as today.
   - **Quest:** `QuestLog::Get().Talk(npc)`. The quest script takes over through `TakeTalk`, exactly as now.
   - **Buy & Sell:** open `Shop Canvas` in Buy mode.
   - **Leave**, or **Esc:** close; walking resumes.
5. In the shop:
   - Left/Right switches Buy ↔ Sell.
   - Buy mode lists the shop's goods. Sell mode lists the party's items that are sellable (`Price > 0`, not `KeyItem`, count > 0).
   - Enter buys or sells **one** of the selected item. The director calls `ShopRules`, shows a message for every result
     (step Q4), and refreshes the shop.
   - Esc closes the shop; walking resumes.

## 4. Steps for Qwen (game code: rules, data, views)

Do them in order, one at a time; show the user each step before the next. After each: lint
(`py ..\Hydro\scripts\lint_conventions.py --workspace . <files>`) and build
(`cmd /c "scripts\Build.bat Debug --project EonloreGame --no-pause"`). Rule steps also run the tests:
`bash scripts/check.sh --tests` (WSL), or say the user must run them.

**Q1. `QuestLog::IsClaimed(npc)` (state).** A `const` query next to `Claim`/`Talk` in `State/QuestLog.h`: true when the
NPC is in `m_Claimed`. Unlike `Talk`, it must not remember anything.
*Check:* a test in `GameRulesTest.cpp`: claim → true, release → false, `Talk` not triggered.

**Q2. `Rules/NpcRules.h` (new rule).** `enum class NpcOption : uint8_t { Talk, Quest, Shop, Leave };`, and
`static std::vector<NpcOption> Options(bool hasLines, bool hasQuest, bool hasShop)`. It returns the real options in the
order of §1.5, plus `Leave` at the end only when there are two or more real ones. So a single option comes back alone,
and the director runs it directly.
*Check:* tests for each case: only lines; lines + shop; lines + quest + shop; quest only; nothing (empty list).

**Q3. The stock check in `ShopRules` (rule).**
- Add `static bool InStock(const InventoryDefinition& stock, const DataRowRef& item)`.
- Give `Buy` a `const InventoryDefinition& stock` parameter: an item not in stock returns `NotForSale`, before any other check except `BadCount`.
- Add `static bool CanSell(const ItemDefinition& item)`, the same test `Sell` uses, so the Sell list and `Sell` can't disagree.

*Check:* update the existing buy tests (they pass a stock that has the item); add "not in stock → NotForSale, money unchanged" and `CanSell` cases.

**Q4. Labels (data).** Add `FieldLabel` entries and their text in `GetFieldText`:
- Choice menu: `ChoiceTalk` "Talk", `ChoiceQuest` "Quest", `ChoiceShop` "Buy & Sell", `ChoiceLeave` "Leave".
- Shop modes: `ShopBuy` "Buy", `ShopSell` "Sell".
- Shop messages: `ShopBought` "Bought.", `ShopSold` "Sold.", `ShopNoMoney` "Not enough aster.", `ShopBagFull` "You can't carry any more.", `ShopCantSell` "That can't be sold.", `ShopNothingToSell` "Nothing to sell."
- Shop help: `ShopHelp` "Enter: buy/sell   ←/→: buy/sell   Esc: leave".

*Check:* build.

**Q5. `Scripts/Field/NpcChoiceView.h` (new view).** Copy `InventoryView`'s shape:
- `Bind(scene)` finds the §2 tags under `Choice Canvas`.
- `Show(title, options)` fills and shows the rows, and focuses the first one.
- `Hide()`, and `IsOpen()`.
- `HandleEvent(event, Request&)` returns `Request { None, Pick, Cancel }` and the picked `NpcOption`.

No rule calls and no state changes.
*Check:* build.

**Q6. `Scripts/Field/ShopView.h` (new view).** Same shape:
- `Bind(scene)`.
- `Open(title, stock)`: keeps the stock rows and starts in Buy mode.
- `Present()`: fills the rows from the stock (Buy) or from `Inventory::Get().Slots` filtered by `ShopRules::CanSell` (Sell). Price column: `Price` in Buy mode, `ShopRules::SellPrice` in Sell mode. Also the money and the details.
- `HandleEvent(event, Request&)` returns `Request { None, Buy, Sell, Close }`, plus `GetSelectedItem()` (a `DataRowRef`). The UI has no
Left/Right events, so Buy/Sell is `SwitchMode()`, which the director calls when it sees those keys.

No rule calls and no state changes.
*Check:* build.

**Q7. Docs.** Add a line to `docs/CodeMap.md` for each new file (`NpcRules.h`, `NpcChoiceView.h`, `ShopView.h`) and for
`QuestLog::IsClaimed`.

**Stop here.** Q8 and the rest change `FieldDirectorScript.h` (999 lines) and the UI builder, which `QWEN.md` hands off.

## 5. Steps for Claude (handed off: builder, director, scenes)

**C1. UI.** Build `Choice Canvas` and `Shop Canvas` in `build_game_ui.py` (`build_field`), with the §2 tags. Rebuild the
field scenes and the `FieldUI` prefab. Preview with `preview_ui.py`.

**C2. Director wiring** (`FieldDirectorScript.h`):
- members `NpcChoiceView m_Choice` and `ShopView m_Shop`, bound in `OnCreate`
- `case Kind::Npc` builds `NpcRules::Options` and runs the single option or shows the menu
- `OnUIEvent` routes the choice and shop requests: Pick, Buy, Sell, SwitchMode, Close
- `BuyOne`/`SellOne` call `ShopRules`, show the result message (Q4 labels) and `m_Shop.Present()`, the same pattern as `UseSelected`
- the quest option calls `QuestLog::Get().Talk(npc)`
- the Tab menu can't open while either screen is open (both capture input, so `UI::CapturesInput` already blocks it: confirm)

**C3. Test in Play.** In the city: Pottery merchant → choice menu → Buy a Potion (money drops, count rises) → Sell it back
→ Esc. Then the Blacksmith in the Farm Village; the shrine monk during his quest (Talk + Quest); and an NPC with only lines
(no menu).

## 6. Data the user fills in (Oxygen)

- **Goods per shop**: what fits each trade, from items that exist:
  - Pottery: Potion, Hi-Potion, Antidote, Herb Tea
  - Sake: Herb Tea, Spring Water, Ether
  - Paper: Ward Tome, Straw Charm
  - Blacksmith: Tekko, Iron Sword, Leather Boots
- **ClothMerchant** has no shop. Create `Shops/ClothMerchant.hydef` in Oxygen, for example with Shrine Robe, Headband and Jingasa, and set it on the ClothMerchant row. Qwen must not invent a file handle.

## 7. Risks

- **Quest scripts poll `TakeTalk`.** Choosing Quest calls `Talk` exactly like today's E press, so it behaves the same. Only
  the timing moves by one menu choice.
- **Opening the shop from a dialogue's last line** isn't part of v1: there is no completion signal on `DialogueScript`.
  The choice menu makes it unnecessary.
- **Scenes with their own baked field UI** need the builder re-run (C1). Hand edits there are lost, so port them into the
  builder first.
