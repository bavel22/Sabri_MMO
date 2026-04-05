// ro_arrow_crafting.js — Arrow Crafting (AC_MAKINGARROW, Skill ID 305) recipe data
// Source: rAthena db/pre-re/produce_db.yml + iRO Wiki Classic Arrow_Crafting
// Each entry: sourceItemId → { arrowId, qty } (consume 1 source → produce qty arrows)

const ARROW_CRAFTING_RECIPES = {
    // ── Common Loot → Basic Arrows (1750) ──
    909:  { arrowId: 1750, qty: 4 },       // Jellopy
    902:  { arrowId: 1750, qty: 7 },       // Tree Root
    1019: { arrowId: 1750, qty: 40 },      // Trunk
    907:  { arrowId: 1750, qty: 3 },       // Resin
    940:  { arrowId: 1750, qty: 2 },       // Feather
    916:  { arrowId: 1750, qty: 5 },       // Claw of Wolves
    7001: { arrowId: 1750, qty: 2 },       // Tooth of Bat
    1020: { arrowId: 1750, qty: 20 },      // Log (Wooden Block)
    935:  { arrowId: 1750, qty: 10 },      // Shell
    939:  { arrowId: 1750, qty: 3 },       // Worm Peeling
    914:  { arrowId: 1750, qty: 5 },       // Insect Feeler
    904:  { arrowId: 1750, qty: 3 },       // Chonchon Doll
    7063: { arrowId: 1750, qty: 15 },      // Solid Trunk

    // ── Iron/Steel → Iron Arrows (1770) & Steel Arrows (1753) ──
    998:  { arrowId: 1770, qty: 100 },     // Iron → 100 Iron Arrows
    999:  { arrowId: 1753, qty: 100 },     // Steel → 100 Steel Arrows
    1010: { arrowId: 1770, qty: 50 },      // Phracon → 50 Iron Arrows
    1011: { arrowId: 1753, qty: 50 },      // Emveretarcon → 50 Steel Arrows
    985:  { arrowId: 1753, qty: 1000 },    // Elunium → 1000 Steel Arrows

    // ── Oridecon → Oridecon Arrows (1765) ──
    984:  { arrowId: 1765, qty: 250 },     // Oridecon → 250 Oridecon Arrows

    // ── Fire Element → Fire Arrows (1752) ──
    990:  { arrowId: 1752, qty: 600 },     // Red Blood → 600 Fire Arrows
    994:  { arrowId: 1752, qty: 1800 },    // Flame Heart → 1800 Fire Arrows
    7097: { arrowId: 1752, qty: 30 },      // Burning Heart → 30 Fire Arrows

    // ── Water Element → Crystal Arrows (1754) ──
    991:  { arrowId: 1754, qty: 150 },     // Crystal Blue → 150 Crystal Arrows
    995:  { arrowId: 1754, qty: 450 },     // Mystic Frozen → 450 Crystal Arrows

    // ── Wind Element → Wind Arrows (1755) ──
    992:  { arrowId: 1755, qty: 150 },     // Wind of Verdure → 150 Wind Arrows
    996:  { arrowId: 1755, qty: 450 },     // Rough Wind → 450 Wind Arrows

    // ── Earth Element → Stone Arrows (1756) ──
    993:  { arrowId: 1756, qty: 150 },     // Green Live → 150 Stone Arrows
    997:  { arrowId: 1756, qty: 450 },     // Great Nature → 450 Stone Arrows

    // ── Holy → Silver Arrows (1751) & Arrow of Counter Evil (1766) ──
    7510: { arrowId: 1772, qty: 600 },     // Valhala's Flower → 600 Holy Arrows
    912:  { arrowId: 1751, qty: 3 },       // Holy Water → 3 Silver Arrows
    7340: { arrowId: 1766, qty: 150 },     // Rosary in mouth → 150 Counter Evil Arrows

    // ── Ghost → Immaterial Arrows (1757) ──
    714:  { arrowId: 1757, qty: 600 },     // Emperium → 600 Immaterial Arrows

    // ── Dark → Shadow Arrows (1767) ──
    7015: { arrowId: 1767, qty: 100 },     // Dark Crystal Fragment → 100 Shadow Arrows

    // ── Poison → Rusty Arrows (1762) ──
    937:  { arrowId: 1762, qty: 30 },      // Venom Canine → 30 Rusty Arrows
    936:  { arrowId: 1762, qty: 10 },      // Poison Spore → 10 Rusty Arrows
    7033: { arrowId: 1762, qty: 100 },     // Bee Sting → 100 Rusty Arrows

    // ── Status Effect Arrows ──
    7014: { arrowId: 1758, qty: 150 },     // Phracon Fragment → 150 Stun Arrows
    7066: { arrowId: 1759, qty: 150 },     // Snowy Fragment → 150 Frozen Arrows
    7031: { arrowId: 1760, qty: 100 },     // Light Granule → 100 Flash Arrows
    7030: { arrowId: 1761, qty: 100 },     // Dark Granule → 100 Cursed Arrows
    7018: { arrowId: 1768, qty: 150 },     // Moth Dust → 150 Sleep Arrows
    7032: { arrowId: 1769, qty: 100 },     // Mute Fragment → 100 Mute Arrows

    // ── Sharp Arrows (1764) — +20 CRIT ──
    947:  { arrowId: 1764, qty: 100 },     // Horrendous Mouth → 100 Sharp Arrows
    7053: { arrowId: 1764, qty: 150 },     // Talon of Griffon → 150 Sharp Arrows
};

module.exports = { ARROW_CRAFTING_RECIPES };
