#include "../define.h"
asm(
  ".section h0; .set h0,0xA8EF63;"
  "JMP "QU(FAExtLoad)";"
);