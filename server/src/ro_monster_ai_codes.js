// ============================================================
// RO Monster AI Codes — rAthena pre-renewal mob_db.yml
// Maps monster ID → Aegis AI type code
// Source: https://github.com/rathena/rathena/blob/master/db/pre-re/mob_db.yml
// ============================================================
//
// AI Code Reference:
//  01 = Passive, flees when attacked
//  02 = Passive, stands ground when attacked
//  03 = Passive, retaliates when attacked
//  04 = Aggressive melee, short chase
//  05 = Aggressive ranged
//  06 = Immobile (plants, eggs, objects)
//  07 = Passive, assists allies of same type
//  08 = Passive, looter
//  09 = Aggressive, long chase, target switching
//  10 = Immobile aggressive (attacks in range but doesn't move)
//  12 = WoE Guardian
//  13 = Aggressive, assists allies, mob behavior
//  17 = Passive, change target on attack
//  20 = Aggressive ranged, kite/reposition
//  21 = Boss/MVP — aggressive, wide chase, status immune
//  25 = Crystal — immobile, special behavior
//  26 = Aggressive ranged, special attack patterns
//  27 = Special spawn/summon AI

const MONSTER_AI_CODES = {
  1001:  9,   // Scorpion
  1002:  2,   // Poring
  1004:  3,   // Hornet
  1005:  4,   // Familiar
  1007:  1,   // Fabre
  1008:  6,   // Pupa
  1009:  3,   // Condor
  1010:  1,   // Willow
  1011:  1,   // Chonchon
  1012:  1,   // Roda Frog
  1013:  3,   // Wolf
  1014:  1,   // Spore
  1015:  4,   // Zombie
  1016:  5,   // Archer Skeleton
  1018:  1,   // Creamy
  1019:  3,   // Peco Peco
  1020: 10,   // Mandragora
  1023:  4,   // Orc Warrior
  1024: 17,   // Wormtail
  1025:  1,   // Boa
  1026:  4,   // Munak
  1028:  4,   // Soldier Skeleton
  1029:  9,   // Isis
  1030: 17,   // Anacondaq
  1031:  2,   // Poporing
  1032:  2,   // Verit
  1033:  9,   // Elder Willow
  1034:  1,   // Thara Frog
  1035:  4,   // Hunter Fly
  1036:  4,   // Ghoul
  1037:  9,   // Side Winder
  1038: 21,   // Osiris
  1039: 21,   // Baphomet
  1040: 17,   // Golem
  1041:  4,   // Mummy
  1042:  7,   // Steel Chonchon
  1044:  9,   // Obeaune
  1045:  4,   // Marc
  1046: 21,   // Doppelganger
  1047:  6,   // Peco Peco Egg
  1048:  6,   // Thief Bug Egg
  1049:  1,   // Picky
  1050:  1,   // Picky
  1051:  7,   // Thief Bug
  1052:  1,   // Rocker
  1053:  7,   // Thief Bug Female
  1054: 13,   // Thief Bug Male
  1055:  1,   // Muka
  1056: 17,   // Smokie
  1057:  7,   // Yoyo
  1058:  7,   // Metaller
  1059: 21,   // Mistress
  1060: 17,   // Bigfoot
  1061: 20,   // Nightmare
  1062:  1,   // Santa Poring
  1063:  1,   // Lunatic
  1064:  1,   // Megalodon
  1065:  4,   // Strouf
  1066: 17,   // Vadon
  1067: 17,   // Cornutus
  1068: 10,   // Hydra
  1069:  4,   // Swordfish
  1070:  2,   // Kukre
  1071:  4,   // Pirate Skeleton
  1072:  4,   // Kaho
  1073:  1,   // Crab
  1074: 17,   // Shellfish
  1076: 17,   // Skeleton
  1077:  4,   // Poison Spore
  1078:  6,   // Red Plant
  1079:  6,   // Blue Plant
  1080:  6,   // Green Plant
  1081:  6,   // Yellow Plant
  1082:  6,   // White Plant
  1083:  6,   // Shining Plant
  1084:  6,   // Black Mushroom
  1085:  6,   // Red Mushroom
  1086:  7,   // Golden Thief Bug
  1087: 21,   // Orc Hero
  1088: 21,   // Vocal
  1089: 21,   // Toad
  1090: 21,   // Mastering
  1091: 21,   // Dragon Fly
  1092: 21,   // Vagabond Wolf
  1093: 21,   // Eclipse
  1094: 17,   // Ambernite
  1095:  7,   // Andre
  1096: 21,   // Angeling
  1097:  6,   // Ant Egg
  1098: 21,   // Anubis
  1099: 21,   // Argiope
  1100:  9,   // Argos
  1101: 21,   // Baphomet Jr.
  1102: 21,   // Bathory
  1103: 17,   // Caramel
  1104: 17,   // Coco
  1105:  7,   // Deniro
  1106: 13,   // Desert Wolf
  1107:  3,   // Baby Desert Wolf
  1108: 17,   // Deviace
  1109: 21,   // Deviruchi
  1110: 17,   // Dokebi
  1111:  9,   // Drainliar
  1112: 21,   // Drake
  1113:  2,   // Drops
  1114: 17,   // Dustiness
  1115: 21,   // Eddga
  1116: 17,   // Eggyra
  1117: 21,   // Evil Druid
  1118: 10,   // Flora
  1119:  4,   // Frilldora
  1120: 21,   // Ghostring
  1121: 17,   // Giearth
  1122: 21,   // Goblin
  1123:  9,   // Goblin
  1124: 13,   // Goblin
  1125: 13,   // Goblin
  1126: 13,   // Goblin
  1127:  1,   // Hode
  1128: 17,   // Horn
  1129: 13,   // Horong
  1130: 21,   // Jakk
  1131: 21,   // Joker
  1132: 21,   // Khalitzburg
  1133: 13,   // Kobold
  1134: 13,   // Kobold
  1135: 13,   // Kobold
  1138:  2,   // Magnolia
  1139:  9,   // Mantis
  1140:  9,   // Marduk
  1141:  1,   // Marina
  1142:  6,   // Marine Sphere
  1143:  9,   // Marionette
  1144: 17,   // Marse
  1145:  1,   // Martin
  1146:  9,   // Matyr
  1147: 21,   // Maya
  1148: 21,   // Medusa
  1149:  9,   // Minorous
  1150: 21,   // Moonlight Flower
  1151: 21,   // Myst
  1152:  4,   // Orc Skeleton
  1153:  4,   // Orc Zombie
  1154:  9,   // Pasana
  1155:  9,   // Petite
  1156:  9,   // Petite
  1157: 21,   // Pharaoh
  1158: 17,   // Phen
  1159: 21,   // Phreeoni
  1160:  7,   // Piere
  1161:  1,   // Plankton
  1162:  4,   // Rafflesia
  1163:  9,   // Raydric
  1164:  4,   // Requiem
  1165:  4,   // Sandman
  1166: 17,   // Savage
  1167:  1,   // Savage Babe
  1169:  4,   // Skeleton Worker
  1170: 17,   // Sohee
  1174: 17,   // Stainer
  1175: 17,   // Tarou
  1176: 17,   // Vitata
  1177:  2,   // Zenorc
  1178:  4,   // Zerom
  1179:  9,   // Whisper
  1180: 21,   // Nine Tail
  1182:  6,   // Thief Mushroom
  1183:  4,   // Chonchon
  1184:  4,   // Fabre
  1185:  6,   // Whisper
  1186: 21,   // Giant Whisper
  1187:  6,   // Switch
  1188:  9,   // Bongun
  1189:  9,   // Orc Archer
  1190: 21,   // Orc Lord
  1191:  9,   // Mimic
  1192: 21,   // Wraith
  1193: 21,   // Alarm
  1194:  9,   // Arclouze
  1195: 21,   // Rideword
  1196: 13,   // Skeleton Prisoner
  1197: 13,   // Zombie Prisoner
  1198: 13,   // Dark Priest
  1199:  9,   // Punk
  1200: 13,   // Zealotus
  1201: 13,   // Rybio
  1202: 13,   // Phendark
  1203: 21,   // Mysteltainn
  1204: 21,   // Ogretooth
  1205: 21,   // Executioner
  1206: 21,   // Anolian
  1207: 21,   // Sting
  1208: 21,   // Wanderer
  1209:  9,   // Cramp
  1211:  9,   // Brilight
  1212:  9,   // Iron Fist
  1213: 21,   // High Orc
  1214:  9,   // Choco
  1215:  9,   // Stem Worm
  1216: 21,   // Penomena
  1219: 21,   // Abysmal Knight
  1220: 21,   // Desert Wolf
  1221: 21,   // Savage
  1229:  1,   // Fabre
  1230:  6,   // Pupa
  1231:  1,   // Creamy
  1232:  6,   // Peco Peco Egg
  1234:  7,   // Yoyo
  1235: 13,   // Smoking Orc
  1236:  6,   // Ant Egg
  1237:  7,   // Andre
  1238:  7,   // Piere
  1239:  7,   // Deniro
  1240:  1,   // Picky
  1241:  1,   // Picky
  1242:  1,   // Marin
  1243: 21,   // Sasquatch
  1244:  1,   // Christmas Jakk
  1245:  1,   // Christmas Goblin
  1246: 17,   // Christmas Cookie
  1247:  1,   // Antonio
  1248:  5,   // Cruiser
  1249: 17,   // Myst Case
  1250: 21,   // Chepet
  1251: 21,   // Stormy Knight
  1252: 21,   // Hatii
  1253:  5,   // Gargoyle
  1254: 21,   // Raggler
  1255: 21,   // Nereid
  1256: 21,   // Pest
  1257: 21,   // Injustice
  1258:  5,   // Goblin Archer
  1259: 21,   // Gryphon
  1260: 21,   // Dark Frame
  1261:  2,   // Wild Rose
  1262: 21,   // Mutant Dragonoid
  1263: 21,   // Wind Ghost
  1264: 21,   // Merman
  1265:  3,   // Cookie
  1266: 17,   // Aster
  1267: 21,   // Carat
  1268: 21,   // Bloody Knight
  1269: 17,   // Clock
  1270: 17,   // Clock Tower Manager
  1271: 17,   // Alligator
  1272: 21,   // Dark Lord
  1273: 21,   // Orc Lady
  1274: 10,   // Megalith
  1275: 17,   // Alice
  1276:  5,   // Raydric Archer
  1277: 10,   // Greatest General
  1278: 17,   // Stalactic Golem
  1279: 21,   // Tri Joint
  1280: 17,   // Goblin Steamrider
  1281: 17,   // Sage Worm
  1282:  5,   // Kobold Archer
  1283: 21,   // Chimera
  1285: 12,   // Archer Guardian
  1286: 12,   // Knight Guardian
  1287: 12,   // Soldier Guardian
  1288:  6,   // Emperium
  1289: 21,   // Maya Purple
  1290: 21,   // Skeleton General
  1291: 21,   // Wraith Dead
  1292: 21,   // Mini Demon
  1293: 21,   // Creamy Fear
  1294: 21,   // Killer Mantis
  1295: 21,   // Owl Baron
  1296: 21,   // Kobold Leader
  1297: 21,   // Ancient Mummy
  1298: 21,   // Zombie Master
  1299: 21,   // Goblin Leader
  1300: 21,   // Caterpillar
  1301: 21,   // Am Mut
  1302: 21,   // Dark Illusion
  1303: 21,   // Giant Hornet
  1304: 21,   // Giant Spider
  1305: 21,   // Ancient Worm
  1306: 21,   // Leib Olmai
  1307: 21,   // Cat o' Nine Tails
  1308: 21,   // Panzer Goblin
  1309: 21,   // Gajomart
  1310: 21,   // Majoruros
  1311: 21,   // Gullinbursti
  1312: 21,   // Turtle General
  1313: 21,   // Mobster
  1314: 17,   // Permeter
  1315: 21,   // Assaulter
  1316: 17,   // Solider
  1317:  4,   // Seal
  1318: 21,   // Heater
  1319: 21,   // Freezer
  1320: 21,   // Owl Duke
  1321: 21,   // Dragon Tail
  1322:  2,   // Spring Rabbit
  1323:  4,   // Sea Otter
  1324:  6,   // Treasure Chest
  1325:  6,   // Treasure Chest
  1326:  6,   // Treasure Chest
  1327:  6,   // Treasure Chest
  1328:  6,   // Treasure Chest
  1329:  6,   // Treasure Chest
  1330:  6,   // Treasure Chest
  1331:  6,   // Treasure Chest
  1332:  6,   // Treasure Chest
  1333:  6,   // Treasure Chest
  1334:  6,   // Treasure Chest
  1335:  6,   // Treasure Chest
  1336:  6,   // Treasure Chest
  1337:  6,   // Treasure Chest
  1338:  6,   // Treasure Chest
  1339:  6,   // Treasure Chest
  1340:  6,   // Treasure Chest
  1341:  6,   // Treasure Chest
  1342:  6,   // Treasure Chest
  1343:  6,   // Treasure Chest
  1344:  6,   // Treasure Chest
  1345:  6,   // Treasure Chest
  1346:  6,   // Treasure Chest
  1347:  6,   // Treasure Chest
  1348:  6,   // Treasure Chest
  1349:  6,   // Treasure Chest
  1350:  6,   // Treasure Chest
  1351:  6,   // Treasure Chest
  1352:  6,   // Treasure Chest
  1353:  6,   // Treasure Chest
  1354:  6,   // Treasure Chest
  1355:  6,   // Treasure Chest
  1356:  6,   // Treasure Chest
  1357:  6,   // Treasure Chest
  1358:  6,   // Treasure Chest
  1359:  6,   // Treasure Chest
  1360:  6,   // Treasure Chest
  1361:  6,   // Treasure Chest
  1362:  6,   // Treasure Chest
  1363:  6,   // Treasure Chest
  1364: 21,   // Assaulter
  1365: 17,   // Apocalypse
  1366:  9,   // Lava Golem
  1367: 20,   // Blazer
  1368: 10,   // Geographer
  1369:  3,   // Grand Peco
  1370: 21,   // Succubus
  1371:  4,   // False Angel
  1372:  3,   // Goat
  1373: 21,   // Lord of the Dead
  1374: 21,   // Incubus
  1375:  4,   // The Paper
  1376:  4,   // Harpy
  1377:  4,   // Elder
  1378:  4,   // Demon Pungus
  1379:  4,   // Nightmare Terror
  1380:  4,   // Driller
  1381:  4,   // Grizzly
  1382:  4,   // Diabolic
  1383:  4,   // Explosion
  1384: 13,   // Deleter
  1385: 13,   // Deleter
  1386:  4,   // Sleeper
  1387:  4,   // Gig
  1388: 21,   // Arc Angeling
  1389: 21,   // Dracula
  1390:  5,   // Violy
  1391:  7,   // Galapago
  1392:  5,   // Rotar Zairo
  1393:  4,   // Mummy
  1394:  4,   // Zombie
  1395: 25,   // Wind Crystal
  1396: 25,   // Earth Crystal
  1397: 25,   // Fire Crystal
  1398: 25,   // Water Crystal
  1399: 21,   // Baphomet
  1400:  1,   // Karakasa
  1401: 21,   // Shinobi
  1402:  1,   // Poison Toad
  1403:  5,   // Firelock Soldier
  1404: 17,   // Miyabi Doll
  1405:  4,   // Tengu
  1406:  4,   // Kapha
  1408: 13,   // Bloody Butterfly
  1409: 17,   // Dumpling Child
  1410:  5,   // Enchanted Peach Tree
  1412:  5,   // Taoist Hermit
  1413: 17,   // Hermit Plant
  1415:  4,   // Baby Leopard
  1416: 21,   // Evil Nymph
  1417: 17,   // Zipper Bear
  1418: 21,   // Evil Snake Lord
  1419:  4,   // Familiar
  1420:  4,   // Archer Skeleton
  1421:  4,   // Isis
  1422:  4,   // Hunter Fly
  1423:  4,   // Ghoul
  1424:  4,   // Side Winder
  1425:  4,   // Obeaune
  1426:  4,   // Marc
  1427:  4,   // Nightmare
  1428:  4,   // Poison Spore
  1429:  4,   // Argiope
  1430:  4,   // Argos
  1431:  4,   // Baphomet Jr.
  1432:  4,   // Desert Wolf
  1433:  4,   // Deviruchi
  1434:  4,   // Drainliar
  1435:  4,   // Evil Druid
  1436:  4,   // Jakk
  1437:  4,   // Joker
  1438:  4,   // Khalitzburg
  1439:  4,   // High Orc
  1440:  4,   // Stem Worm
  1441:  4,   // Penomena
  1442:  4,   // Sasquatch
  1443:  4,   // Cruiser
  1444:  4,   // Chepet
  1445:  4,   // Raggler
  1446:  4,   // Injustice
  1447:  4,   // Gryphon
  1448:  4,   // Dark Frame
  1449:  4,   // Mutant Dragonoid
  1450:  4,   // Wind Ghost
  1451:  4,   // Merman
  1452:  4,   // Orc Lady
  1453:  4,   // Raydric Archer
  1454:  4,   // Tri Joint
  1455:  4,   // Kobold Archer
  1456:  4,   // Chimera
  1457:  4,   // Mantis
  1458:  4,   // Marduk
  1459:  4,   // Marionette
  1460:  4,   // Matyr
  1461:  4,   // Minorous
  1462:  4,   // Orc Skeleton
  1463:  4,   // Orc Zombie
  1464:  4,   // Pasana
  1465:  4,   // Petite
  1466:  4,   // Petite
  1467:  4,   // Raydric
  1468:  4,   // Requim
  1469:  4,   // Skeleton Worker
  1470:  4,   // Zerom
  1471:  4,   // Nine Tail
  1472:  4,   // Bongun
  1473:  4,   // Orc Archer
  1474:  4,   // Mimic
  1475:  4,   // Wraith
  1476:  4,   // Alarm
  1477:  4,   // Arclouze
  1478:  4,   // Rideword
  1479:  4,   // Skeleton Prisoner
  1480:  4,   // Zombie Prisoner
  1481:  4,   // Punk
  1482:  4,   // Zealotus
  1483:  4,   // Rybio
  1484:  4,   // Phendark
  1485:  4,   // Mysteltainn
  1486:  4,   // Ogretooth
  1487:  4,   // Executioner
  1488:  4,   // Anolian
  1489:  4,   // Sting
  1490:  4,   // Wanderer
  1491:  4,   // Dokebi
  1492: 21,   // Samurai Specter
  1493:  4,   // Dryad
  1494:  3,   // Beetle King
  1495:  4,   // Stone Shooter
  1497:  4,   // Wooden Golem
  1498:  4,   // Wootan Shooter
  1499:  4,   // Wootan Fighter
  1500: 10,   // Parasite
  1502:  4,   // Bring it on!
  1503:  4,   // Gibbet
  1504:  4,   // Dullahan
  1505:  4,   // Loli Ruri
  1506:  4,   // Disguise
  1507:  4,   // Bloody Murderer
  1508:  4,   // Quve
  1509:  4,   // Lude
  1510:  4,   // Heirozoist
  1511: 10,   // Amon Ra
  1512:  4,   // Yao Jun
  1513:  4,   // Mao Guai
  1514:  2,   // Zhu Po Long
  1515:  4,   // Baby Hatii
  1516: 17,   // Mi Gao
  1517:  4,   // Jing Guai
  1518: 21,   // White Lady
  1519: 21,   // Green Maiden
  1520:  1,   // Boiled Rice
  1521: 17,   // Alice
  1522: 21,   // Ancient Mummy
  1523:  5,   // Firelock Soldier
  1524:  4,   // Baby Leopard
  1525: 21,   // Bathory
  1526: 13,   // Bloody Butterfly
  1527: 17,   // Clock Tower Manager
  1528: 17,   // Clock
  1529: 21,   // Evil Snake Lord
  1530: 21,   // Dracula
  1531:  5,   // Taoist Hermit
  1532:  4,   // Explosion
  1533:  9,   // Seal
  1534: 21,   // Goblin
  1535:  9,   // Goblin
  1536: 13,   // Goblin
  1537: 13,   // Goblin
  1538: 13,   // Goblin
  1539: 21,   // Goblin Leader
  1540: 17,   // Golem
  1541: 10,   // Greatest General
  1542: 21,   // Incantation Samurai
  1543:  4,   // Kapha
  1544:  1,   // Karakasa
  1545: 13,   // Kobold
  1546: 13,   // Kobold
  1547: 13,   // Kobold
  1548: 21,   // Kobold Leader
  1549:  9,   // Lava Golem
  1550:  5,   // Enchanted Peach Tree
  1551: 17,   // Marse
  1552: 17,   // Miyabi Doll
  1553: 21,   // Myst
  1554:  4,   // Nightmare Terror
  1555: 10,   // Parasite
  1556:  1,   // Poisonous Toad
  1557:  5,   // Rotar Zairo
  1558:  4,   // Sandman
  1559:  9,   // Scorpion
  1560: 21,   // Shinobi
  1561: 17,   // Smokie
  1562:  4,   // Soldier Skeleton
  1563:  4,   // Tengu
  1564: 21,   // Evil Nymph
  1565: 17,   // Hermit Plant
  1566: 21,   // Wraith Dead
  1567: 21,   // Ancient Worm
  1568: 21,   // Angeling
  1569: 21,   // Bloody Knight
  1570:  9,   // Cramp
  1571: 17,   // Deviace
  1572:  2,   // Drops
  1573:  4,   // Elder
  1574:  9,   // Elder Willow
  1575: 10,   // Flora
  1576: 21,   // Ghostring
  1577:  5,   // Goblin Archer
  1578: 13,   // Horong
  1579: 10,   // Hydra
  1580: 21,   // Incubus
  1581: 21,   // Vocal
  1582: 21,   // Deviling
  1583: 21,   // Tao Gunka
  1584: 13,   // Tamruan
  1585:  2,   // Mime Monkey
  1586:  2,   // Leaf Cat
  1587:  9,   // Kraben
  1588:  1,   // Christmas Orc
  1589: 10,   // Mandragora
  1590: 10,   // Geographer
  1591:  4,   // Lunatic
  1592:  3,   // Gangster
  1593:  4,   // Ancient Mummy
  1594: 21,   // Freezer
  1595:  1,   // Marin
  1596: 13,   // Tamruan
  1597:  5,   // Gargoyle
  1598: 20,   // Blazzer
  1599: 21,   // Giant Whisper
  1600: 21,   // Heater
  1601: 21,   // Permeter
  1602: 21,   // Solider
  1603: 17,   // Bigfoot
  1604: 21,   // Giant Hornet
  1605: 21,   // Dark Illusion
  1606:  4,   // Baby Hatii
  1607: 21,   // Christmas Goblin
  1608: 13,   // Thief Bug Male
  1609:  2,   // Zhu Po Long
  1610:  4,   // Munak
  1611:  9,   // Bongun
  1612:  4,   // Yao Jun
  1613:  2,   // Metaling
  1614: 17,   // Mineral
  1615:  4,   // Obsidian
  1616: 17,   // Pitman
  1617:  4,   // Old Stove
  1618: 21,   // Ungoliant
  1619:  2,   // Porcellio
  1620:  4,   // Noxious
  1621:  4,   // Venomous
  1622: 20,   // Teddy Bear
  1623: 21,   // RSX-0806
  1624:  4,   // Old Stove
  1625:  4,   // Porcellio
  1626: 21,   // Hellion Revenant
  1627:  4,   // Anopheles
  1628:  3,   // Holden
  1629:  4,   // Hill Wind
  1630:  4,   // White Lady
  1631:  4,   // Green Maiden
  1632: 17,   // Gremlin
  1633: 17,   // Beholder
  1634:  9,   // Seyren Windsor
  1635:  9,   // Eremes Guile
  1636:  9,   // Howard Alt-Eisen
  1637: 20,   // Margaretha Sorin
  1638:  9,   // Cecil Damon
  1639: 20,   // Kathryne Keyron
  1640: 21,   // Lord Knight Seyren
  1641: 21,   // Assassin Cross Eremes
  1642: 21,   // Whitesmith Howard
  1643: 21,   // High Priest Margaretha
  1644: 21,   // Sniper Cecil
  1645: 21,   // High Wizard Kathryne
  1646: 21,   // Lord Knight Seyren
  1647: 21,   // Assassin Cross Eremes
  1648: 21,   // Whitesmith Howard
  1649: 21,   // High Priest Margaretha
  1650: 21,   // Sniper Cecil
  1651: 21,   // High Wizard Kathryne
  1652:  4,   // Egnigem Cenia
  1653:  4,   // Wickebine Tres
  1654:  4,   // Armeyer Dinze
  1655:  4,   // Errende Ebecee
  1656:  4,   // Kavach Icarus
  1657:  4,   // Laurell Weinder
  1658: 21,   // Egnigem Cenia
  1659:  4,   // Wickebine Tres
  1660:  4,   // Armeyer Dinze
  1661:  4,   // Errende Ebecee
  1662:  4,   // Kavach Icarus
  1663:  4,   // Laurell Weinder
  1664: 10,   // Photon Cannon
  1665: 10,   // Photon Cannon
  1666: 10,   // Photon Cannon
  1667: 10,   // Photon Cannon
  1668: 21,   // Archdam
  1669:  4,   // Dimik
  1670:  4,   // Dimik
  1671:  4,   // Dimik
  1672:  4,   // Dimik
  1673:  4,   // Dimik
  1674: 10,   // Monemus
  1675:  4,   // Venatu
  1676:  4,   // Venatu
  1677:  4,   // Venatu
  1678:  4,   // Venatu
  1679:  4,   // Venatu
  1680:  4,   // Hill Wind
  1681:  4,   // Gemini-S58
  1682:  4,   // Remover
  1683:  4,   // Photon Cannon
  1684:  4,   // Archdam
  1685: 21,   // Vesper
  1686:  4,   // Orc Baby
  1687:  2,   // Grove
  1688: 10,   // Lady Tanee
  1689: 21,   // White Lady
  1690:  2,   // Spring Rabbit
  1691:  4,   // Kraben
  1692:  4,   // Breeze
  1693:  4,   // Plasma
  1694:  4,   // Plasma
  1695:  4,   // Plasma
  1696:  4,   // Plasma
  1697:  4,   // Plasma
  1698: 21,   // Death Word
  1699:  4,   // Ancient Mimic
  1700: 20,   // Dame of Sentinel
  1701: 20,   // Mistress of Shelter
  1702: 20,   // Baroness of Retribution
  1703: 20,   // Lady Solace
  1704: 21,   // Odium of Thanatos
  1705: 21,   // Despero of Thanatos
  1706: 21,   // Maero of Thanatos
  1707: 21,   // Dolor of Thanatos
  1708: 21,   // Memory of Thanatos
  1709: 20,   // Odium of Thanatos
  1710: 20,   // Despero of Thanatos
  1711: 20,   // Maero of Thanatos
  1712: 20,   // Dolor of Thanatos
  1713:  9,   // Acidus
  1714:  9,   // Ferus
  1715:  4,   // Novus
  1716:  9,   // Acidus
  1717:  9,   // Ferus
  1718:  4,   // Novus
  1719: 21,   // Detardeurus
  1720: 21,   // Hydrolancer
  1721:  6,   // Dragon Egg
  1722:  1,   // Jakk
  1723: 21,   // Cecil Damon
  1724: 10,   // Photon Cannon
  1725:  2,   // Poring
  1726:  2,   // Lunatic
  1727:  2,   // Savage Babe
  1728:  2,   // Baby Desert Wolf
  1729:  2,   // Baphomet Jr.
  1730:  2,   // Deviruchi
  1731: 21,   // Doppelganger
  1732:  6,   // Treasure Chest
  1733: 21,   // Kiehl
  1734: 21,   // Kiel D-01
  1735: 13,   // Alicel
  1736: 13,   // Aliot
  1737: 17,   // Aliza
  1738:  4,   // Constant
  1739: 13,   // Alicel
  1740: 13,   // Aliot
  1741:  4,   // Christmas Cookie
  1742:  4,   // Carat
  1743:  4,   // Myst Case
  1744:  4,   // Wild Rose
  1745:  5,   // Constant
  1746:  4,   // Aliza
  1747:  4,   // Boa
  1748:  4,   // Anacondaq
  1749:  4,   // Medusa
  1750:  6,   // Red Plant
  1751: 21,   // Valkyrie Randgris
  1752: 20,   // Skogul
  1753: 20,   // Frus
  1754: 21,   // Skeggiold
  1755: 21,   // Skeggiold
  1756:  4,   // Hydrolancer
  1757:  4,   // Acidus
  1758:  4,   // Ferus
  1759:  4,   // Acidus
  1760:  4,   // Ferus
  1761:  4,   // Skogul
  1762:  4,   // Frus
  1763:  4,   // Skeggiold
  1764:  4,   // Skeggiold
  1765: 21,   // Valkyrie
  1766: 21,   // Angeling
  1767: 21,   // Deviling
  1768: 21,   // Gloom Under Night
  1769: 20,   // Agav
  1770: 20,   // Echio
  1771:  4,   // Vanberk
  1772:  4,   // Isilla
  1773:  4,   // Hodremlin
  1774: 20,   // Seeker
  1775:  4,   // Snowier
  1776:  2,   // Siroma
  1777:  4,   // Ice Titan
  1778: 20,   // Gazeti
  1779: 21,   // Ktullanux
  1780: 10,   // Muscipular
  1781: 10,   // Drosera
  1782:  7,   // Roween
  1783:  7,   // Galion
  1784:  2,   // Stapo
  1785: 21,   // Atroce
  1786: 20,   // Agav
  1787: 20,   // Echio
  1788: 20,   // Ice Titan
  1789: 10,   // Iceicle
  1790:  4,   // Rafflesia
  1791:  7,   // Galion
  1792:  6,   // Soccer Ball
  1793: 21,   // Megalith
  1794: 20,   // Roween
  1795: 21,   // Bloody Knight
  1796: 20,   // Aunoe
  1797: 21,   // Fanat
  1798:  6,   // Treasure Chest
  1799: 21,   // Lord Knight Seyren
  1800: 21,   // Assassin Cross Eremes
  1801: 21,   // Mastersmith Howard
  1802: 21,   // High Priest Margaretha
  1803: 20,   // Sniper Cecil
  1804: 21,   // High Wizard Kathryne
  1805: 21,   // Lord Knight Seyren
  1806: 21,   // Assassin Cross Eremes
  1807: 21,   // Mastersmith Howard
  1808: 21,   // High Priest Margaretha
  1809: 21,   // Sniper Cecil
  1810: 21,   // High Wizard Kathryne
  1811: 17,   // Bandit
  1812:  6,   // Delightful Lude
  1813: 21,   // Hydrolancer
  1814: 21,   // Moonlight Flower
  1815:  6,   // Rice Cake
  1816:  6,   // Gourd
  1817: 21,   // Detarderous
  1818: 21,   // Alarm
  1819: 21,   // Bathory
  1820: 17,   // Bigfoot
  1821: 13,   // Desert Wolf
  1822: 21,   // Deviruchi
  1823: 21,   // Freezer
  1824:  4,   // Baby Hatii
  1825: 21,   // Christmas Goblin
  1826: 21,   // Myst
  1827: 21,   // Sasquatch
  1828: 17,   // Gullinbrusti
  1829: 21,   // Sword Master
  1830: 26,   // Bow Master
  1831: 21,   // Salamander
  1832: 21,   // Ifrit
  1833: 21,   // Kasa
  1834: 21,   // Salamander
  1835: 21,   // Kasa
  1836:  2,   // Magmaring
  1837: 26,   // Fire Imp
  1838: 17,   // Knocker
  1839: 21,   // Byorgue
  1840:  1,   // Golden Savage
  1841:  1,   // Snake Lord's Minion
  1842: 17,   // Snake Lord's Minion
  1843:  9,   // Snake Lord's Minion
  1844:  9,   // Snake Lord's Minion
  1845:  6,   // Treasure Box
  1846:  6,   // Dream Metal
  1847: 21,   // Poring
  1848: 21,   // Baphomet
  1849: 21,   // Osiris
  1850: 21,   // Orc Hero
  1851: 21,   // Mobster
  1852: 21,   // Angeling
  1853: 21,   // Deviling
  1854:  2,   // Muka
  1855:  4,   // Poison Spore
  1856:  2,   // Magnolia
  1857:  1,   // Marin
  1858:  1,   // Plankton
  1859: 10,   // Mandragora
  1860: 17,   // Coco
  1861:  9,   // Choco
  1862:  1,   // Martin
  1863:  2,   // Spring Rabbit
  1864: 21,   // Zombie Slaughter
  1865: 26,   // Ragged Zombie
  1866: 21,   // Hell Poodle
  1867: 21,   // Banshee
  1868: 21,   // Banshee
  1869:  9,   // Flame Skull
  1870: 21,   // Necromancer
  1871: 21,   // Fallen Bishop Hibram
  1872: 26,   // Hell Fly
  1873: 21,   // Beelzebub
  1874: 21,   // Beelzebub
  1875: 21,   // Dead King
  1876: 21,   // Lord of the Dead
  1877: 25,   // Crystal
  1878:  6,   // Shining Plant
  1879: 21,   // Eclipse
  1880:  1,   // Wood Goblin
  1881:  3,   // Les
  1882:  4,   // Baba Yaga
  1883:  4,   // Uzhas
  1884:  4,   // Mavka
  1885: 21,   // Gopinich
  1886:  4,   // Mavka
  1887: 21,   // Freezer
  1888:  4,   // Baby Hatii
  1889: 21,   // Marozka's Guard
  1890: 20,   // The Immortal Koshei
  1891: 21,   // Valkyrie
  1892:  4,   // Lolo Ruri
  1893: 21,   // Abysmal Knight
  1894:  7,   // Pouring
  1895: 20,   // Seyren Windsor
  1896: 20,   // Kathryne Keyron
  1897: 21,   // Baphomet
  1898:  4,   // Zombie
  1899: 12,   // Sword Guardian
  1901:  3,   // Condor
  1902:  6,   // Treasure Box
  1903:  6,   // Treasure Box
  1904: 13,   // Bomb Poring
  1905:  6,   // Barricade
  1906:  6,   // Barricade
  1907:  6,   // Guardian Stone
  1908:  6,   // Guardian Stone
  1909:  6,   // Food Storage
  1910:  6,   // Food Depot
  1911:  6,   // Neutrality Flag
  1912:  6,   // Lion Flag
  1913:  6,   // Eagle Flag
  1914:  6,   // Blue Crystal
  1915:  6,   // Pink Crystal
  1916: 21,   // Satan Morocc
  1917: 21,   // Wounded Morocc
  1918: 21,   // Incarnation of Morocc
  1919: 21,   // Incarnation of Morocc
  1920: 21,   // Incarnation of Morocc
  1921: 21,   // Incarnation of Morocc
  1922: 21,   // Incarnation of Morocc
  1923: 21,   // Incarnation of Morocc
  1924: 21,   // Incarnation of Morocc
  1925: 21,   // Incarnation of Morocc
  1926: 21,   // Jakk
  1927:  9,   // Whisper
  1928: 21,   // Deviruchi
  1929: 21,   // Great Demon Baphomet
  1930: 21,   // Piamette
  1931: 21,   // Wish Maiden
  1932:  1,   // Garden Keeper
  1933: 21,   // Garden Watcher
  1934:  6,   // Blue Flower
  1935:  6,   // Red Flower
  1936:  6,   // Yellow Flower
  1937:  4,   // Constant
  1938:  6,   // Treasure Chest
  1939:  6,   // Treasure Chest
  1940:  6,   // Treasure Chest
  1941:  6,   // Treasure Chest
  1942:  6,   // Treasure Chest
  1943:  6,   // Treasure Chest
  1944:  6,   // Treasure Chest
  1945:  6,   // Treasure Chest
  1946:  6,   // Treasure Chest
  1947: 21,   // Piamette
  1948:  4,   // Egnigem Cenia
  1949:  5,   // Camp Guardian
  1950:  5,   // Camp Guardian
  1951: 25,   // Crystal
  1952: 25,   // Crystal
  1953: 25,   // Crystal
  1954: 25,   // Crystal
  1955:  6,   // Treasure Chest
  1956: 21,   // Naght Sieger
  1957: 10,   // Entweihen Crothen
  1958: 27,   // Thorny Skeleton
  1959: 27,   // Thorn of Recovery
  1960: 27,   // Thorn of Magic
  1961: 10,   // Thorn of Purification
  1962:  2,   // Christmas Thief
  1963: 21,   // New Year Doll
  1964:  6,   // Nightmare
  1965:  6,   // Wild Rose
  1966:  6,   // Doppelganger
  1967:  6,   // Egnigem Cenia
  1968:  6,   // Strouf
  1969:  6,   // Marc
  1970:  6,   // Obeune
  1971:  6,   // Vadon
  1972:  6,   // Marina
  1973:  6,   // Poring
  1974: 20,   // Banshee Master
  1975: 20,   // Beholder master
  1976: 20,   // Cobalt Mineral
  1977: 20,   // Heavy Metaling
  1978: 20,   // Hell Apocalypse
  1979: 21,   // Zakudam
  1980: 21,   // Kublin
  1981: 21,   // Safeguard Chief
  1982:  9,   // Orc Sniper
  1983:  4,   // Depraved Orc Spirit
  1984: 21,   // Shaman Cargalache
  1985: 21,   // Dandelion Member
  1986:  7,   // Tatacho
  1987: 21,   // Centipede
  1988: 10,   // Nepenthes
  1989: 13,   // Hillslion
  1990: 21,   // Hardrock Mammoth
  1991: 21,   // Tendrilion
  1992:  3,   // Cornus
  1993: 21,   // Naga
  1994:  8,   // Luciola Vespa
  1995: 13,   // Pinguicula
  1997:  7,   // Tatacho
  1998: 13,   // Hillslion
  1999: 21,   // Centipede Larva
  2008: 21,   // Woomawang
  2009: 21,   // Woomawang
  2010:  6,   // Ox
  2013:  3,   // Draco
  2014:  6,   // Draco Egg
  2015: 13,   // Dark Pinguicula
  2016:  9,   // Aqua Elemental
  2017: 20,   // Rata
  2018: 20,   // Duneyrr
  2019: 13,   // Ancient Tree
  2020: 13,   // Rhyncho
  2021:  5,   // Phylla
  2022: 21,   // Nidhoggr's Shadow
  2023:  9,   // Dark Shadow
  2024: 20,   // Bradium Golem
  2026: 21,   // Runaway Dandelion Member
  2027: 21,   // Dark Shadow
  2030: 21,   // Hiden Priest
  2031: 21,   // Dandelion
  2042: 10,   // Silver Sniper
  2043:  6,   // Magic Decoy
  2044:  6,   // Magic Decoy
  2045:  6,   // Magic Decoy
  2046:  6,   // Magic Decoy
  2047: 21,   // Naga
  2049:  4,   // Bradium Golem
  2057:  6,   // Cramp
  2068: 21,   // Boitata
  2069: 17,   // Iara
  2070:  4,   // Piranha
  2071:  4,   // Headless Mule
  2072: 17,   // Jaguar
  2073:  3,   // Toucan
  2074:  7,   // Curupira
  2076: 21,   // Shadow of Deception
  2077: 20,   // Shadow of Illusion
  2078: 21,   // Shadow of Pleasure
  2081:  6,   // Strange Hydra
  2082: 20,   // Piranha
};

module.exports = { MONSTER_AI_CODES };
