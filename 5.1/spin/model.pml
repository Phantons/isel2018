ltl spec {
  [] (((state == LED_ON) && (button == 1)) -> <> (state == LED_ON)) &&
  [] (((state == LED_OFF) && (button == 1)) -> <> (state == LED_ON)) &&
  [] (((state == LED_ON) && (timeout)) -> <> (state == LED_OFF))
}

#define timeout true

mtype = {LED_ON, LED_OFF};
byte state;
bit button;

active proctype lampara_fsm() {
  state = LED_OFF;
  do
  :: (state == LED_OFF) -> atomic {
    if
    :: (button) -> state = LED_ON; button = 0;
    fi;
  }
  :: (state == LED_ON) -> atomic {
    if
    :: (button) -> state = LED_ON; button = 0;
    :: (timeout) -> state = LED_OFF;
    fi;
  }
  od;
}

active proctype entorno() {
  do
  :: button = 1;
  :: (!button) -> skip;
  od;
}
