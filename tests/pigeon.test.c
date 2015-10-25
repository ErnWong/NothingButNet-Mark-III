#include "tap.h"
#include "pigeon.h"

int main()
{
    plan(1);
    ok(true, "Hello World from test!");
    done_testing();
}
