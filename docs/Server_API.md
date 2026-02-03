# Server API Reference

## Overview

Complete REST API documentation for the Node.js/Express server. All endpoints return JSON and use standard HTTP status codes.

## Base URL

```
http://localhost:3001
```

## Authentication

### JWT Token

Most endpoints require a JWT token in the Authorization header:

```
Authorization: Bearer <token>
```

### Obtaining a Token

Login or register to receive a token:

```bash
curl -X POST http://localhost:3001/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"test","password":"pass123"}'
```

Response:
```json
{
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```

---

## Endpoints

### Health Check

#### GET /health

Check server and database status.

**Auth Required:** No

**Response (200):**
```json
{
  "status": "OK",
  "timestamp": "2026-02-03T01:31:24.310Z",
  "message": "Server is running and connected to database"
}
```

**Response (500):**
```json
{
  "status": "ERROR",
  "message": "Database connection failed",
  "error": "connection refused"
}
```

---

### Authentication

#### POST /api/auth/register

Register a new user account.

**Auth Required:** No

**Request:**
```json
{
  "username": "newplayer",
  "email": "player@example.com",
  "password": "securepass123"
}
```

**Validation:**
- Username: 3-50 characters
- Email: Valid email format
- Password: Minimum 8 characters, must contain letter and number

**Response (201):**
```json
{
  "message": "User registered successfully",
  "user": {
    "user_id": 3,
    "username": "newplayer",
    "email": "player@example.com",
    "created_at": "2026-02-03T01:31:24.310Z"
  },
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```

**Errors:**
- `400` - Invalid input (validation failed)
- `409` - Username or email already exists
- `500` - Server error

---

#### POST /api/auth/login

Authenticate user and receive JWT token.

**Auth Required:** No

**Request:**
```json
{
  "username": "testplayer",
  "password": "password123"
}
```

**Response (200):**
```json
{
  "message": "Login successful",
  "user": {
    "user_id": 2,
    "username": "testplayer",
    "email": "test@example.com"
  },
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```

**Errors:**
- `400` - Missing username or password
- `401` - Invalid credentials
- `500` - Server error

---

#### GET /api/auth/verify

Verify JWT token validity.

**Auth Required:** Yes

**Headers:**
```
Authorization: Bearer <token>
```

**Response (200):**
```json
{
  "message": "Token is valid",
  "user": {
    "user_id": 2,
    "username": "testplayer",
    "email": "test@example.com",
    "created_at": "2026-02-01T10:00:00.000Z"
  }
}
```

**Errors:**
- `401` - No token provided
- `403` - Invalid or expired token
- `404` - User not found

---

### Characters

#### GET /api/characters

Get all characters for authenticated user.

**Auth Required:** Yes

**Headers:**
```
Authorization: Bearer <token>
```

**Response (200):**
```json
{
  "message": "Characters retrieved successfully",
  "characters": [
    {
      "character_id": 10,
      "name": "MyHero",
      "class": "warrior",
      "level": 1,
      "x": 123.45,
      "y": 67.89,
      "z": 0.00,
      "health": 100,
      "mana": 100,
      "created_at": "2026-02-02T15:30:00.000Z"
    },
    {
      "character_id": 11,
      "name": "MagicMan",
      "class": "mage",
      "level": 1,
      "x": 0.00,
      "y": 0.00,
      "z": 0.00,
      "health": 100,
      "mana": 100,
      "created_at": "2026-02-02T16:00:00.000Z"
    }
  ]
}
```

**Errors:**
- `401` - Not authenticated
- `500` - Server error

---

#### POST /api/characters

Create a new character.

**Auth Required:** Yes

**Headers:**
```
Authorization: Bearer <token>
Content-Type: application/json
```

**Request:**
```json
{
  "name": "NewHero",
  "characterClass": "warrior"
}
```

**Valid Classes:**
- `warrior` (default if invalid)
- `mage`
- `archer`
- `healer`

**Response (201):**
```json
{
  "message": "Character created successfully",
  "character": {
    "character_id": 12,
    "name": "NewHero",
    "class": "warrior",
    "level": 1,
    "x": 0.00,
    "y": 0.00,
    "z": 0.00,
    "health": 100,
    "mana": 100,
    "created_at": "2026-02-03T01:31:24.310Z"
  }
}
```

**Errors:**
- `400` - Invalid name (2-50 characters required)
- `401` - Not authenticated
- `409` - Character name already exists for this user
- `500` - Server error

---

#### GET /api/characters/:id

Get specific character details.

**Auth Required:** Yes

**Headers:**
```
Authorization: Bearer <token>
```

**Parameters:**
- `id` (path) - Character ID

**Response (200):**
```json
{
  "message": "Character retrieved successfully",
  "character": {
    "character_id": 10,
    "name": "MyHero",
    "class": "warrior",
    "level": 1,
    "x": 123.45,
    "y": 67.89,
    "z": 0.00,
    "health": 100,
    "mana": 100,
    "created_at": "2026-02-02T15:30:00.000Z"
  }
}
```

**Errors:**
- `401` - Not authenticated
- `404` - Character not found
- `500` - Server error

---

#### PUT /api/characters/:id/position

Update character position (auto-saved during gameplay).

**Auth Required:** Yes

**Headers:**
```
Authorization: Bearer <token>
Content-Type: application/json
```

**Parameters:**
- `id` (path) - Character ID

**Request:**
```json
{
  "x": 150.50,
  "y": 75.25,
  "z": 10.00
}
```

**Response (200):**
```json
{
  "message": "Position saved successfully",
  "position": {
    "x": 150.50,
    "y": 75.25,
    "z": 10.00
  }
}
```

**Errors:**
- `400` - Invalid coordinates (must be numbers)
- `401` - Not authenticated
- `404` - Character not found or not owned by user
- `500` - Server error

---

## HTTP Status Codes

| Code | Meaning | Usage |
|------|---------|-------|
| 200 | OK | Successful GET/PUT |
| 201 | Created | Successful POST (resource created) |
| 400 | Bad Request | Invalid input data |
| 401 | Unauthorized | Missing authentication |
| 403 | Forbidden | Invalid/expired token |
| 404 | Not Found | Resource doesn't exist |
| 409 | Conflict | Duplicate resource |
| 500 | Internal Server Error | Server error |

## Rate Limiting

All `/api/*` endpoints are rate limited:

- **Window:** 15 minutes
- **Max Requests:** 100 per IP
- **Exceed Limit:** `429 Too Many Requests`

## CORS

Cross-Origin Resource Sharing is enabled for development:

```javascript
app.use(cors());
```

Allows requests from any origin (localhost development only).

## Testing with cURL

### Health Check
```bash
curl http://localhost:3001/health
```

### Register
```bash
curl -X POST http://localhost:3001/api/auth/register \
  -H "Content-Type: application/json" \
  -d '{"username":"test","email":"test@test.com","password":"password123"}'
```

### Login
```bash
curl -X POST http://localhost:3001/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"test","password":"password123"}'
```

### Get Characters (with token)
```bash
curl http://localhost:3001/api/characters \
  -H "Authorization: Bearer <token>"
```

### Create Character
```bash
curl -X POST http://localhost:3001/api/characters \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{"name":"Hero","characterClass":"warrior"}'
```

### Update Position
```bash
curl -X PUT http://localhost:3001/api/characters/10/position \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{"x":100.0,"y":50.0,"z":0.0}'
```

## Files

- `server/src/index.js` - All endpoint implementations
