ltl spec {
  [] (((state == ALARM_ON) && (flagCorrectCode)) -> <> (state == ALARM_OFF)) &&
  [] (((state == ALARM_OFF) && (flagCorrectCode)) -> <> (state == ALARM_ON)) &&
  [] (((state == ALARM_ON) && (isSomeoneHere)) -> (<> (led == 1)))
}

mtype = {ALARM_OFF, ALARM_ON};
byte state;
bit isSomeoneHere;
bit flagCorrectCode;
bit led;

active proctype alarm_fsm() {
  state = ALARM_OFF;
  do
  :: (state == ALARM_OFF) -> atomic {
    if
    :: (flagCorrectCode) -> state = ALARM_ON; flagCorrectCode = 0; led = 0;
    fi;
  }
  :: (state == ALARM_ON) -> atomic {
    if
    :: (isSomeoneHere) -> state = ALARM_ON; led = 1; isSomeoneHere = 0;
    :: (flagCorrectCode) -> state = ALARM_OFF; flagCorrectCode = 0; led = 0;
    fi;
  }
  od;
}

active proctype entorno() {
  do
  :: isSomeoneHere = 1
  :: (!isSomeoneHere) -> skip;
  od;
}

active proctype checkcode_fsm() {
  do
  :: flagCorrectCode = 1;
  :: (!flagCorrectCode) -> skip;
  od
}
