# Bug Fix Notes

## Bug: Camera Uses Wrong Pawn on First Character Creation

**Date Fixed**: 2026-02-03  
**Severity**: Medium  
**Status**: ✅ Fixed

### Problem
When creating a new character and entering the world for the first time, the game used the incorrect camera (default ThirdPersonCharacter camera instead of the custom SpringArm camera). However, after closing and relaunching the game, logging back in used the correct camera.

### Root Cause
The Level Blueprint had **two different Spawn Actor nodes** for different scenarios:
1. **First spawn** (new character creation) → Spawned `BP_ThirdPersonCharacter`
2. **Subsequent spawns** (existing character login) → Spawned `BP_MMOCharacter`

The first spawn node was never updated when the custom character blueprint was created.

### Solution
Changed both Spawn Actor nodes in the Level Blueprint to use `BP_MMOCharacter`:

1. Open **lvl_ThirdPerson** → **Level Blueprint**
2. Find all **Spawn Actor** nodes
3. Change **Class** pin on each node to: `BP_MMOCharacter`
4. **Compile** and **Save**

### Prevention
When creating a custom character blueprint:
1. Search for ALL Spawn Actor nodes in Level Blueprint
2. Update the Class on every spawn node
3. Test both paths: new character creation AND existing character login

### Files Modified
- `lvl_ThirdPerson` (Level Blueprint)

### Related Documentation
- [Camera System](Camera_System.md)
- [Top-Down Movement System](Top_Down_Movement_System.md)

---

## Known Issues & Workarounds

### Issue: None currently

All reported bugs have been resolved. If you encounter issues:
1. Check this document first
2. Verify Input Mapping Context is added in BeginPlay
3. Confirm SpringArm Inherit settings are all FALSE
4. Check both Spawn Actor nodes use BP_MMOCharacter

---

**Last Updated**: 2026-02-03  
**Version**: 1.0.1
