#ifndef CTEST
#define CTEST

#include <ctileactor.h>
#include <cmover.h>
#include <cvisual.h>

class Test : public TileActor, public Mover, public Visual
{
public:
	Test() {}
	Test(int Item_xpos, int Item_ypos, int Item_width, int Item_height, double Mover_angle, double Mover_acc, double Mover_dec, Str Object_Name);
	Test(Test &model) : Item(model), Mover(model), Visual(model) {}
};

#endif