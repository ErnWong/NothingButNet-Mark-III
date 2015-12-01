# PTC Model

## Constants

```c
const float PTC_TEMPERATURE_AMBIENT = 20.0f
```

This is the temperature of the surroundings (in deg C).

```c
const float PTC_TEMPERATURE_TRIP = 100.0f
```

This is the temperature over which a PTC will trip (in deg C).

```c
const float PTC_TEMPERATURE_UNTRIP = 90.0f
```

This is the temperature under which a tripped PTC will untrip (in deg C).

```c
extern const float PTC_CURRENT_HOLD_...
extern const float PTC_TIME_TRIP_...
extern const float PTC_K_TAU_...
extern const float PTC_CONSTANT_TAU_...
extern const float PTC_CONSTANT_1_...
extern const float PTC_CONSTANT_2_...
```

These are constants that characterises different PTCs.

The hold current is the current with which the PTC should not trip.

The trip time is the time taken to trip the PTC when the current is five times that of the hold current.

The k_tau constant is a safety factor (0.5 ~ 0.8). Increasing this constant increases the duration before the PTC model trips.

The rest of the constants are the pre-calculated constants that depends on the previous parameters:

Constant-1 is the reciprocal of the dissipation constant,
$$ \frac{1}{k} = \frac{T_c - T_0}{I_{\text{hold}}^2} $$

Constant-2 is the dissipation constant per specific heat, $1/\tau$, where
$$\tau = \frac{1}{2} \left(\frac{I_trip}{I_0}\right)^2 t_{\text{trip}}$$
