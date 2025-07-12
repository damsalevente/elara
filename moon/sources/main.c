#include "main.h"
#include "motor.h"
#include <pthread.h>
#include <unistd.h>
#include <raylib.h>
#include <zmq.h>
#include <raymath.h>
#define RAYGUI_IMPLEMENTATION
#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer_raylib.c"

#define TRUE 1u
#define FALSE 0u

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

static Vector2 position_target = {200, 400};
static Vector2 position_current = {200, 400};
static Vector2 projectile_current = {0};
static Vector2 projectile_end = {0};
bool shoot_projectile = false; /* shoot projectile or now */

static pthread_mutex_t position_target_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t position_current_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t projectile_target_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t projectile_current_mutex = PTHREAD_MUTEX_INITIALIZER;

static bool running = true;

void set_position_target(Vector2 pos)
{
	pthread_mutex_lock(&position_target_mutex);
	position_target.x = pos.x;
	position_target.y = pos.y;
	pthread_mutex_unlock(&position_target_mutex);
}

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

Vector2 get_position_target()
{
	Vector2 pos;
	pthread_mutex_lock(&position_target_mutex);
	pos.x = position_target.x;
	pos.y = position_target.y;
	pthread_mutex_unlock(&position_target_mutex);
	return pos;
}

void set_position_current(Vector2 pos)
{
	pthread_mutex_lock(&position_current_mutex);
	position_current.x = pos.x;
	position_current.y = pos.y;
	pthread_mutex_unlock(&position_current_mutex);
}

Vector2 get_position_current()
{
	Vector2 pos;
	pthread_mutex_lock(&position_current_mutex);
	pos.x = position_current.x;
	pos.y = position_current.y;
	pthread_mutex_unlock(&position_current_mutex);
	return pos;
}

static void *subscriber_thread(void *ctx)
{

	void *subscribe = zmq_socket(ctx, ZMQ_SUB);
	zmq_connect(subscribe, "tcp://localhost:5564");
	zmq_setsockopt(subscribe, ZMQ_SUBSCRIBE, "P", 1);

	uint8_t address[2] = {0};
	while (running) {
		Mqtt_Payload_r buffer = {0};
		zmq_recv(subscribe, address, 1, ZMQ_NOBLOCK);
		if (-1 != zmq_recv(subscribe, &buffer, sizeof(Mqtt_Payload_r), ZMQ_NOBLOCK)) {
			set_position_current(buffer.player_position_current);
			set_projectile_curr(buffer.player_projectile_current);
      printf(" position current: %lf %lf \n ", buffer.player_position_current.x, buffer.player_position_current.y);
      printf(" projectile current: %lf %lf \n ", buffer.player_projectile_current.x, buffer.player_projectile_current.y);
		}
		usleep(10 * 1000);
	}
	zmq_close(subscribe);
	return NULL;
}

static void *publisher_thread(void *ctx)
{
	void *pub = zmq_socket(ctx, ZMQ_PUB);
	zmq_bind(pub, "tcp://*:5563");
	while (running) {
    Mqtt_Payload_s outbuf = 
      {
        .player_position_target = get_position_target(),
        .player_projectile = get_projectile_end(), 
        .projectile_active = shoot_projectile 
      };
    zmq_send(pub, "T", 1, ZMQ_SNDMORE);
		zmq_send(pub, &outbuf, sizeof(Mqtt_Payload_s), 0);
		usleep(10 * 1000);
	}
	zmq_close(pub);
	return NULL;
}


void DrawProjectile(int x, int y)
{
  DrawCircle(x, y, 5, RED);
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

static uint8_t slider_params[9] = {0};
const char *paramtext[9] = {
	"Speed P", "Speed I", "Speed D", "ID P", "ID I", "ID D", "IQ P", "IQ I", "IQ D",
};

/* build slider layout and return if update is needed */
bool Sliders(uint8_t slider_params[], uint8_t size)
{
	bool update = false;
	for (int i = 0; i < size; i++) {
		uint8_t newVal = 0;
		GuiSlider((Rectangle){0, 200 + 15 * i, 120, 20}, paramtext[i],
			  TextFormat("%d", slider_params[i]), &newVal, 0.0, 20.0);
		if (newVal != slider_params[i]) {
			update = true;
			slider_params[i] = newVal;
		}
	}
	return update;
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
	float w_ref = 0.0;
	int motor_type = 0;
	float desired_position = 0.0f;
	/* should be a button */
	bool param_update_from_client = false;

	int posX = screenWidth / 2;
	int posY = screenHeight / 2;
	float distance = 0;
	int motor_selected = 0;
	Vector2 center = {80, 80};
	InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");
	GuiLoadStyle("/Users/leventedamsa/Downloads/style_enefete.rgs");
	/* Load textures */
	Texture2D carTexture = LoadTexture("./docs/removed_glutter.jpeg");

	SetTargetFPS(120); // Set our game to run at 60 frames-per-second
	//--------------------------------------------------------------------------------------
	/* zmq init */
	void *context = zmq_ctx_new();
	pthread_t pub;
	pthread_t sub;
	pthread_create(&pub, NULL, publisher_thread, context);
	pthread_create(&sub, NULL, subscriber_thread, context);
	Vector2 local_pos_current = {0, 0};
	Vector2 local_pos_target = {0, 0};

	// Main game loop
	while (!WindowShouldClose()) // Detect window close button or ESC key
	{
		/* plant motor stuff */
		BeginDrawing();
		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

		GuiTextBox((Rectangle){0, 400, 200, 20}, TextFormat("Distance: %lf", distance), 12,
			   0);
		local_pos_current = get_position_current();
		local_pos_target = get_position_target();
		// param_update_from_client = Sliders(slider_params, 9);

		if (param_update_from_client) {
			/* TODO send zmq message */
			// zmq_send(socket, slider_params, 9, 0);
		}
		/* get current position from server */

		// printf("Received position: [%lf, %lf]\n", local_pos_current.x, position_current.y);
		// printf("Target position: [%lf, %lf]\n", local_pos_target.x, local_pos_target.y);

		if (MotorSelect(&motor_type)) {
			/* TODO: send request to server */
		}
		if (IsGestureDetected(GESTURE_TAP) == TRUE ||
		    TRUE == IsGestureDetected(GESTURE_DRAG)) {
			set_position_target(GetTouchPosition(0));
		}
		if (IsKeyDown(KEY_B) == TRUE) {
			/* TODO: send shoot switch command */
			shoot_switcher = !shoot_switcher;
		}
    if (IsKeyDown(KEY_SPACE) == TRUE) {
      /* TODO shoot */
      if (!shoot_projectile) {
        set_projectile_end(GetMousePosition());
        shoot_projectile = true;
      }
    }
    if(shoot_projectile)
    {
      DrawProjectile(get_projectile_curr().x, get_projectile_curr().y);
      if(get_projectile_end().x == get_projectile_curr().x &&
        get_projectile_end().y == get_projectile_curr().y  ) 
      {
        shoot_projectile = false;
      }
    }
		DrawCar(&carTexture, local_pos_current.x, local_pos_current.y);

		EndDrawing();
		//----------------------------------------------------------------------------------
	}
	UnloadTexture(carTexture);
	// De-Initialization
	//--------------------------------------------------------------------------------------
	CloseWindow(); // Close window and OpenGL context
	running = false;
	pthread_join(pub, NULL);
	pthread_join(sub, NULL);
	zmq_ctx_destroy(context);
	//
	//--------------------------------------------------------------------------------------

	return 0;
}
