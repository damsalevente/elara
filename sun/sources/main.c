#include "main.h"
#include <_string.h>
#include <mach-o/dyld.h>
#include <raylib.h>
#include <unistd.h>
#include <zmq.h>
#include <raymath.h>
#include "controllers.h"
#include "motor.h"
#define RAYGUI_IMPLEMENTATION
#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer_raylib.c"
#include "unistd.h"
#include <pthread.h>

#include "solver.h"

#define GEARBOX ((float)20.0f)
#define TS      2.0f /* motor simulation time */

static Vector2 position_target = {0, 0};
static Vector2 position_current = {0, 0};
static pthread_mutex_t position_target_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t position_current_mutex = PTHREAD_MUTEX_INITIALIZER;

static bool running = true;

static void set_position_target(Vector2 pos)
{
	pthread_mutex_lock(&position_target_mutex);
	position_target.x = pos.x;
	position_target.y = pos.y;
	pthread_mutex_unlock(&position_target_mutex);
}

static Vector2 get_position_target()
{
	Vector2 pos = {0};
	pthread_mutex_lock(&position_target_mutex);
	pos.x = position_target.x;
	pos.y = position_target.y;
	pthread_mutex_unlock(&position_target_mutex);
	return pos;
}

static void set_position_current(Vector2 pos)
{

	pthread_mutex_lock(&position_current_mutex);
	position_current.x = pos.x;
	position_current.y = pos.y;
	pthread_mutex_unlock(&position_current_mutex);
}

static Vector2 get_position_current()
{
	Vector2 pos = {0};
	pthread_mutex_lock(&position_current_mutex);
	pos.x = position_current.x;
	pos.y = position_current.y;
	pthread_mutex_unlock(&position_current_mutex);
	return pos;
}

static void *subscriber_thread(void *ctx)
{

	void *subscribe = zmq_socket(ctx, ZMQ_SUB);
	zmq_connect(subscribe, "tcp://localhost:5563");
	zmq_setsockopt(subscribe, ZMQ_SUBSCRIBE, "T", 1);

	uint8_t address[2];
	while (running) {
		Vector2 position_from_server = {0};
		char buffer[sizeof(Vector2)] = {0};
		zmq_recv(subscribe, address, 1, ZMQ_NOBLOCK);
		if (-1 != zmq_recv(subscribe, buffer, sizeof(Vector2), ZMQ_NOBLOCK)) {
			memcpy(&position_from_server, buffer, sizeof(Vector2));
			set_position_target(position_from_server);
		}

		usleep(10 * 1000);
	}
	zmq_close(subscribe);
	return NULL;
}

static void *publisher_thread(void *ctx)
{
	void *pub = zmq_socket(ctx, ZMQ_PUB);
	zmq_bind(pub, "tcp://*:5564");
	while (running) {
		char outbuf[sizeof(Vector2)] = {0};
		Vector2 target_pos = get_position_current();
		memcpy(outbuf, &target_pos, sizeof(Vector2));
		zmq_send(pub, "P", 1, ZMQ_SNDMORE);
		zmq_send(pub, outbuf, sizeof(Vector2), 0);
		usleep(10 * 1000);
	}
	zmq_close(pub);
	return NULL;
}

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

static int MotorSelect(int *motor_type)
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

static Vector2 handle_wall(Vector2 _pos_curr, int screenWidth, int screenHeight)
{
	if (_pos_curr.x > screenWidth) {
		_pos_curr.x = screenWidth;
	} else if (_pos_curr.x < 0) {
		_pos_curr.x = 0;
	}
	if (_pos_curr.y > screenHeight) {
		_pos_curr.y = screenHeight;
	} else if (_pos_curr.y < 0) {
		_pos_curr.y = 0;
	}
	return _pos_curr;
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
	int posX = screenWidth * 0.5;
	int posY = screenHeight * 0.5;
	float distance = 0;
	Vector2 projectile_end = {0};
	Vector2 position_current = {posX, posY};
	int motor_selected = 0;
	Vector2 center = {80, 80};
	/* Load textures */

	/* setup clay */
	// uint64_t totalMemorySize = Clay_MinMemorySize();
	// Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize,
	// malloc(totalMemorySize)); Clay_Initialize(arena,  (Clay_Dimensions){screenWidth,
	// screenHeight}); Clay_SetMeasureTextFunction(measureText);
 
	motor_turn_on(motor_state);
	set_motor(motor_type);

	/* zmq init */
	void *context = zmq_ctx_new();
	pthread_t pub;
	pthread_t sub;
	pthread_create(&pub, NULL, publisher_thread, context);
	pthread_create(&sub, NULL, subscriber_thread, context);

	Vector2 local_pos_current = {0, 0};
	Vector2 local_pos_target = {100.0, 200.0};
	//--------------------------------------------------------------------------------------

	// Main game loop
	while (1) // Detect window close button or ESC key
	{
		local_pos_current = get_position_current();
		local_pos_target = get_position_target();
		/* plant motor stuff */
		step(t, t + TS, motor_state);
		t = t + TS;
		/* controller stuff */
		distance = Vector2Distance(local_pos_current, local_pos_target);
		w_ref = pos_control(distance, 0); /* control distance  to be zero */
		control_runner(&w_ref, &motor_state[WR], &motor_state[ID], &motor_state[IQ],
			       &motor_state[VD], &motor_state[VQ]);

		local_pos_current =
			Vector2MoveTowards(local_pos_current, local_pos_target, motor_state[WR]);
		local_pos_current = handle_wall(local_pos_current, screenWidth, screenHeight);
		set_position_current(local_pos_current);
		/* send topic */
		printf("Received position: [%lf, %lf]\n", local_pos_target.x, local_pos_target.y);
		printf("Current position: [%lf, %lf]\n", local_pos_current.x, local_pos_current.y);
		usleep(10 * 1000);
	}

	return 0;
}
