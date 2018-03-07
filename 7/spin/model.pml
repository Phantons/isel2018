ltl spec {
  [] (((state == ALARM_ON) && (isSomeoneHere == 1)) -> <> (led == 1))
}

#define timeout true

mtype = {ALARM_OFF, ACT_1, ACT_2, ALARM_ON, DESACT_1, DESACT_2};
byte state;
bit button;
bit isSomeoneHere;
bit led;

active proctype alarm_fsm() {
  state = ALARM_OFF;
  do
  :: (state == ALARM_OFF) -> atomic {
    if
    :: (button == 1) -> state = ACT_1; button = 0; led = 0;
    :: (timeout) -> state = ALARM_OFF; button = 0; led = 0;
    fi;
  }
  :: (state == ACT_1) -> atomic {
    if
    :: (button == 1) -> state = ACT_2; button = 0; led = 0;
    :: (timeout) -> state = ALARM_OFF; button = 0; led = 0;
    fi;
  }
  :: (state == ACT_2) -> atomic {
    if
    :: (button == 1) -> state = ALARM_ON; button = 0; led = 0;
    :: (timeout) -> state = ALARM_OFF; button = 0; led = 0;
    fi;
  }
  :: (state == ALARM_ON) -> atomic {
    if
    :: (isSomeoneHere) -> state = ALARM_ON; led = 1; isSomeoneHere = 0;
    :: (button == 1) -> state = DESACT_1; button = 0; led = 0;
    fi;
  }
  :: (state == DESACT_1) -> atomic {
    if
    :: (button == 1) -> state = DESACT_2; button = 0; led = 0;
    :: (timeout) -> state = ALARM_ON; button = 0; led = 0;
    fi;
  }
  :: (state == DESACT_2) -> atomic {
    if
    :: (button == 1) -> state = ALARM_OFF; button = 0; led = 0;
    :: (timeout) -> state = ALARM_ON; button = 0; led = 0;
    fi;
  }
  od;
}

active proctype entorno() {
  do
  :: button = 1
  :: (!button) -> skip;
  :: isSomeoneHere = 1
  :: (!isSomeoneHere) -> skip;
  od;
}
