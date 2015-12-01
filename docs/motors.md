# Motors

## Constants

```c
const int SMARTMOTOR_COMMAND_NO_LIMIT = 255
```

This special motor command is used internally to represent the state of not needing any command limit.

```c
const int SMARTMOTOR_COMMAND_MAX = 127
const int SMARTMOTOR_COMMAND_MIN = -127
```

This is the upper bound and lower bound of the possible command values. This is used internally to set command limits.

```c
const float SMARTMOTOR_CURRENT_UNTRIP_FACTOR = 0.9f
```

The motor current is marked as "tripped" when the motor current goes above the targeted current. It then requires the motor current to fall below by this factor before it is marked as untripped again.
