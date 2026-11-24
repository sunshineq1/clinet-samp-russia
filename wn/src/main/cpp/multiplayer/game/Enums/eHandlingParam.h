#pragma once

// Номера правок хендлинга, приходящих от сервера
enum eHandlingParam {
    HANDLING_PARAM_MAX_SPEED             = 0,
    HANDLING_PARAM_ACCELERATION          = 1,
    HANDLING_PARAM_DRIVE_TYPE            = 2,
    HANDLING_PARAM_ENGINE_INERTIA        = 3,
    HANDLING_PARAM_MASS                  = 4,
    HANDLING_PARAM_TURN_MASS             = 5,
    HANDLING_PARAM_BRAKE_DECELERATION    = 6,
    HANDLING_PARAM_TRACTION_MULTIPLIER   = 7,
    HANDLING_PARAM_TRACTION_LOSS         = 8,
    HANDLING_PARAM_TRACTION_BIAS         = 9,
    HANDLING_PARAM_SUSPENSION_LOWER      = 10,
    HANDLING_PARAM_SUSPENSION_BIAS       = 11,
    HANDLING_PARAM_STEERING_LOCK         = 12,
    HANDLING_PARAM_SUSPENSION_FORCE      = 13,
    HANDLING_PARAM_SUSPENSION_DAMPING    = 14,
    HANDLING_PARAM_SUSPENSION_HIGH_DAMP  = 15,
    HANDLING_PARAM_SUSPENSION_UPPER      = 16,
    HANDLING_PARAM_SUSPENSION_ANTIDIVE   = 17,
    HANDLING_PARAM_BRAKE_BIAS            = 18,
    HANDLING_PARAM_NUMBER_OF_GEARS       = 19,
    HANDLING_PARAM_DRAG_MULT             = 20,
    HANDLING_PARAM_CENTRE_OF_MASS_Z      = 21,
    HANDLING_PARAM_COLLISION_DAMAGE      = 22
};

// Как понимать пришедшее значение
enum eHandlingEditMode {
    HANDLING_MODE_SCALE      = 0, // множитель к заводскому
    HANDLING_MODE_ABSOLUTE   = 1, // готовое значение
    HANDLING_MODE_MASS_SCALE = 2  // доля от массы этой машины
};
