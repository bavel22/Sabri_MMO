# Authentication Flow

## Overview

Sabri_MMO uses **JWT (JSON Web Tokens)** for authentication. The flow spans three layers: UE5 Blueprints → C++ `UHttpManager` → Node.js server → PostgreSQL.

## Complete Login Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│ 1. USER enters username + password in WBP_LoginScreen              │
│    → Button click calls UHttpManager::LoginUser()                  │
└─────────────────────────────────┬───────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 2. C++ sends POST /api/auth/login {username, password}             │
│    via FHttpModule (HTTP/1.1 to localhost:3001)                     │
└─────────────────────────────────┬───────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 3. SERVER receives request                                          │
│    → SELECT password_hash FROM users WHERE username = $1            │
│    → bcrypt.compare(password, hash)                                │
│    → If valid: jwt.sign({user_id, username}, secret, {expiresIn})  │
│    → Response: {token, user: {user_id, username, email}}           │
│    → UPDATE users SET last_login = NOW()                           │
└─────────────────────────────────┬───────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 4. C++ OnLoginResponse() parses JSON                                │
│    → Extracts token, username, user_id from response               │
│    → Calls GameInstance->SetAuthData(token, username, userId)      │
│    → GameInstance sets bIsLoggedIn = true                          │
│    → GameInstance broadcasts OnLoginSuccess delegate               │
└─────────────────────────────────┬───────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│ 5. BLUEPRINT receives OnLoginSuccess                                │
│    → BP_GameFlow calls UHttpManager::GetCharacters()               │
│    → Passes Authorization: Bearer <token> header                   │
│    → Server validates JWT, returns character list                  │
│    → C++ calls GameInstance->SetCharacterList()                    │
│    → GameInstance broadcasts OnCharacterListReceived               │
│    → WBP_CharacterSelect populates UI                             │
└─────────────────────────────────────────────────────────────────────┘
```

## Token Storage

- **Location**: `UMMOGameInstance::AuthToken` (FString)
- **Lifetime**: Persists across level loads (GameInstance survives level transitions)
- **Expiry**: 24 hours (set by server `jwt.sign(..., {expiresIn: '24h'})`)
- **Usage**: Added to HTTP headers via `GameInstance->GetAuthHeader()` → returns `"Bearer " + AuthToken`

## JWT Token Structure

```
Header: {"alg": "HS256", "typ": "JWT"}
Payload: {"user_id": 1, "username": "player1", "iat": ..., "exp": ...}
Signature: HMAC-SHA256(header.payload, JWT_SECRET)
```

## Server-Side JWT Middleware

```javascript
function authenticateToken(req, res, next) {
    const authHeader = req.headers['authorization'];
    const token = authHeader && authHeader.split(' ')[1];
    
    if (!token) return res.status(401).json({ error: 'Access token required' });
    
    jwt.verify(token, process.env.JWT_SECRET, (err, user) => {
        if (err) return res.status(403).json({ error: 'Invalid or expired token' });
        req.user = user;  // {user_id, username}
        next();
    });
}
```

Applied to: `GET /api/characters`, `POST /api/characters`, `GET /api/characters/:id`, `PUT /api/characters/:id/position`, `GET /api/auth/verify`

## Registration Flow

```
WBP_LoginScreen (Register button)
    → UHttpManager::RegisterUser(username, email, password)
    → POST /api/auth/register
    → Server validates input (validateRegisterInput middleware)
    → bcrypt.hash(password, 10) → store in DB
    → Returns token (auto-login after register)
    → Success (201): Log shows "User registered successfully"
    → Error (409): "Username or email already exists"
```

## Password Security

| Aspect | Implementation |
|--------|---------------|
| **Hashing** | bcrypt with 10 salt rounds |
| **Storage** | `users.password_hash` VARCHAR(255) |
| **Comparison** | `bcrypt.compare(plaintext, hash)` |
| **Transport** | Plaintext over HTTP (localhost only — HTTPS required for production) |

## Input Validation

| Field | Validation | Error |
|-------|-----------|-------|
| Username | 3–50 chars, required | 400: "Username must be between 3 and 50 characters" |
| Email | Valid email regex, required | 400: "Valid email is required" |
| Password | 8+ chars, letter + number | 400: "Password must contain at least one letter and one number" |

## Security Considerations

- **Rate Limiting**: 100 requests / 15 minutes per IP on `/api/*`
- **No Socket.io Auth**: Socket.io connections currently don't validate JWT (token is sent in `player:join` but not verified server-side)
- **Production TODO**: Enable HTTPS, validate JWT on Socket.io handshake, add CSRF protection

---

**Last Updated**: 2026-02-17
