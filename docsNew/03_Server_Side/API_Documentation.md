# REST API Documentation

## Base URL

```
http://localhost:3001
```

## Authentication

All protected endpoints require a JWT token in the `Authorization` header:

```
Authorization: Bearer <jwt_token>
```

Tokens are obtained via `/api/auth/login` or `/api/auth/register` and expire after 24 hours.

## Rate Limiting

All `/api/*` endpoints are rate-limited to **100 requests per 15 minutes per IP**.

---

## Health Check

### GET /health

**Auth**: None  
**Purpose**: Verify server is running and database is connected.

**Response (200)**:
```json
{
    "status": "OK",
    "timestamp": "2026-02-17T10:00:00.000Z",
    "message": "Server is running and connected to database"
}
```

**Response (500)**:
```json
{
    "status": "ERROR",
    "message": "Database connection failed",
    "error": "connection refused"
}
```

---

## Authentication Endpoints

### POST /api/auth/register

**Auth**: None  
**Purpose**: Create a new user account.

**Request Body**:
```json
{
    "username": "string (3-50 chars)",
    "email": "string (valid email)",
    "password": "string (8+ chars, must contain letter + number)"
}
```

**Validation** (middleware `validateRegisterInput`):
- Username: 3–50 characters, required
- Email: Valid email format, required
- Password: 8+ characters, must contain at least one letter and one number

**Response (201)**:
```json
{
    "message": "User registered successfully",
    "user": {
        "user_id": 1,
        "username": "player1",
        "email": "player1@example.com",
        "created_at": "2026-02-17T10:00:00.000Z"
    },
    "token": "eyJhbGciOiJIUzI1NiIs..."
}
```

**Error Responses**:
- **400**: Invalid input (username length, email format, password strength)
- **409**: `{"error": "Username or email already exists"}`
- **500**: `{"error": "Registration failed"}`

### POST /api/auth/login

**Auth**: None  
**Purpose**: Authenticate and receive JWT token.

**Request Body**:
```json
{
    "username": "string",
    "password": "string"
}
```

**Response (200)**:
```json
{
    "message": "Login successful",
    "user": {
        "user_id": 1,
        "username": "player1",
        "email": "player1@example.com"
    },
    "token": "eyJhbGciOiJIUzI1NiIs..."
}
```

**Error Responses**:
- **400**: `{"error": "Username and password are required"}`
- **401**: `{"error": "Invalid credentials"}`
- **500**: `{"error": "Login failed"}`

**Side Effects**: Updates `users.last_login` timestamp.

### GET /api/auth/verify

**Auth**: Required (JWT)  
**Purpose**: Verify token validity and get user info.

**Response (200)**:
```json
{
    "message": "Token is valid",
    "user": {
        "user_id": 1,
        "username": "player1",
        "email": "player1@example.com",
        "created_at": "2026-02-17T10:00:00.000Z"
    }
}
```

**Error Responses**:
- **401**: `{"error": "Access token required"}`
- **403**: `{"error": "Invalid or expired token"}`
- **404**: `{"error": "User not found"}`

---

## Character Endpoints

### GET /api/characters

**Auth**: Required (JWT)  
**Purpose**: List all characters for the authenticated user.

**Response (200)**:
```json
{
    "message": "Characters retrieved successfully",
    "characters": [
        {
            "character_id": 1,
            "name": "TestHero",
            "class": "warrior",
            "level": 1,
            "x": 100.5,
            "y": -200.3,
            "z": 300.0,
            "health": 100,
            "mana": 100,
            "created_at": "2026-02-17T10:00:00.000Z"
        }
    ]
}
```

### POST /api/characters

**Auth**: Required (JWT)  
**Purpose**: Create a new character.

**Request Body**:
```json
{
    "name": "string (2-50 chars)",
    "characterClass": "warrior|mage|archer|healer|priest"
}
```

**Response (201)**:
```json
{
    "message": "Character created successfully",
    "character": {
        "character_id": 2,
        "name": "NewHero",
        "class": "mage",
        "level": 1,
        "x": 0,
        "y": 0,
        "z": 0,
        "health": 100,
        "mana": 100,
        "created_at": "2026-02-17T10:00:00.000Z"
    }
}
```

**Error Responses**:
- **400**: `{"error": "Character name must be between 2 and 50 characters"}`
- **409**: `{"error": "You already have a character with this name"}`

**Notes**:
- Invalid or missing `characterClass` defaults to `"warrior"`
- New characters spawn at position (0, 0, 0) with 100 HP/MP

### GET /api/characters/:id

**Auth**: Required (JWT)  
**Purpose**: Get a specific character (must belong to authenticated user).

**Response (200)**:
```json
{
    "message": "Character retrieved successfully",
    "character": { ... }
}
```

**Error Responses**:
- **404**: `{"error": "Character not found"}`

### PUT /api/characters/:id/position

**Auth**: Required (JWT)  
**Purpose**: Save character world position.

**Request Body**:
```json
{
    "x": 150.5,
    "y": -300.2,
    "z": 300.0
}
```

**Response (200)**:
```json
{
    "message": "Position saved successfully",
    "position": { "x": 150.5, "y": -300.2, "z": 300.0 }
}
```

**Validation**:
- `x`, `y`, `z` must all be numbers
- Character must belong to authenticated user

**Error Responses**:
- **400**: `{"error": "Invalid coordinates. x, y, z must be numbers"}`
- **404**: `{"error": "Character not found"}`

---

## Test Endpoint

### GET /api/test

**Auth**: None  
**Purpose**: Simple connectivity test.

**Response (200)**:
```json
{
    "message": "Hello from MMO Server!"
}
```

---

## JWT Token Structure

**Algorithm**: HS256  
**Expiry**: 24 hours  
**Payload**:
```json
{
    "user_id": 1,
    "username": "player1",
    "iat": 1739793600,
    "exp": 1739880000
}
```

## Client-Side Usage (UE5 C++)

The `UHttpManager` class in `MMOHttpManager.cpp` consumes these endpoints:

| C++ Function | Endpoint | Method |
|-------------|----------|--------|
| `HealthCheck()` | `/health` | GET |
| `RegisterUser()` | `/api/auth/register` | POST |
| `LoginUser()` | `/api/auth/login` | POST |
| `GetCharacters()` | `/api/characters` | GET |
| `CreateCharacter()` | `/api/characters` | POST |
| `SaveCharacterPosition()` | `/api/characters/:id/position` | PUT |

---

**Last Updated**: 2026-02-17
