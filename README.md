# CCH (C-CHAT)

A simple chat server/client written in c.
It all comes down to a single binary named cch.

### cch cli

## cch protocol documentation

| syntax         | description |
| -------------- | ----------- |
| <username>     | The username which should be used (max MAX_USERNAME_LEN)
| -s             | Enables server mode
| -f <filename>  | If given, all messages will be stored up into <filename> and they will also be restored next time
| -p <port>      | The port to which to connect to
| -h <hostname>  | The host to which to connect to
    

### client -> server

- P_MSG (0)
    | type            | description                        |
    |-----------------|------------------------------------|
    | u8              | id P_MSG (0)                       |
    | u8[MAX_BUF_LEN] | the message which should be sended |

- P_USER_LIST (1)
    | type            | description        |
    |-----------------|--------------------|
    | u8              | id P_USER_LIST (1) |

- P_MSG_LIST (4)
    | type            | description       |
    |-----------------|-------------------|
    | u8              | id P_MSG_LIST (4) |

- P_USER_RENAME (6)
    | type                 | description          |
    |----------------------|----------------------|
    | u8                   | id P_USER_RENAME (6) |
    | u8[MAX_USERNAME_LEN] | the new username     |

### server -> client
- P_MSG (0)
    | type                 | description      |
    |----------------------|------------------|
    | u8                   | id P_MSG (0)     |
    | u8[MAX_USERNAME_LEN] | username         |
    | u8[MAX_BUF_LEN]      | message          |

- P_USER_LIST (1)
    | type            | description      |
    |-----------------|------------------|
    | u8              | id P_MSG (0)     |
    | u8[128]         |  |

- P_USER_DISCONNECT (2)
    | type            | description      |
    |-----------------|------------------|
    | u8              | id P_MSG (0)     |
    | u8[128]         |  |

P_USER_CONNECT 3
P_MSG_LIST 4
P_SERVER_END 5
P_USER_RENAME 6
