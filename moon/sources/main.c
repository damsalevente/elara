#include "main.h"

#include <mach-o/dyld.h>
#include <raylib.h>
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
  
  void *context = zmq_ctx_new();

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

	/* setup clay */
	// uint64_t totalMemorySize = Clay_MinMemorySize();
	// Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize,
	// malloc(totalMemorySize)); Clay_Initialize(arena,  (Clay_Dimensions){screenWidth,
	// screenHeight}); Clay_SetMeasureTextFunction(measureText);
	set_motor(motor_type);

	SetTargetFPS(120); // Set our game to run at 60 frames-per-second
	//--------------------------------------------------------------------------------------

	// Main game loop
	while (!WindowShouldClose()) // Detect window close button or ESC key
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

		if (shoot_projectile) {

			projectile_current =
				Vector2MoveTowards(projectile_current, projectile_end, 4);
      GuiLabel((Rectangle){projectile_current.x-10, projectile_current.y, 200,10}, "rontas");
      if((projectile_current.x == projectile_end.x) && (projectile_current.y == projectile_end.y))
      {
        shoot_projectile = false;
      }
		}

		BeginDrawing();
		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

		GuiTextBox((Rectangle){0, 400, 200, 20}, TextFormat("Distance: %lf", distance),
			   12, 0);
		GuiSlider((Rectangle){0, 40, 120, 20}, "Reference position",
			  TextFormat("%lf", desired_position), &desired_position, 0.0f,
			  screenWidth);
		GuiSlider((Rectangle){0, 200, 120, 20}, "PID P",
			  TextFormat("%lf", controller_w.K), &controller_w.K, 0.0f, 20.0f);
		GuiSlider((Rectangle){0, 220, 120, 20}, "PID I",
			  TextFormat("%lf", controller_w.I), &controller_w.I, 0.0f, 20.0f);
		GuiSlider((Rectangle){0, 240, 120, 20}, "PID D",
			  TextFormat("%lf", controller_w.D), &controller_w.D, 0.0f, 20.0f);
		GuiSlider((Rectangle){0, 260, 120, 20}, "PID P",
			  TextFormat("%lf", controller_id.K), &controller_id.K, 0.0f, 20.0f);
		GuiSlider((Rectangle){0, 280, 120, 20}, "PID I",
			  TextFormat("%lf", controller_id.I), &controller_id.I, 0.0f, 20.0f);
		GuiSlider((Rectangle){0, 300, 120, 20}, "PID D",
			  TextFormat("%lf", controller_id.D), &controller_id.D, 0.0f, 20.0f);
		GuiSlider((Rectangle){0, 320, 120, 20}, "PID P",
			  TextFormat("%lf", controller_iq.K), &controller_iq.D, 0.0f, 20.0f);
		GuiSlider((Rectangle){0, 340, 120, 20}, "PID I",
			  TextFormat("%lf", controller_iq.I), &controller_iq.D, 0.0f, 20.0f);
		GuiSlider((Rectangle){0, 360, 120, 20}, "PID D",
			  TextFormat("%lf", controller_iq.D), &controller_iq.D, 0.0f, 20.0f);

		if (MotorSelect(&motor_type)) {
			set_motor(motor_type);
			motor_turn_on(motor_state);
		}
		DrawText(TextFormat("Speed:\t\t %.2lf", motor_state[WR]), 340, 0, 16, GRAY);
		DrawText(TextFormat("D Current: %.2lf", motor_state[ID]), 450, 0, 16, GRAY);
		DrawText(TextFormat("Q Current: %l.2f", motor_state[IQ]), 600, 0, 16, GRAY);
		// DrawCircleSector(center, 60, 0, 360, 40, GRAY);
		// DrawCircleSector(center, 60, motor_state[THETA], motor_state[THETA] + 10, 40,
		// BLUE);
		if (IsGestureDetected(GESTURE_TAP) == TRUE || TRUE == IsGestureDetected(GESTURE_DRAG)) {
			position_target = GetTouchPosition(0);
		}
    if (IsKeyDown(KEY_B) == TRUE)
    {
      shoot_switcher = !shoot_switcher;
    }
		if (IsKeyDown(KEY_SPACE) == TRUE) {
			if (!shoot_switcher || !shoot_projectile) {
				projectile_current = position_current;
				projectile_end = GetMousePosition();
				shoot_projectile = true;
			}
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
