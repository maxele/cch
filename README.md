# CCH (C-CHAT)

A simple chat server/client written in c.
It all comes down to a single binary named cch.

### cch cli

SERVER:
Für den server können sie nur den port (-p) und ein file angeben, in dem alle
nachrichten gespeichert werden. Sie werden auch bei erneuten ausführen des
Servers in die msg_list gespeichert, das bedeutet wenn sich ein neuer client
verbindet, werden ihm alle alten nachrichten zugesendet.

CLIENT:
Wenn man cch als client verwenden will, muss man 'cch <username>' angeben.
Der default port PORT ist in cch.h als 25555 definiert. Wenn sie einen anderen
port verwenden wollen, können sie mit -p <port> den port überscreiben. 
Wenn sie sich nicht nur zu localhost verbinden wollen, müssen sie mit
-h <hostname> einen alternativen hostname angeben. Sie können entweder
die ip addresse (z.B. 127.0.0.1) oder einen name (z.B. localhost) übergeben.

Wenn sie sich mit einem server verbunden haben, werden ihnen einige funktionen
zur verfügung gegeben. Sie können mit /help die liste von verfügbaren befehlen
ausgeben, aber hier ist trotzdem die liste nochmals mit erklährungen.

- /help oder /h:
    /help gibt die liste der befehle aus.
- /list oder /l:
    /list gibt die liste der verbundenen benutzern aus.
- /prevmsg oder /pr:
    /prevmsg gibt alle nachrichten (maximum in cch.h als MSG_LIST_MAX) zurück.
- /clear oder /clr:
    /clear leert den bildschirm.
- /quit or /q:
    /quit schließt die verbindung mit dem server
- /rn <name>:
    Wenn sie '/rn <name>' eingeben werden sie in <name> unbenennt.


## cch protocol documentation

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

- P_USER_CONNECT (3)
- P_MSG_LIST (4)
- P_SERVER_END (5)
- P_USER_RENAME (6)
