-- Migration: Add Stone item (ID 7049) for Pick Stone / Throw Stone skills
-- Stone is a misc item picked up by Thief's Pick Stone skill and consumed by Throw Stone

INSERT INTO items (item_id, name, description, item_type, weight, price, buy_price, sell_price, stackable, max_stack, icon)
VALUES (7049, 'Stone', 'A small stone picked up from the ground. Can be thrown at enemies using the Throw Stone skill.', 'misc', 3, 0, 0, 0, true, 99, 'stone')
ON CONFLICT (item_id) DO NOTHING;
