ltl spec {
  [] (((state == 1) && (button)) -> <> (state == 0)) &&
  [] (((state == 0) && (button)) -> <> (state == 1)) &&
  [] (((state == 1) && (isSomeoneHere)) -> (<> (led == 1)))
}

ltl spec2 {
}

//mtype = {ALARM_OFF, ALARM_ON};
byte state;
bit button;
bit isSomeoneHere;
bit led;

active proctype alarm_fsm() {
  state = 0;
  isSomeoneHere = 0;
  led = 0;
  button = 0;
  do
  :: (state == 0) -> atomic {
    if
    :: (button) -> button = 0; state = 1;
    fi
  }
  :: (state == 1) -> atomic {
    if
    :: (button) ->  button = 0; led = 0; state = 0;
    :: (isSomeoneHere) -> isSomeoneHere = 0; led = 1;
    fi
  }
  od
}

active proctype entorno() {
  do
    :: button = 1;
    :: (!button) -> skip;
    :: isSomeoneHere = 1;
    :: (!isSomeoneHere) -> skip;
  od
}
