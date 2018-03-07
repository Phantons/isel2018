ltl spec {
  [] (((button == 1)) -> <> (isPlacingCode == 1)) &&
  [] (((code_index == 3 && codeIsCorrect == 1)) -> <> (flagCorrectCode == 1 && isPlacingCode == 0 && code_index == 0)) &&
  [] (((code_index == 3 && codeIsCorrect == 0)) -> <> (flagCorrectCode == 0 && isPlacingCode == 0 && code_index == 0))
}

#define timeout true
#define CODE_LENGTH 3

mtype = {CHECK_CODE};
byte state;
bit button;
bit isPlacingCode;
bit codeIsCorrect;
bit flagCorrectCode;
byte i;
byte code_index;
byte code_inserted[CODE_LENGTH];
byte CODE[CODE_LENGTH] = {1, 2, 3};

active proctype checkcode_fsm() {
  state = CHECK_CODE;
  code_index = 0;
  isPlacingCode = 0;
  do
  :: codeIsCorrect = 1;
  :: (state == CHECK_CODE) -> atomic {

    for (i : 1 .. CODE_LENGTH - 1) {
      if
      :: CODE[i] != code_inserted[CODE_LENGTH] -> codeIsCorrect = 0; break;
      :: else -> skip
      fi
    }

    if
    :: (button == 1) -> isPlacingCode = 1; button = 0; code_inserted[code_index]++;
    :: (code_index == 3 && codeIsCorrect == 1) -> flagCorrectCode = 1; button = 0; code_index = 0; isPlacingCode = 0;
    :: (code_index == 3 && codeIsCorrect == 0) -> flagCorrectCode = 0; button = 0; code_index = 0; isPlacingCode = 0;
    :: (timeout) -> code_index++;
    fi;
  }
  od;
}

active proctype entorno() {
  do
  :: button = 1
  :: (!button) -> skip;
  od;
}

active proctype alarm_fsm() {
  do
  :: flagCorrectCode -> flagCorrectCode = 0;
  :: (!flagCorrectCode) -> skip;
  od

}
