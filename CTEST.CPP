#include "ctest.h"

Test::Test(int Item_xpos, int Item_ypos, int Item_width, int Item_height, double Mover_angle, double Mover_acc, double Mover_dec, Str Object_Name) :
Item(Item_xpos, Item_ypos, Item_width, Item_height, Object_Name),
Mover(Mover_angle, Mover_acc, Mover_dec)
{
}
