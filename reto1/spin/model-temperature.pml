ltl spec {
  [] ((temp > TEMP_H) -> <> (state == REF_MODE_3)) &&
  [] ((temp < TEMP_L && slope < HIGH) -> <> (state == NO_REFRIGERATION))
}

#define timeout true

mtype = {NO_REFRIGERATION, REF_MODE_1, REF_MODE_2, REF_MODE_3};
byte state;
byte slope;
byte temp;

byte HIGH = 3;

byte TEMP_H = 24;
byte TEMP_M = 23;
byte TEMP_L = 22;

active proctype ref_fsm() {
  state = NO_REFRIGERATION;
  do
  :: (state == NO_REFRIGERATION) -> atomic {
    if
    :: (slope > HIGH || temp > TEMP_L) -> state = REF_MODE_1;
    fi
  }
  :: (state == REF_MODE_1) -> atomic {
    if
    :: ((slope > HIGH && temp > TEMP_L) || temp > TEMP_M) -> state = REF_MODE_2;
    :: (temp < TEMP_L && slope < HIGH) -> state = NO_REFRIGERATION;
    fi
  }
  :: (state == REF_MODE_2) -> atomic {
    if
    :: ((slope > HIGH && temp > TEMP_M) || temp > TEMP_H) -> state = REF_MODE_3;
    :: (temp < TEMP_M && slope < HIGH) -> state = REF_MODE_1;
    fi
  }
  :: (state == REF_MODE_3) -> atomic {
    if
    :: (temp < TEMP_M && slope < HIGH) -> state = REF_MODE_1;
    fi
  }
  od;
}

active proctype entorno() {
  do
  :: if
    :: temp = 18;
    :: temp = 19;
    :: temp = 20;
    :: temp = 21;
    :: temp = 22;
    :: temp = 23;
    :: temp = 24;
    :: temp = 25;
    :: temp = 26;
    :: temp = 27;
    fi;
    printf("state %d, slope %d, temp %d\n", state, slope, temp);
  od;
}

active proctype entorno2() {
  do
  :: slope = 0;
  :: slope = 1;
  :: slope = 2;
  :: slope = 3;
  :: slope = 4;
  :: slope = 5;
  :: slope = 6;
  :: slope = 7;
  :: slope = 8;
  :: slope = 9;
  od;
}

