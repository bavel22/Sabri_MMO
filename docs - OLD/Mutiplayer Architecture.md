CLIENT (UE5)          SERVER (Node.js)          REDIS
     │                        │                    │
     │─Input(WASD/Click)─────►│                    │
     │                        │─Update Pos────────►│
     │                        │◄─Get Nearby───────│
     │◄─Positions─────────────│                    │
     │                        │                    │
     │[Predict locally]        [Broadcast 30Hz]     [Cache positions]