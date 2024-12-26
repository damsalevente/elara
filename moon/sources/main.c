#include "main.h"
#include "motor.h"

#include <mach-o/dyld.h>
#include <raylib.h>
#include <zmq.h>
#include <raymath.h>
#define RAYGUI_IMPLEMENTATION
#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer_raylib.c"

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
	bool shoot_projectile = false; /* shoot projectile or now */
	float desired_position = 0.0f;
	/* should be a button */
	bool param_update_from_client = false;

	int posX = screenWidth / 2;
	int posY = screenHeight / 2;
	float distance = 0;
	Vector2 position_target = {posX, posY};
	Vector2 projectile_current = {0};
	Vector2 projectile_end = {0};
	Vector2 position_current = {posX, posY};
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
  void *publishser = zmq_socket(context,ZMQ_PUB);
  void *subscribe = zmq_socket(context,ZMQ_SUB);
  
  zmq_bind(publishser, "tcp://*:5563");
  zmq_connect(subscribe, "tcp://localhost:5564");
  zmq_setsockopt(subscribe, ZMQ_SUBSCRIBE, "P", 1);
	// Main game loop
	while (!WindowShouldClose()) // Detect window close button or ESC key
	{
		/* plant motor stuff */
		BeginDrawing();
		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

		GuiTextBox((Rectangle){0, 400, 200, 20}, TextFormat("Distance: %lf", distance), 12,
			   0);

		//param_update_from_client = Sliders(slider_params, 9);

		if (param_update_from_client) {
			/* TODO send zmq message */
			//zmq_send(socket, slider_params, 9, 0);
		}
    /* get current position from server */
    char buffer[sizeof(Vector2)];
    uint8_t address[2];
    zmq_recv(subscribe, address, 1, ZMQ_NOBLOCK);
    zmq_recv(subscribe, buffer, sizeof(Vector2), ZMQ_NOBLOCK);
    memcpy(&position_current, buffer, sizeof(Vector2));

    printf("Received position: [%lf, %lf]\n", position_current.x, position_current.y);
    printf("Target position: [%lf, %lf]\n", position_target.x, position_target.y);
    char outbuf[sizeof(Vector2)];
    memcpy(outbuf, &position_target, sizeof(Vector2));
    zmq_send(publishser,"T",1,ZMQ_SNDMORE);
    zmq_send(publishser,outbuf,sizeof(Vector2), 0);
    

		if (MotorSelect(&motor_type)) {
			/* TODO: send request to server */
			//zmq_send(socket, motor_type, 1, 0);
		}
		if (IsGestureDetected(GESTURE_TAP) == TRUE ||
		    TRUE == IsGestureDetected(GESTURE_DRAG)) {
			position_target = GetTouchPosition(0);
			//zmq_send(socket, buffer, 64, 0);
		}
		if (IsKeyDown(KEY_B) == TRUE) {
			/* TODO: send shoot switch command */
			shoot_switcher = !shoot_switcher;
//			zmq_send(socket, &shoot_switcher, 1, 0);
		}
		if (IsKeyDown(KEY_SPACE) == TRUE) {
			/* TODO shoot */
			if (!shoot_switcher || !shoot_projectile) {
				projectile_current = position_current;
				projectile_end = GetMousePosition();
				shoot_projectile = true;
			}
			//zmq_send(socket, &shoot_projectile, 1, 0);
		}
		DrawCar(&carTexture, position_current.x, position_current.y);

		EndDrawing();
		//----------------------------------------------------------------------------------
	}
	UnloadTexture(carTexture);
	// De-Initialization
	//--------------------------------------------------------------------------------------
	CloseWindow(); // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

	return 0;
}
