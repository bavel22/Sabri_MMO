# Server Testing Quick Start Guide

## 🚀 QUICK START - Server Testing Framework

### **Step 1: Compile**
```bash
C:\Sabri_MMO\compile_with_tests.bat
```

### **Step 2: Open UE5 Editor**
- Navigate to: `C:/Sabri_MMO/client/SabriMMO/SabriMMO.uproject`
- Double-click to open in UE5 Editor

### **Step 3: Open Test Map**
- Content Browser → `Content/SabriMMO/Levels/`
- Open `L_AutomationTestMap.umap`

### **Step 4: Add Server Test Actor**
- Content Browser → `C++ Classes` → `ASabriMMOServerTests`
- **Drag and drop** into the level

### **Step 5: Configure (Optional)**
- Select `ASabriMMOServerTests` in the level
- In Details panel, you can adjust:
  - `InitialDelay`: 3.0f (time before tests start)
  - `DelayBetweenTests`: 1.0f (time between tests)
  - `TestServerURL`: "http://localhost:3001" (default)

### **Step 6: Run Tests**
- Press **Play** button
- Tests will automatically start after 3 seconds
- Watch **Output Log** and **on-screen messages**

## 📊 Expected Results

### **On-Screen Display:**
```
=== SABRI_MMO SERVER TEST SUITE ===
✅ PASS: Server Health Check
✅ PASS: Server Authentication  
✅ PASS: Character API
✅ PASS: Inventory API
Results: 4/4 passed, 0 failed
ALL SERVER TESTS PASSED
```

### **Output Log:**
```
LogServerTests: Server Test Runner placed. Tests will start in 3.0 seconds...
[PASS] Server Health Check
[PASS] Server Authentication
[PASS] Character API
[PASS] Inventory API
Results: 4/4 passed, 0 failed
ALL SERVER TESTS PASSED
```

## 🔧 **Troubleshooting**

### **If Tests Don't Start:**
1. Check `ASabriMMOServerTests` appears in C++ Classes
2. Ensure server is running on `localhost:3001`
3. Check Output Log for errors

### **If Tests Fail:**
1. **Server Not Running**: Start Node.js server first
2. **Wrong Port**: Update `TestServerURL` in Details panel
3. **Network Issues**: Check firewall and connectivity

### **If Compilation Fails:**
1. Run `C:/Sabri_MMO/compile_with_tests.bat` again
2. Check Output Log for compilation errors

## 🎯 **Test Details**

### **What Each Test Does:**

#### **1. Server Health Check**
- Tests `/health` endpoint
- Validates server status and database connectivity
- Expected: `{"status":"OK","message":"Server is running and connected to database"}

#### **2. Server Authentication**  
- Tests `/api/auth/login` endpoint
- Validates JWT token generation
- Uses test credentials: `testplayer` / `password123`

#### **3. Character API**
- Tests `/api/characters` endpoint  
- Validates character list retrieval
- Checks for characters array in response

#### **4. Inventory API**
- Tests `/api/inventory/1` endpoint
- Validates inventory structure
- Checks for items array in response

## 🎯 **Ready for Testing**

The server testing framework is **100% ready to use** and provides comprehensive validation of your Sabri_MMO backend systems! 🚀
