#include "main.h"
#include <string.h>
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



typedef struct Mqtt_Payload_r{
  Vector2 player_position_current;
  Vector2 player_projectile_current;
  bool projectile_active;
}Mqtt_Payload_r;



typedef struct Mqtt_Payload_s{
  Vector2 player_position_target;
  Vector2 player_projectile;
  bool projectile_active;
}Mqtt_Payload_s;



#define GEARBOX ((float)20.0f)
#define TS      2.0f /* motor simulation time */


static Vector2 position_target = {200, 400};
static Vector2 position_current = {200, 400};
static Vector2 projectile_current = {0};
static Vector2 projectile_end = {0};

static pthread_mutex_t position_target_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t position_current_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t projectile_target_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t projectile_current_mutex = PTHREAD_MUTEX_INITIALIZER;
void set_projectile_curr(Vector2 pos)
{
    pthread_mutex_lock(&projectile_current_mutex);
    projectile_current = pos;
    pthread_mutex_unlock(&projectile_current_mutex);
}

void set_projectile_end(Vector2 pos)
{
    pthread_mutex_lock(&projectile_target_mutex);
    projectile_end = pos;
    pthread_mutex_unlock(&projectile_target_mutex);
}

Vector2 get_projectile_curr()
{
    Vector2 ret = {0};
    pthread_mutex_lock(&projectile_current_mutex);
    ret = projectile_current;
    pthread_mutex_unlock(&projectile_current_mutex);
    return ret;
}

Vector2 get_projectile_end()
{
    Vector2 ret = {0};
    pthread_mutex_lock(&projectile_target_mutex);
    ret =  projectile_end;
    pthread_mutex_unlock(&projectile_target_mutex);
    return ret;
}

bool projectile_active = false;
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
    Mqtt_Payload_s buffer = {0};
		zmq_recv(subscribe, address, 1, ZMQ_NOBLOCK);
		if (-1 != zmq_recv(subscribe, &buffer, sizeof(Mqtt_Payload_s), ZMQ_NOBLOCK)) {
      printf("Current position: %lf %lf\n", buffer.player_position_target.x, buffer.player_position_target.y);
      printf("Projectile : %lf %lf\n", buffer.player_projectile.x, buffer.player_projectile.y );
      printf("Projectile active: %d\n", buffer.projectile_active);
      set_projectile_end(buffer.player_projectile);
      set_position_target(buffer.player_position_target);
      projectile_active = buffer.projectile_active;
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
		Mqtt_Payload_r outbuf = {
      .player_position_current = get_position_current(),
      .player_projectile_current = get_projectile_curr(),
      .projectile_active = false
    };
		zmq_send(pub, "P", 1, ZMQ_SNDMORE);
		zmq_send(pub, &outbuf, sizeof(Mqtt_Payload_r), 0);
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
  bool prev_projectile_active = false;

	Vector2 local_pos_current = {0, 0};
	Vector2 local_pos_target = {100.0, 200.0};
	Vector2 local_projectile_current = {0, 0};
	Vector2 local_projectile_target = {100.0, 200.0};
	//--------------------------------------------------------------------------------------

	// Main game loop
	while (1) // Detect window close button or ESC key
	{
		local_pos_current = get_position_current();
		local_pos_target = get_position_target();

    if(projectile_active)
    {
      if(!prev_projectile_active)
      {
        local_projectile_current = local_pos_current;
      }
      else
      {
        local_projectile_current = Vector2MoveTowards(get_projectile_curr(), get_projectile_end(), 30);
      }
      set_projectile_curr(local_projectile_current);
    }
    prev_projectile_active = projectile_active;
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
		// printf("Received position: [%lf, %lf]\n", local_pos_target.x, local_pos_target.y);
		// printf("Current position: [%lf, %lf]\n", local_pos_current.x, local_pos_current.y);
		usleep(10 * 1000);
	}

	return 0;
}
