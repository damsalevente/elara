#include "main.h"

#include <_string.h>
#include <mach-o/dyld.h>
#include <raylib.h>
#include <unistd.h>
#include <zmq.h>
#include <raymath.h>
#include "controllers.h"
#include "motor.h"
#include "pid.h"
#define RAYGUI_IMPLEMENTATION
#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer_raylib.c"

#include "solver.h"

#define GEARBOX ((float)20.0f)
#define TS      1.0f /* motor simulation time */

void DrawCar(Texture2D *texture, int x, int y)
{
	static uint8_t run_anim_counter = 0;
	static int prev_x = 0;
	static int prev_y = 0;
	static bool isRunning = false;
	int carWidth = texture->width;
	int carHeight = texture->height;
	Rectangle sourceRect = {0.0f, 0.0f, (float)carWidth, (float)carHeight};
	Rectangle destRect = {x - (float)carWidth / 8, y - (float)carHeight / 8,
			      (float)carWidth / 4, (float)carHeight / 4};
	Vector2 origin = {0, 0};
	DrawTexturePro(*texture, sourceRect, destRect, origin, 0.0f, WHITE);

	if (abs(x - prev_x) > 0.5 || abs(y - prev_y) > 0.5) {
		if (run_anim_counter % 5 == 0) {
			isRunning = !isRunning;
		}
		if (isRunning) {
			GuiLabel((Rectangle){x - 10, y - 10, 10, 10}, "#150#");
		} else {
			GuiLabel((Rectangle){x - 10, y - 10, 10, 10}, "#149#");
		}
	} else {
		GuiLabel((Rectangle){x - 10, y - 10, 10, 10}, "#149#");
	}
	prev_x = x;
	prev_y = y;
	run_anim_counter++;
}

int MotorSelect(int *motor_type)
{
	int motor_selected = 0;
	motor_selected = GuiButton((Rectangle){600, 70, 120, 20}, "Motor 1");
	if (motor_selected) {
		*motor_type = 0;
		return 1;
	}
	motor_selected = GuiButton((Rectangle){600, 90, 120, 20}, "Motor 2");
	if (motor_selected) {
		*motor_type = 1;
		return 1;
	}

	motor_selected = GuiButton((Rectangle){600, 110, 120, 20}, "Motor 3");
	if (motor_selected) {
		*motor_type = 2;
		return 1;
	}
	return 0;
}

static inline Clay_Dimensions measureText(Clay_String *cs, Clay_TextElementConfig *tec)
{
	Clay_Dimensions dimensions = {0.0f, 0.0f};
	dimensions.height = tec->lineHeight;
	dimensions.width = cs->length * 15;
	return dimensions;
}

int main(void)
{
	// Initialization
	//--------------------------------------------------------------------------------------
	const int screenWidth = 1200;
	const int screenHeight = 600;
	/* motor parameters */
	float t = 0.0; /* time */
	int counter = 0;
  bool shoot_switcher = false;
	int ctrl_run = 0;
	float motor_state[N] = {0.0}; /* buffer for output */
	float w_ref = 0.0;
	int motor_type = 0;
	bool shoot_projectile = false; /* shoot projectile or now */
	float desired_position = 0.0f;
	/* should be a button */
	motor_turn_on(motor_state);
  
	int posX = screenWidth / 2;
	int posY = screenHeight / 2;
	float distance = 0;
	Vector2 position_target = {posX, posY};
	Vector2 projectile_current = {0};
	Vector2 projectile_end = {0};
	volatile Vector2 position_current = {posX, posY};
	int motor_selected = 0;
	Vector2 center = {80, 80};
	/* Load textures */

	/* setup clay */
	// uint64_t totalMemorySize = Clay_MinMemorySize();
	// Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize,
	// malloc(totalMemorySize)); Clay_Initialize(arena,  (Clay_Dimensions){screenWidth,
	// screenHeight}); Clay_SetMeasureTextFunction(measureText);
	set_motor(motor_type);

  /* zmq init */
  void *context = zmq_ctx_new();
  void *publishser = zmq_socket(context,ZMQ_PUB);
  void *subscribe = zmq_socket(context,ZMQ_SUB);
  zmq_bind(publishser, "tcp://*:5564");
  zmq_connect(subscribe, "tcp://localhost:5563");
  zmq_setsockopt(subscribe, ZMQ_SUBSCRIBE, "T", 1);

	//--------------------------------------------------------------------------------------
  position_target = (Vector2){100.0, 200.0};
	// Main game loop
	while (1) // Detect window close button or ESC key
	{
		/* plant motor stuff */
		step(t, t + TS, motor_state);
		t = t + TS;

		/* controller stuff */
		distance = Vector2Distance(position_current, position_target);
		w_ref = pos_control(distance, 0); /* control distance  to be zero */
		control_runner(&w_ref, &motor_state[WR], &motor_state[ID], &motor_state[IQ],
			       &motor_state[VD], &motor_state[VQ]);

		position_current =
			Vector2MoveTowards(position_current, position_target, motor_state[WR]);

		if (position_current.x > screenWidth) {
			position_current.x = screenWidth;
		} else if (position_current.x < 0) {
			position_current.x = 0;
		}
		if (position_current.y > screenHeight) {
			position_current.y = screenHeight;
		} else if (position_current.y < 0) {
			position_current.y = 0;
		}
    /* send topic */
    zmq_send(publishser, "P", 1, ZMQ_SNDMORE);
    /* send data */
    
    uint8_t buffer[sizeof(Vector2)];
    memcpy(buffer, &position_current, sizeof(Vector2));
    /* TODO: in one thread */
    zmq_send(publishser, buffer, sizeof(Vector2), 0);
    char address[2];
    zmq_recv(subscribe, address, 1, 0);
    printf("Received address: [%s]\n", address);
    uint8_t data [sizeof(Vector2)];
    zmq_recv(subscribe, data, sizeof(Vector2), 0);
    memcpy(&position_target,data, sizeof(Vector2));
    printf("Received position: [%lf, %lf]\n", position_target.x, position_target.y);
    printf("Current position: [%lf, %lf]\n", position_current.x, position_current.y);
	}

	return 0;
}
