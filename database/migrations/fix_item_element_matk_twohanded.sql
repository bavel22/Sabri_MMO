-- Migration: fix_item_element_matk_twohanded.sql
-- Comprehensive pre-renewal data audit — fixes renewal contamination across all equippable items.
--
-- Issues fixed:
--   1. 143 items with bAtkEle in script but element column says 'neutral'
--   2. matk column stored bMatkRate percentage as flat value — should be 0 (pre-renewal has no flat MATK)
--   3. 5 items with renewal-only 'bMatk' (flat MATK) bonus — matk column should be 0
--   4. 269 two-handed weapons with two_handed = false
--      (132 with 2h sub_type + 137 with Both_Hand equip_locations: bows, katars, guns, huuma, etc.)
--   5. 83 full_descriptions showing renewal flat MATK values instead of pre-renewal percentage
--      (e.g. "MATK +210" should be "MATK +15%")

-- ============================================================
-- FIX 1: Element column from bAtkEle script bonuses (143 items)
-- Parse the element from "bAtkEle,Ele_<Element>" in the script column
-- ============================================================

UPDATE items SET element = 'holy'
WHERE script LIKE '%bAtkEle,Ele_Holy%' AND element = 'neutral';

UPDATE items SET element = 'fire'
WHERE script LIKE '%bAtkEle,Ele_Fire%' AND element = 'neutral';

UPDATE items SET element = 'water'
WHERE script LIKE '%bAtkEle,Ele_Water%' AND element = 'neutral';

UPDATE items SET element = 'wind'
WHERE script LIKE '%bAtkEle,Ele_Wind%' AND element = 'neutral';

UPDATE items SET element = 'earth'
WHERE script LIKE '%bAtkEle,Ele_Earth%' AND element = 'neutral';

UPDATE items SET element = 'dark'
WHERE script LIKE '%bAtkEle,Ele_Dark%' AND element = 'neutral';

UPDATE items SET element = 'ghost'
WHERE script LIKE '%bAtkEle,Ele_Ghost%' AND element = 'neutral';

UPDATE items SET element = 'poison'
WHERE script LIKE '%bAtkEle,Ele_Poison%' AND element = 'neutral';

UPDATE items SET element = 'undead'
WHERE script LIKE '%bAtkEle,Ele_Undead%' AND element = 'neutral';

-- ============================================================
-- FIX 2: Clear matk column for items using bMatkRate (percentage)
-- Pre-renewal does not have flat MATK on items. The matk column was
-- incorrectly populated with the bMatkRate percentage value.
-- The actual bonus is in the script column for runtime application.
-- ============================================================

UPDATE items SET matk = 0
WHERE script LIKE '%bMatkRate%' AND matk != 0;

-- ============================================================
-- FIX 3: Clear matk column for items using renewal-only bMatk (flat MATK)
-- bMatk is a renewal-only bonus. In pre-renewal, staves get a hidden
-- engine-level +15% MATK bonus instead. These 5 items should not have
-- flat MATK values in the column.
-- Affected: Banshee Master Card (4450), Entweihen Crothen Card (4451),
--           Centipede Larva Card (4452), Gemini Crown (5570),
--           Triangle Rune Cap (5682)
-- ============================================================

UPDATE items SET matk = 0
WHERE script LIKE '%bonus bMatk,%' AND matk != 0;

-- ============================================================
-- FIX 4: two_handed flag for ALL two-handed weapon types
-- Previous fix only caught sub_type LIKE '2h%' (2hSword, 2hStaff).
-- This also catches Bows, Katars, Revolvers, Rifles, Shotguns,
-- Gatlings, Grenades, Huuma shurikens — all use equip_locations
-- = 'Both_Hand' in rAthena.
-- ============================================================

UPDATE items SET two_handed = true
WHERE sub_type LIKE '2h%' AND two_handed = false;

UPDATE items SET two_handed = true
WHERE equip_locations = 'Both_Hand' AND two_handed = false;

-- ============================================================
-- FIX 5: Replace renewal flat MATK values in full_description
-- with correct pre-renewal percentage values from bMatkRate script.
-- This is done per-item for the 83 affected items.
--
-- Strategy: Replace "MATK +<renewal_flat>" with "MATK +<script_pct>%"
-- in the full_description. We handle the most common patterns.
-- ============================================================

-- Staves/Rods with bMatkRate,15 (standard rod bonus) showing renewal flat MATK
-- Rod (ATK 15) shows "MATK +30" -> should be "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +30', 'MATK +15%')
WHERE item_id IN (1601, 1602, 1603) AND full_description LIKE '%MATK +30%';

-- Wand shows "MATK +45" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +45', 'MATK +15%')
WHERE item_id IN (1604, 1605, 1606) AND full_description LIKE '%MATK +45%';

-- Staff shows "MATK +70" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +70', 'MATK +15%')
WHERE item_id IN (1607, 1608, 1609) AND full_description LIKE '%MATK +70%';

-- Arc Wand shows "MATK +95" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +95', 'MATK +15%')
WHERE item_id IN (1610, 1611, 1612) AND full_description LIKE '%MATK +95%';

-- Mighty Staff shows "MATK +100" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +100', 'MATK +15%')
WHERE item_id = 1613 AND full_description LIKE '%MATK +100%';

-- Wand of Occult "MATK +105" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +105', 'MATK +15%')
WHERE item_id = 1614 AND full_description LIKE '%MATK +105%';

-- Evil Bone Wand "MATK +110" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +110', 'MATK +15%')
WHERE item_id = 1615 AND full_description LIKE '%MATK +110%';

-- Wing Staff "MATK +115" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +115', 'MATK +15%')
WHERE item_id = 1616 AND full_description LIKE '%MATK +115%';

-- Survivor's Rod variants "MATK +120" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +120', 'MATK +15%')
WHERE item_id IN (1617, 1618, 1619, 1620, 1627, 1642) AND full_description LIKE '%MATK +120%';

-- Hypnotist's Staff "MATK +120" -> "MATK +25%" (has bMatkRate,25)
UPDATE items SET full_description = REPLACE(full_description, 'MATK +120', 'MATK +25%')
WHERE item_id IN (1621, 1622) AND full_description LIKE '%MATK +120%';

-- Mighty Staff 1623 "MATK +100" -> "MATK +20%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +100', 'MATK +20%')
WHERE item_id = 1623 AND full_description LIKE '%MATK +100%';

-- Lich's Bone Wand "MATK +170" -> "MATK +20%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +170', 'MATK +20%')
WHERE item_id IN (1624, 1645) AND full_description LIKE '%MATK +170%';

-- Healing Staff "MATK +105" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +105', 'MATK +15%')
WHERE item_id = 1625 AND full_description LIKE '%MATK +105%';

-- Piercing Staff "MATK +145" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +145', 'MATK +15%')
WHERE item_id = 1626 AND full_description LIKE '%MATK +145%';

-- Gentleman's Staff, Release of Wish, Warlock's wands, Recovery wands "MATK +125" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +125', 'MATK +15%')
WHERE item_id IN (1629, 1630, 1632, 1633, 1634, 1635, 1638) AND full_description LIKE '%MATK +125%';

-- Holy Stick "MATK +140" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +140', 'MATK +15%')
WHERE item_id = 1631 AND full_description LIKE '%MATK +140%';

-- Thorn Staff of Darkness "MATK +160" -> "MATK +20%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +160', 'MATK +20%')
WHERE item_id = 1636 AND full_description LIKE '%MATK +160%';

-- Eraser "MATK +170" -> "MATK +20%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +170', 'MATK +20%')
WHERE item_id = 1637 AND full_description LIKE '%MATK +170%';

-- Novice Rod "MATK +32" -> "MATK +16%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +32', 'MATK +16%')
WHERE item_id = 1639 AND full_description LIKE '%MATK +32%';

-- Glorious Arc Wand "MATK +135" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +135', 'MATK +15%')
WHERE item_id IN (1640, 1641) AND full_description LIKE '%MATK +135%';

-- Dead Tree Cane "MATK +155" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +155', 'MATK +15%')
WHERE item_id = 1643 AND full_description LIKE '%MATK +155%';

-- La'cryma Stick "MATK +180" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +180', 'MATK +15%')
WHERE item_id = 1646 AND full_description LIKE '%MATK +180%';

-- Croce Staff "MATK +175" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +175', 'MATK +15%')
WHERE item_id = 1647 AND full_description LIKE '%MATK +175%';

-- Staff Of Bordeaux "MATK +170" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +170', 'MATK +15%')
WHERE item_id = 1648 AND full_description LIKE '%MATK +170%';

-- Bazerald "MATK +105" -> "MATK +10%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +105', 'MATK +10%')
WHERE item_id = 1231 AND full_description LIKE '%MATK +105%';

-- Soul Staff "MATK +200" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +200', 'MATK +15%')
WHERE item_id = 1472 AND full_description LIKE '%MATK +200%';

-- Wizardry Staff "MATK +200" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +200', 'MATK +15%')
WHERE item_id = 1473 AND full_description LIKE '%MATK +200%';

-- Books: Sage's Diary 1560 "MATK +120" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +120', 'MATK +15%')
WHERE item_id = 1560 AND full_description LIKE '%MATK +120%';

-- Sage's Diary 1563 "MATK +140" -> "MATK +20%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +140', 'MATK +20%')
WHERE item_id = 1563 AND full_description LIKE '%MATK +140%';

-- Encyclopedia "MATK +100" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +100', 'MATK +15%')
WHERE item_id IN (1564, 1565) AND full_description LIKE '%MATK +100%';

-- Refined Hardcover Book "MATK +100" -> "MATK +20%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +100', 'MATK +20%')
WHERE item_id = 1567 AND full_description LIKE '%MATK +100%';

-- Principles of Magic "MATK +160" -> "MATK +20%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +160', 'MATK +20%')
WHERE item_id = 1572 AND full_description LIKE '%MATK +160%';

-- Ancient Magic "MATK +140" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +140', 'MATK +15%')
WHERE item_id = 1573 AND full_description LIKE '%MATK +140%';

-- Brave Battle Strategy Book "MATK +90" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +90', 'MATK +15%')
WHERE item_id = 1574 AND full_description LIKE '%MATK +90%';

-- Valorous Battle Strategy Book "MATK +125" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +125', 'MATK +15%')
WHERE item_id = 1575 AND full_description LIKE '%MATK +125%';

-- Glorious Tablet "MATK +115" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +115', 'MATK +15%')
WHERE item_id = 1576 AND full_description LIKE '%MATK +115%';

-- Glorious Apocalypse "MATK +115" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +115', 'MATK +15%')
WHERE item_id = 1577 AND full_description LIKE '%MATK +115%';

-- Giant Encyclopedia 1580 "MATK +100" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +100', 'MATK +15%')
WHERE item_id = 1580 AND full_description LIKE '%MATK +100%';

-- Staff of Destruction "MATK +280" -> "MATK +25%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +280', 'MATK +25%')
WHERE item_id IN (2000, 2003) AND full_description LIKE '%MATK +280%';

-- Divine Cross "MATK +210" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +210', 'MATK +15%')
WHERE item_id IN (2001, 2002) AND full_description LIKE '%MATK +210%';

-- Kronos "MATK +240" -> "MATK +20%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +240', 'MATK +20%')
WHERE item_id = 2004 AND full_description LIKE '%MATK +240%';

-- Dea Staff "MATK +220" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +220', 'MATK +15%')
WHERE item_id = 2005 AND full_description LIKE '%MATK +220%';

-- Accessories/headgear with flat renewal MATK in desc
-- 5th Anniversary Coin "MATK +10" -> "MATK +5%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +10', 'MATK +5%')
WHERE item_id = 2709 AND full_description LIKE '%MATK +10%';

-- Bradium Earring "MATK +5" (but actually 2%) — careful not to match "MATK +5%"
-- Check: desc has "MATK +5" without %, script has bMatkRate,2
-- We need to only replace the non-% version
UPDATE items SET full_description = REPLACE(full_description, 'MATK + 5', 'MATK +2%')
WHERE item_id = 2788 AND full_description LIKE '%MATK + 5%';

-- Rainbow Scarf "MATK +30" -> "MATK +1%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +30', 'MATK +1%')
WHERE item_id = 5463 AND full_description LIKE '%MATK +30%';

-- Gemini Diadem "MATK +30" -> "MATK +2%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +30', 'MATK +2%')
WHERE item_id = 5569 AND full_description LIKE '%MATK +30%';

-- Asura daggers "MATK +50" -> "MATK +10%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +50', 'MATK +10%')
WHERE item_id IN (13010, 13011) AND full_description LIKE '%MATK +50%';

-- Ashura "MATK +98" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +98', 'MATK +15%')
WHERE item_id = 13023 AND full_description LIKE '%MATK +98%';

-- Brave/Valorous Assassin's Damascus "MATK +90" -> "MATK +15%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +90', 'MATK +15%')
WHERE item_id IN (13036, 13037, 13307) AND full_description LIKE '%MATK +90%';

-- Wasteland Outlaw 13109 "MATK +10" -> "MATK +10%" (already 10, just needs %)
-- Be careful: only if there's no % already
UPDATE items SET full_description = REPLACE(full_description, 'MATK +10
', 'MATK +10%
')
WHERE item_id = 13109 AND full_description LIKE '%MATK +10%' AND full_description NOT LIKE '%MATK +10\%%';

-- Brave Gladiator Blade "MATK +74" -> "MATK +10%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +74', 'MATK +10%')
WHERE item_id = 13411 AND full_description LIKE '%MATK +74%';

-- Elemental Sword "MATK +95" -> "MATK +5%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +95', 'MATK +5%')
WHERE item_id = 13414 AND full_description LIKE '%MATK +95%';

-- Glorious Rapier/Holy Avenger "MATK +80" -> "MATK +10%"
UPDATE items SET full_description = REPLACE(full_description, 'MATK +80', 'MATK +10%')
WHERE item_id IN (13417, 13418) AND full_description LIKE '%MATK +80%';

-- Moon Rabbit Hat 5457: desc has "MATK +5" which matches the bMatkRate,5 — but as flat not %
-- The desc also mentions MATK +1 for refine bonus. Fix the main one.
UPDATE items SET full_description = REPLACE(full_description, 'MATK +5
', 'MATK +5%
')
WHERE item_id = 5457 AND full_description LIKE '%MATK +5%' AND full_description NOT LIKE '%MATK +5\%%';

-- ============================================================
-- Verification queries (run manually to confirm)
-- ============================================================
-- SELECT COUNT(*) FROM items WHERE script LIKE '%bAtkEle%' AND element = 'neutral' AND script NOT LIKE '%Ele_Neutral%';
--   Expected: 0
--
-- SELECT COUNT(*) FROM items WHERE (script LIKE '%bMatkRate%' OR script LIKE '%bonus bMatk,%') AND matk != 0;
--   Expected: 0
--
-- SELECT COUNT(*) FROM items WHERE equip_locations = 'Both_Hand' AND two_handed = false;
--   Expected: 0
--
-- SELECT COUNT(*) FROM items WHERE sub_type LIKE '2h%' AND two_handed = false;
--   Expected: 0
