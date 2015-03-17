#ifndef CHARACTER_RACE_TBL_H_INCLUDED
#define CHARACTER_RACE_TBL_H_INCLUDED

#define RACE_TABLE_SQL "CREATE TABLE RACE_TABLE( \
        RACE_ID             INTEGER PRIMARY KEY     NOT NULL, \
        RACE_NAME           TEXT, \
        RACE_DESCRIPTION    TEXT, \
        INITIAL_EMU         INT,  \
        EMU_MULTIPLIER      REAL, \
        INITIAL_VISPROX     INT,  \
        VISPROX_MULTIPLIER  REAL, \
        INITIAL_CHATPROX    INT,  \
        CHATPROX_MULTIPLIER REAL, \
        INITIAL_NIGHTVIS    REAL, \
        NIGHTVIS_MULTIPLIER REAL, \
        CHAR_COUNT          INT)"

#endif // CHARACTER_RACE_TBL_H_INCLUDED