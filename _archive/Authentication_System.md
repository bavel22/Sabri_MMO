# Authentication System

## Overview

JWT-based authentication system providing secure user login and registration with bcrypt password hashing. The system uses a stateless token approach where the server validates JWT tokens on each request.

## Architecture

```
UE5 Client (WBP_LoginScreen)
    ↓
HttpManager::LoginUser()
    ↓
POST /api/auth/login
    ↓
Node.js Server (bcrypt compare)
    ↓
JWT Token Generated
    ↓
UE5 Client (Store token in GameInstance)
    ↓
Subsequent requests include JWT header
```

## Server Implementation

### Dependencies
```javascript
const bcrypt = require('bcrypt');
const jwt = require('jsonwebtoken');
```

### Password Hashing

**Registration:**
```javascript
const saltRounds = 10;
const passwordHash = await bcrypt.hash(password, saltRounds);
```

**Login Verification:**
```javascript
const validPassword = await bcrypt.compare(password, user.password_hash);
```

### JWT Token Generation

```javascript
const token = jwt.sign(
    { user_id: user.user_id, username: user.username },
    process.env.JWT_SECRET,
    { expiresIn: '24h' }
);
```

### JWT Middleware

```javascript
function authenticateToken(req, res, next) {
    const authHeader = req.headers['authorization'];
    const token = authHeader && authHeader.split(' ')[1]; // Bearer TOKEN
    
    if (!token) {
        return res.status(401).json({ error: 'Access token required' });
    }
    
    jwt.verify(token, process.env.JWT_SECRET, (err, user) => {
        if (err) {
            return res.status(403).json({ error: 'Invalid or expired token' });
        }
        req.user = user;
        next();
    });
}
```

## Client Implementation (C++)

### MMOGameInstance

**Stored Authentication Data:**
```cpp
UPROPERTY(BlueprintReadOnly, Category = "MMO Auth")
FString AuthToken;

UPROPERTY(BlueprintReadOnly, Category = "MMO Auth")
FString Username;

UPROPERTY(BlueprintReadOnly, Category = "MMO Auth")
int32 UserId;

UPROPERTY(BlueprintReadOnly, Category = "MMO Auth")
bool bIsLoggedIn;
```

**Event Dispatchers:**
```cpp
UPROPERTY(BlueprintAssignable, Category = "MMO Events")
FOnLoginSuccess OnLoginSuccess;

UPROPERTY(BlueprintAssignable, Category = "MMO Events")
FOnLoginFailed OnLoginFailed;
```

**Functions:**
```cpp
UFUNCTION(BlueprintCallable, Category = "MMO Auth")
void SetAuthData(const FString& InToken, const FString& InUsername, int32 InUserId);

UFUNCTION(BlueprintCallable, Category = "MMO Auth")
void ClearAuthData();

UFUNCTION(BlueprintPure, Category = "MMO Auth")
bool IsAuthenticated() const;

UFUNCTION(BlueprintPure, Category = "MMO Auth")
FString GetAuthHeader() const;
```

### HttpManager

**Login User:**
```cpp
static void LoginUser(UObject* WorldContextObject, const FString& Username, const FString& Password);
```

**Implementation Flow:**
1. Create HTTP POST request to `/api/auth/login`
2. Send JSON payload: `{"username":"...","password":"..."}`
3. On response, parse JWT token and user data
4. Call `GameInstance->SetAuthData()`
5. Broadcast `OnLoginSuccess` or `OnLoginFailed`

## Blueprint Flow

### WBP_LoginScreen

**Event Construct:**
```
Set Input Mode UI Only
Set Show Mouse Cursor: true
Set User Focus: TB_Username
```

**Btn_Login OnClicked:**
```
Get TB_Username → Get Text → To String → Username variable
Get TB_Password → Get Text → To String → Password variable

Branch (Username != "" AND Password != "")
    ├─ FALSE → Show validation error
    └─ TRUE → HttpManager::LoginUser(World, Username, Password)
```

### BP_GameFlow

**Event BeginPlay:**
```
Create WBP_LoginScreen → Add to Viewport → Store: LoginWidget

Get Game Instance → Cast to MMOGameInstance
    ↓
Bind Event to OnLoginSuccess → HandleLoginSuccess
Bind Event to OnLoginFailed → HandleLoginFailed
```

**HandleLoginSuccess:**
```
Get LoginWidget → Is Valid → Branch
    ↓
Remove from Parent
Set Input Mode Game and UI
Set Show Mouse Cursor: false
Clear LoginWidget variable
[Continue to character selection or game]
```

**HandleLoginFailed:**
```
Get LoginWidget → Cast to WBP_LoginScreen
    ↓
Get Txt_Error → Set Text: "Invalid username or password"
Get Txt_Error → Set Visibility: Visible
```

## API Endpoints

### POST /api/auth/login

**Request:**
```json
{
    "username": "testplayer",
    "password": "password123"
}
```

**Success Response (200):**
```json
{
    "message": "Login successful",
    "user": {
        "user_id": 2,
        "username": "testplayer",
        "email": "player@example.com"
    },
    "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```

**Error Responses:**
- `400` - Missing username or password
- `401` - Invalid credentials
- `500` - Server error

### POST /api/auth/register

**Request:**
```json
{
    "username": "newplayer",
    "email": "new@example.com",
    "password": "password123"
}
```

**Validation:**
- Username: 3-50 characters
- Email: Valid format
- Password: Minimum 8 characters, must contain letter and number

**Success Response (201):**
```json
{
    "message": "User registered successfully",
    "user": { ... },
    "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```

### GET /api/auth/verify

**Headers:**
```
Authorization: Bearer <jwt_token>
```

**Success Response (200):**
```json
{
    "message": "Token is valid",
    "user": { ... }
}
```

## Security Features

### Password Security
- bcrypt with 10 salt rounds
- 60-character hash storage
- Timing attack resistant comparison

### JWT Security
- 24-hour expiration
- HMAC SHA256 signing
- Server-side secret (not exposed)
- Payload contains user_id and username only

### API Security
- Rate limiting on auth endpoints
- Input validation on all fields
- SQL injection protection via parameterized queries
- CORS enabled for localhost development

## Error Handling

### Client-Side
- Empty field validation before sending
- Network failure detection
- Clear error messaging to user
- Token storage in memory only

### Server-Side
- Generic "Invalid credentials" message (no user enumeration)
- Proper HTTP status codes
- Detailed server logs (without exposing sensitive data)

## Files

### Server
- `server/src/index.js` - Authentication endpoints

### Client C++
- `client/SabriMMO/Source/SabriMMO/MMOGameInstance.h`
- `client/SabriMMO/Source/SabriMMO/MMOGameInstance.cpp`
- `client/SabriMMO/Source/SabriMMO/HttpManager.h`
- `client/SabriMMO/Source/SabriMMO/HttpManager.cpp`

### Client Blueprints
- `client/SabriMMO/Content/UI/WBP_LoginScreen.uasset`
- `client/SabriMMO/Content/Blueprints/BP_GameFlow.uasset`
