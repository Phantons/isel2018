ltl spec {
  [] (((state == LIGHTS_OFF) && (isSomeoneHere)) -> <> (state == LIGHTS_ON)) &&
  [] (((state == LIGHTS_ON) && (!isSomeoneHere && timeout)) -> <> (state == LIGHTS_OFF)) &&
}

ltl spec2 {
}

#define timeout true

mtype = {LIGHTS_OFF, LIGHTS_ON};

byte state;
bit isSomeoneHere;

active proctype alarm_fsm() {
  state = 0;
  isSomeoneHere = 0;
  do
  :: (state == LIGHTS_OFF) -> atomic {
    if
    :: (isSomeoneHere) -> isSomeoneHere = 0; state = LIGHTS_ON;
    fi
  }
  :: (state == 1) -> atomic {
    if
    :: (!isSomeoneHere && timeout) -> state = LIGHTS_OFF;
    :: (isSomeoneHere) -> isSomeoneHere = 0;
    fi
  }
  od
}

active proctype entorno() {
  do
    :: isSomeoneHere = 1;
    :: (!isSomeoneHere) -> skip;
  od
}

