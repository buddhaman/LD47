typedef enum 
{
    ACTION_UNKNOWN,
    ACTION_RIGHT,
    ACTION_UP,
    ACTION_LEFT,
    ACTION_DOWN,
    ACTION_Z,
    ACTION_X,
    ACTION_Q,
    ACTION_E,
    ACTION_R,
    NUM_KEY_ACTIONS
} KeyAction;

typedef struct AppState AppState;
struct AppState
{
    i32 screenWidth;
    i32 screenHeight;
    r32 ratio;
    i32 mx;
    i32 my;

    b32 isActionDown[NUM_KEY_ACTIONS];
    b32 wasActionDown[NUM_KEY_ACTIONS];
};
