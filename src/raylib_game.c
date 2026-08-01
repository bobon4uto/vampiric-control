#include "raylib.h"
#include "raymath.h"
#include "raymath_snake_case.h"
#include "snake_case_api.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

// :include
#define MONO_BUILD
#include "resources/info.h"
#include <stdio.h>

// :macro
// :color
#define DEEP_RED          (Color){0xAA,0x00,0x00,0xFF}
#define SLIGHTLY_RED_GRAY (Color){0x75, 0x45, 0x45}


#define MAX_UNITS 8000
#define DBG(mess) do {fprintf(stderr, mess); fflush(stderr);} while (0)
// :type
typedef enum {
  SCREEN_DISCLAIMER,
  SCREEN_TITLE,
  SCREEN_CREDITS,
  SCREEN_GAMEPLAY,
  SCREEN_COUNT,
} GameScreen;

typedef struct sTalkBox {
  const char* text;
  float finish;
  float progress;
  float speed;
  float scale;
  int x, y;
  Color color;
  bool closed;
  bool is_background_talking;
} TalkBox;


typedef struct sPointer {
  Vector2 position;
  bool is_pressed, is_released, is_down;
} Pointer;

typedef struct sButton {
  bool is_pressed, is_released, is_down;
} Button;



typedef struct sVirtualStick {
  Vector2 position;
  float radius;
  int captured_touch;
} VirtualStick;

typedef struct sGamepadSetting {
  int id;
  int move_axis_x;
  int move_axis_y;
  int shoot_axis_x;
  int shoot_axis_y;
  int action_btn;
} GamepadSetting;


typedef struct sKeyboardSetting {
  int left_key;
  int right_key;
  int up_key;
  int down_key;
  int shoot_left_key;
  int shoot_right_key;
  int shoot_up_key;
  int shoot_down_key;
  int action_key;
} KeyboardSetting;


typedef struct sInput {
  Pointer pointer;
  Button action;
  Vector2 move_vec;
  Vector2 shoot_vec;
  float deadzone;
  bool is_touch_enabled, is_mouse_enabled, is_gamepad_enabled;
  VirtualStick left_stick;
  VirtualStick right_stick;
  GamepadSetting  gamepad;
  KeyboardSetting keyboard;
} Input;

typedef struct sMusicPlayer {
  float fade;
  float fade_speed;
  EnumMusic main_stream;
  EnumMusic secondary_stream;
} MusicPlayer;

typedef struct sUIButton {
  Rectangle rec;
  Color back_color;
  Color text_color;
  Vector2 text_offset;
  const char* text;
  bool is_hovered, is_pressed, is_down, is_released;
  bool was_in_range, was_pressed;
} UIButton;


typedef struct sUITitle {
  UIButton start;
  UIButton credits;
  UIButton exit_game;
  int selected;
  bool lock_select;
} UITitle;


typedef struct sUI{
  UITitle title;
} UI;

typedef struct sErrorBanner {
  const char* text;
  Rectangle rec;
  float anim;
  float anim_end;
  float anim_speed;
  float opacity;
} ErrorBanner;


typedef enum {
  UNIT_CHAR,
  UNIT_PROJECTILE,
  UNIT_ITEM,
} UnitKind;


typedef enum {
  UNIT_STRATEGY_RUSH,
  UNIT_STRATEGY_STAY_AWAY,
} UnitStrategy;



typedef enum {
  UNIT_WEAPON_SHOTGUN,
  UNIT_WEAPON_RIFLE,
  UNIT_WEAPON_PUNCH,
} UnitWeapon;

typedef struct sUnit {
  UnitKind kind;
  UnitStrategy strategy;
  UnitWeapon weapon;
  Vector2 position;
  float damage;
  float damage_speed;
    float damage_reload;
  float knockback;
  float health;
  float speed;
} Unit;


typedef struct sUnitArray {
  int count;
  Unit items[MAX_UNITS];
} UnitArray;

typedef struct sPlayerStat {
  float score; // why not?
} PlayerStat;

typedef struct sGameplay {
  UnitArray units;
  Camera2D camera;
  PlayerStat player_stats;
} Gameplay;



// :glob
static const int screen_x_ = 720;
static const int screen_y_ = 1080;
static bool should_exit = false;


static Gameplay gameplay = {0};

static RenderTexture2D target = {0};
static int frame_counter = 0;

static AssetBank assets = {0};
static TalkBox current_talk_box = {0};
static TalkBox vlad_talk_box = {0};
static Input input = {0};
static MusicPlayer music_player = {0};
static GameScreen current_screen = SCREEN_DISCLAIMER;
static bool SHUTUPMAKE = false;

static float controlled_float = 32.0f;

static UI ui = {0};
static ErrorBanner error_banner = {0};


// :func
static void check_control_methods();
static Vector2 measure_text_default(const char* text, float scale);
static void draw_boxed_text_centered(Rectangle rec, const char* text, float roundness, Color rec_color, Color text_color );
static void report_error(const char* mess);
static void update_diclaimer();
static void update_title();
static void update_credits();
static void draw_diclaimer();
static void draw_title();
static void draw_credits();
static void update_draw_frame(void);
static void draw_text_default(const char* text, int x, int y, float scale, Color color);
static int get_string_len(const char* s);

static float anim_get_stage_progress(float overall_progress,float stage_start, float stage_end);

static Music get_music(EnumMusic music);
static Font get_font(EnumFont font);
// :fTalkBox
static TalkBox talk_box_new(const char* text, int x, int y, float scale, float speed, Color color);
static void talk_box_update(TalkBox* self);
static void talk_box_draw(TalkBox self);
static TalkBox talk_box_vlad(const char* text);
// :fInput
static Input input_init();
static void input_update(Input* input);
static void input_draw(Input input);
static Vector2 input_fix_vec(Vector2 unfixed, float deadzone, float maximum);
// :fVirtualStick
void virtual_stick_draw(VirtualStick self, Vector2 control_vec);
// :fMusicPlayer
static MusicPlayer music_player_init();
static void music_player_update(MusicPlayer* music_player);
static void music_player_fade_to(MusicPlayer* self, EnumMusic id);

// :fUI
static UI ui_init();

// :fUnit
void unit_update(Unit* self);
void unit_draw(Unit self);
void unit_char_draw_controlled_overlay( Unit self );



// :fPlayerStat
void player_stat_draw(PlayerStat self);

// :fTitle
static UITitle title_init();


// :fButton
static UIButton ui_button_new(int x, int y, int x_, int y_, const char* text);
static void button_draw(UIButton self);
void button_update(UIButton* self);

// :fErrorBanner
ErrorBanner error_banner_init();
void error_banner_update(ErrorBanner* self);
void error_banner_draw(ErrorBanner self);


// :fGameplay
static Gameplay gameplay_init();
static void update_gameplay();
static void draw_gameplay();
static void gameplay_draw_floor();
Unit* gameplay_get_controlled_unit_ref(Gameplay* gameplay);

// :main
int main(void) {
#if !defined(_DEBUG)
  set_trace_log_level(LOG_NONE);
#endif

  // :init
  init_window(screen_x_, screen_y_, "raylib gamejam template");
  init_audio_device();
  assets = ab_init();

  music_player = music_player_init();
  error_banner = error_banner_init();

  gameplay = gameplay_init();

  vlad_talk_box = talk_box_new("Vlad says: ...", 50,300, 32.0f, 0.3f, BLACK );
  current_talk_box = talk_box_vlad("well well well what do we have there.");
  input = input_init();
    seek_music_stream(ab_music_get(assets, MUSIC_SKY_VIBE) ,77.7f);
  ui = ui_init();

  // :load

  target = load_render_texture(screen_x_, screen_y_);
  set_texture_filter(target.texture, TEXTURE_FILTER_POINT);

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(update_draw_frame, 60, 1);
#else
  set_target_f_p_s(60);

  while (!window_should_close() && !should_exit)
  {
    update_draw_frame();
  }
#endif

  // :deinit
  unload_render_texture(target);

  close_audio_device();
  close_window();



  if (SHUTUPMAKE) {
    talk_box_draw((TalkBox){0});
  }

  return 0;
}

void update_draw_frame(void) {
  // :update
  frame_counter++;
#ifdef _DEBUG

  // :debug
  if (is_key_down(KEY_M)) {
    //printf("======\n");
    //printf("%f\n",  get_music_time_played( ab_music_get(assets, MUSIC_SKY_VIBE) ));
    //fflush(stdout);
    if ( is_key_pressed(KEY_ONE) ) {
      music_player_fade_to(&music_player, MUSIC_PUNK_SOUND);
    } else if ( is_key_pressed(KEY_TWO) ) {
      music_player_fade_to(&music_player, MUSIC_SKY_VIBE);
    }
  }
  if ( is_key_pressed(KEY_MINUS) ) {
    controlled_float -= 1.0f;
  }
  if ( is_key_pressed(KEY_EQUAL) ) {
    controlled_float += 1.0f;
  }


#endif
  input_update(&input);


  switch (current_screen) {
    case SCREEN_DISCLAIMER: update_diclaimer(); break;
    case SCREEN_TITLE:      update_title();     break;
    case SCREEN_CREDITS:    update_credits();   break;
    case SCREEN_GAMEPLAY: update_gameplay(); break;
    default: break;
  }


  music_player_update(&music_player);
  error_banner_update(&error_banner);

  talk_box_update(&current_talk_box);
  // :draw
  begin_drawing();
  clear_background( BLACK );
  {
    switch (current_screen) {
      case SCREEN_DISCLAIMER: draw_diclaimer(); break;
      case SCREEN_TITLE:      draw_title();     break;
      case SCREEN_CREDITS:    draw_credits();   break;
      case SCREEN_GAMEPLAY:    draw_gameplay();   break;
      default: break;
    }
  }
  error_banner_draw(error_banner);

  draw_boxed_text_centered( (Rectangle){100,100,100,100}, "AHAHA", 0.0f, BLACK, RED);

  // :dbg
#ifdef _DEBUG
  draw_text( text_format("%f", controlled_float), 10,10,10, BLUE );
  char* screen_name = "SCREEN_UNKNOWN";
  switch (current_screen) {
    case SCREEN_DISCLAIMER: screen_name = "SCREEN_DISCLAIMER"; break;
    case SCREEN_TITLE:      screen_name = "SCREEN_TITLE"; break;
    case SCREEN_CREDITS:    screen_name = "SCREEN_CREDITS"; break;
    case SCREEN_GAMEPLAY: screen_name = "SCREEN_GAMEPLAY"; break;
    default: break;
  }
  draw_text( text_format("%s", screen_name), 10,30,10, BLUE );

#define dbg_state(input_elem, X, Y) \
  draw_text_default( #input_elem ": " \
            ,X, Y, 16, BLUE); \
  draw_text_default( text_format("p%d r%d d%d", \
                        input_elem.is_pressed, input_elem.is_released, \
                        input_elem.is_down), \
            X +150, Y, 16, BLUE)


#define dbg_vec(input_elem, X, Y) \
  draw_text_default( #input_elem ": " \
            ,X, Y, 16, BLUE); \
  draw_text_default( text_format("x%f y%f", \
                        input_elem.x, input_elem.y), \
            X +150, Y, 16, BLUE)

  dbg_vec(input.pointer.position, 720/2, 10);
  //draw_text_default( text_format("pointer %f %f", input.pointer.position.x, input.pointer.position.y), 720/2,10,16, BLUE );
  dbg_state(input.pointer, 720/2, 30);
  dbg_state(input.action, 720/2, 50);
  //draw_text_default( text_format("opacity %f", error_banner.opacity), 720/2,70,16, BLUE );
  dbg_vec(input.move_vec, 720/2, 70);
  dbg_vec(input.shoot_vec, 720/2, 90);


#endif

  end_drawing();
}


// :screen
void update_diclaimer() {
  check_control_methods();
  if (input.pointer.is_pressed || input.action.is_pressed) {
    current_screen = SCREEN_TITLE;
  }
}
void draw_diclaimer() {
  const char* disclaimer =
      "===========DISCLAIMER=========\n"
      "This game contains vampires,\n"
      "deadly weapons and\n"
      "fictional medicine.\n"
      "\n"
      "Please make sure you do not\n"
      "attempt to repeat character's\n"
      "actions and consider anything\n"
      "happening in the game to be\n"
      "false in our world.\n"
      "==============================\n"
      "\n"
      "press space or click anywhere\n"
      "if you read the disclaimer.\n"
      "The game supports:\n"
      "keyboard (mouse or HJKL)\n"
      "touch\n"
      "gamepad (2 joys and a button)\n"
      "X or A are space\n"
    ;

  clear_background( (Color){45,45,45, 255} );
  draw_text_default(
      disclaimer
      , 165, 286, 32.0f, DEEP_RED);

}

//:iTitle
static UITitle title_init() {
  UITitle title = {0};
  title.start     = ui_button_new(20,540 + 150 * 0, 680,130, "start");
  title.credits   = ui_button_new(20,540 + 150 * 1, 680,130, "credits");
  title.exit_game = ui_button_new(20,540 + 150 * 2, 680,130, "exit");
  title.selected = -1;
  return title;
}
static void update_title() {
  check_control_methods();

  button_update(&ui.title.start);
  button_update(&ui.title.credits);
  button_update(&ui.title.exit_game);


  if (!ui.title.lock_select) {
    if (input.move_vec.y > 0.1f) {
      ui.title.selected = (ui.title.selected + 1)%3;
    }
    if (input.move_vec.y < -0.1f) {
      if (ui.title.selected<=0) {
        ui.title.selected = 3;
      }
      ui.title.selected = ui.title.selected - 1;
    }
    ui.title.lock_select = true;
  }
  if (vector2_equals(input.move_vec, vector2_zero() )) {
    ui.title.lock_select = false;
  }


  if (ui.title.selected == 0) {
    ui.title.start.is_hovered = true;
    ui.title.start.is_down = input.action.is_down;
    ui.title.start.is_pressed = input.action.is_pressed;
    ui.title.start.is_released = input.action.is_released;
  } else if (ui.title.selected == 1) {
    ui.title.credits.is_hovered = true;
    ui.title.credits.is_down = input.action.is_down;
    ui.title.credits.is_pressed = input.action.is_pressed;
    ui.title.credits.is_released = input.action.is_released;
  } else if (ui.title.selected == 2) {
    ui.title.exit_game.is_hovered = true;
    ui.title.exit_game.is_down = input.action.is_down;
    ui.title.exit_game.is_pressed = input.action.is_pressed;
    ui.title.exit_game.is_released = input.action.is_released;
  }



  if (ui.title.start.is_pressed) {
    current_screen = SCREEN_GAMEPLAY;
  }

  if (ui.title.exit_game.is_released) {
#if defined(PLATFORM_WEB)
    report_error("Cannot exit on web");
#else
    should_exit = true;
#endif
  }




  if (ui.title.credits.is_pressed) {
    report_error("Not impolemented");
  }



}
static void draw_title() {
  clear_background( SLIGHTLY_RED_GRAY );
  button_draw(ui.title.start);
  button_draw(ui.title.credits);
  button_draw(ui.title.exit_game);
}

static void update_credits() {
}
static void draw_credits() {
}

// :iGameplay
static Gameplay gameplay_init() {
  Gameplay gameplay = {0};
  Camera2D camera = {
    .offset = (Vector2){screen_x_/2, screen_y_/2},
    .target = (Vector2){0.0f,0.0f},
    .rotation = 0.0f,
    .zoom = 1.0f,
  };
  gameplay.camera = camera;
  return gameplay;
}
static void update_gameplay() {

}
static void draw_gameplay() {
  begin_mode2_d(gameplay.camera);
  gameplay_draw_floor();

  for ( int i = 0; i < gameplay.units.count; ++i ) {
    unit_draw(gameplay.units.items[i]);
  }

  Unit* controlled = gameplay_get_controlled_unit_ref(&gameplay);
  if (controlled!=NULL) {
    unit_char_draw_controlled_overlay( *controlled );
  }

    end_mode2_d();
  player_stat_draw(gameplay.player_stats);

  input_draw(input);
}
static void gameplay_draw_floor() {
}
Unit* gameplay_get_controlled_unit_ref(Gameplay* gameplay) {
  return NULL;
}


// :iPlayerStat
void player_stat_draw(PlayerStat self) {}



// :impl
static void check_control_methods() {

  if ( is_mouse_button_down(MOUSE_BUTTON_LEFT) ) {
    // detected mouse
    if (input.is_mouse_enabled == false) {
      report_error("Mouse enabled");
    }
    input.is_mouse_enabled = true;
  }

  if ( get_touch_point_count() > 0 ) {
    // detected touch
    if (input.is_touch_enabled == false) {
      report_error("Touch enabled");
    }
    input.is_mouse_enabled = false;
    input.is_touch_enabled = true;
  }


    #define MAX_GAMEPADS                   4
    #define MAX_GAMEPAD_BUTTONS           32
  for (int igamepad =0; igamepad < MAX_GAMEPADS;++igamepad) {
   for (int i =0; i < MAX_GAMEPAD_BUTTONS;++i) {
     if ( is_gamepad_button_down(igamepad, i) ) {
       input.gamepad.id = igamepad;
       if (input.is_gamepad_enabled == false) {
         report_error("Gamepad enabled");
       }
       input.is_gamepad_enabled = true;
     }
   }
  }
}
void draw_boxed_text_centered(Rectangle rec, const char* text, float roundness, Color rec_color, Color text_color ) {
  Vector2 text_dim = measure_text_default(text, 32.0f);
  Vector2 text_offset = (Vector2){(rec.width - text_dim.x)/2.0f, (rec.height - text_dim.y)/2.0f};
  draw_rectangle_rounded(rec, roundness, 12, rec_color);
  draw_text_default(text, rec.x+text_offset.x, rec.y+text_offset.y, 32.0f, text_color);
}

static void report_error(const char* mess) {
  error_banner.text = mess;
  error_banner.anim = 0.0f;
}
static void draw_text_default(const char* text, int x, int y, float scale, Color color) {
    draw_text_ex(ab_font_get(assets, FONT_IOSEVKA), text, (Vector2){x,y}, scale, 0.0f, color);
}
static Vector2 measure_text_default(const char* text, float scale) {
  return measure_text_ex( get_font(FONT_IOSEVKA)  , text, scale, 0.0);
}
static int get_string_len(const char* s) { // i aint include string for stlen bru
  int i = 0;
  for (i=0; s[i]!='\0'; ++i) {
  }
  return i;
  // its readable i think
}
static Music get_music(EnumMusic music) {
  return ab_music_get(assets, music);
}
static Font get_font(EnumFont font) {
  return ab_font_get(assets, font);
}

static void resume_music_streamw(EnumMusic music) {
  resume_music_stream( get_music(music) );
}
static void set_music_volumew(EnumMusic music, float volume) {
  set_music_volume(get_music(music), assets.music_volumes[music]*volume);
}
static void update_music_streamw( EnumMusic music ) {
  update_music_stream( get_music(music) );
}
static void pause_music_streamw( EnumMusic music ) {
  pause_music_stream( get_music(music) );
}

// :iTalkBox
static TalkBox talk_box_new(const char* text, int x, int y, float scale, float speed, Color color) {
  TalkBox self = {0};
  self.text = text;
  self.x = x;
  self.y = y;
  self.scale = scale;
  self.speed = speed;
  self.color = color;
  self.finish = (float)( get_string_len(text) );
  self.progress = 0.0f;
  self.closed = false;
  self.is_background_talking = false;
  return self;
}
static TalkBox talk_box_vlad(const char* text) {
  return talk_box_new(text, vlad_talk_box.x, vlad_talk_box.y, vlad_talk_box.scale, vlad_talk_box.speed, vlad_talk_box.color);
}
static void talk_box_update(TalkBox* self) {
  if (self->closed) {
    return;
  }
  if (self->progress < self->finish) {
    self->progress += self->speed;
    if (!self->is_background_talking) {
      if (input.pointer.is_pressed || input.action.is_pressed) {
        self->progress = self->finish;
      }
    }
  } else {
    if (!self->is_background_talking) {
      if (input.pointer.is_pressed || input.action.is_pressed) {
          self->closed = true;
      }
    } else {
      self->progress += self->speed;
      if (self->progress > self->finish * 1.5f) {
          self->closed = true;
      }
    }
  }
}
static void talk_box_draw(TalkBox self) {
  if (self.closed) {
    return;
  }
  draw_text_default(text_format("%.*s", (int)(self.progress), self.text ), self.x, self.y, self.scale, self.color);
}
// :iInput
static Input input_init() {
  Input self = {0};
  self.is_touch_enabled = false;
  self.is_mouse_enabled = false;
  self.is_gamepad_enabled = false;
  self.deadzone= 0.2f;

  float r = 180.0f;
  self.left_stick.position = (Vector2){0.0f + r, screen_y_ - r - 40.0f};
  self.left_stick.radius = r-20.0f;
  self.right_stick.position = (Vector2){screen_x_ - r, screen_y_ - r - 40.0f};
  self.right_stick.radius = r-20.0f;

  self.keyboard.left_key = KEY_A;
  self.keyboard.right_key = KEY_D;
  self.keyboard.up_key = KEY_W;
  self.keyboard.down_key = KEY_S;

  self.keyboard.shoot_left_key  = KEY_H;
  self.keyboard.shoot_right_key = KEY_L;
  self.keyboard.shoot_up_key    = KEY_K;
  self.keyboard.shoot_down_key  = KEY_J;

  self.keyboard.action_key = KEY_SPACE;

  self.gamepad.id = -1;

  self.gamepad.move_axis_x = GAMEPAD_AXIS_LEFT_X;
  self.gamepad.move_axis_y = GAMEPAD_AXIS_LEFT_Y;
  self.gamepad.shoot_axis_x = GAMEPAD_AXIS_RIGHT_X;
  self.gamepad.shoot_axis_y = GAMEPAD_AXIS_RIGHT_Y;
  self.gamepad.action_btn = GAMEPAD_BUTTON_RIGHT_FACE_DOWN;

  return self;
}
static void input_update(Input* input) {

  input->shoot_vec   = vector2_zero();

  if (input->is_mouse_enabled) {
    input->pointer.position = get_mouse_position();
    input->pointer.is_pressed = is_mouse_button_pressed(MOUSE_BUTTON_LEFT);
    input->pointer.is_down= is_mouse_button_down(MOUSE_BUTTON_LEFT);
    input->pointer.is_released= is_mouse_button_released(MOUSE_BUTTON_LEFT);
  }

  input->action.is_pressed  = is_key_pressed (input->keyboard.action_key);
  input->action.is_down     = is_key_down    (input->keyboard.action_key);
  input->action.is_released = is_key_released(input->keyboard.action_key);

  if (input->is_touch_enabled) {
    if (current_screen == SCREEN_GAMEPLAY) {
      bool captured_left = true;
      bool captured_right = true;
      int count = get_touch_point_count();
      for (int i = 0; i < count; ++i) {
        Vector2 pos = get_touch_position(i);
        if (input->left_stick.captured_touch != -1) {
          if (input->left_stick.captured_touch == get_touch_point_id(i)) {
            input->move_vec = vector2_scale(vector2_subtract(pos, input->left_stick.position) , 1/input->left_stick.radius);
            captured_left = true;
          } else {
            captured_left = false;
          }
        } else {
          if (vector2_distance(input->left_stick.position, pos) < input->left_stick.radius) {
            // need to capture
            input->move_vec = vector2_scale(vector2_subtract(pos, input->left_stick.position) , 1/input->left_stick.radius);
            captured_left = true;
            continue;
          }
        }


        if (input->right_stick.captured_touch != -1) {
          if (input->right_stick.captured_touch == get_touch_point_id(i)) {
            input->shoot_vec = vector2_scale(vector2_subtract(pos, input->right_stick.position) , 1/input->right_stick.radius);
            captured_right = true;
          } else {
            captured_right = false;
          }
        } else {
          if (vector2_distance(input->right_stick.position, pos) < input->right_stick.radius) {
            // need to capture
            input->shoot_vec = vector2_scale(vector2_subtract(pos, input->right_stick.position) , 1/input->right_stick.radius);
            captured_right = true;
            continue;
          }
        }

      }


      if (!captured_left) {
        input->left_stick.captured_touch = -1;
      }
      if (!captured_right) {
        input->right_stick.captured_touch = -1;
      }

    } else {
      // raylib treats singletouch like a mouse
      input->pointer.position = get_mouse_position();
      input->pointer.is_pressed = is_mouse_button_pressed(MOUSE_BUTTON_LEFT);
      input->pointer.is_down= is_mouse_button_down(MOUSE_BUTTON_LEFT);
      input->pointer.is_released= is_mouse_button_released(MOUSE_BUTTON_LEFT);
    }


  } else {

    input->move_vec  = (Vector2){0.0f,0.0f};
    if ( is_key_down(input->keyboard.left_key) ) {
      input->move_vec.x -= 1.0f;
    }
    if ( is_key_down(input->keyboard.right_key) ) {
      input->move_vec.x += 1.0f;
    }
    if ( is_key_down(input->keyboard.up_key) ) {
      input->move_vec.y -= 1.0f;
    }
    if ( is_key_down(input->keyboard.down_key) ) {
      input->move_vec.y += 1.0f;
    }


    input->shoot_vec  = (Vector2){0.0f,0.0f};
    if ( is_key_down(input->keyboard.shoot_left_key) ) {
      input->shoot_vec.x -= 1.0f;
    }
    if ( is_key_down(input->keyboard.shoot_right_key) ) {
      input->shoot_vec.x += 1.0f;
    }
    if ( is_key_down(input->keyboard.shoot_up_key) ) {
      input->shoot_vec.y -= 1.0f;
    }
    if ( is_key_down(input->keyboard.shoot_down_key) ) {
      input->shoot_vec.y += 1.0f;
    }



    input->move_vec = vector2_normalize(input->move_vec);

    if ( input->is_gamepad_enabled && is_gamepad_available(input->gamepad.id) ) {
      if ( vector2_equals( input->move_vec, vector2_zero() ) ) {
        input->move_vec.x = get_gamepad_axis_movement(input->gamepad.id, input->gamepad.move_axis_x);
        input->move_vec.y = get_gamepad_axis_movement(input->gamepad.id, input->gamepad.move_axis_y);


        input->action.is_pressed  = is_gamepad_button_pressed (input->gamepad.id, input->gamepad.action_btn);
        input->action.is_down     = is_gamepad_button_down    (input->gamepad.id, input->gamepad.action_btn);
        input->action.is_released = is_gamepad_button_released(input->gamepad.id, input->gamepad.action_btn);
      }
        input->shoot_vec.x = get_gamepad_axis_movement(input->gamepad.id, input->gamepad.shoot_axis_x);
        input->shoot_vec.y = get_gamepad_axis_movement(input->gamepad.id, input->gamepad.shoot_axis_y);
    }

    if (input->is_mouse_enabled) {
      if (input->pointer.is_down) {
        input->shoot_vec = vector2_normalize( vector2_subtract(input->pointer.position, (Vector2){screen_x_/2,screen_y_/2}) );
      }
    }

    input->move_vec    = input_fix_vec(input->move_vec, input->deadzone, 1.0f);
    input->shoot_vec   = input_fix_vec(input->shoot_vec, input->deadzone, 1.0f);
  }

  // TODO: add touch buttons
}

static void input_draw(Input input) {
  if (input.is_touch_enabled) {
    virtual_stick_draw(input.left_stick,  input.move_vec);
    virtual_stick_draw(input.right_stick, input.shoot_vec);
  }
}

static Vector2 input_fix_vec(Vector2 unfixed, float deadzone, float maximum) {
  Vector2 normalized = vector2_normalize(unfixed);
  Vector2 zero = vector2_zero();
  float len = vector2_distance(unfixed, zero);
  if (len > maximum) {
    return vector2_scale( normalized, maximum);
  }
  if (len < deadzone) {
    return zero;
  }
  return unfixed; // its fixed already

}
// :iVirtualStick
void virtual_stick_draw(VirtualStick self, Vector2 control_vec) {
  draw_circle_v(self.position, self.radius, fade(WHITE,0.1));
  draw_circle_v(vector2_add( self.position, vector2_scale( control_vec, self.radius ) ), 40.0f, fade(WHITE,0.1));
}

// :iMusicPlayer
static MusicPlayer music_player_init() {
  MusicPlayer self = {0};
  self.fade = -67.0f;
  self.main_stream = MUSIC_SKY_VIBE;
  self.secondary_stream = MUSIC_PUNK_SOUND;
  self.fade_speed = 0.05f;
  return self;
}

static void music_player_update(MusicPlayer* self) {
  if (self->fade < -3.0f) {
      resume_music_streamw(self->main_stream);
      set_music_volumew(self->main_stream,1.0f);
      update_music_streamw( self->main_stream );
      return;
  }
  if (self->fade < -1.0f) {
      pause_music_streamw(self->main_stream);
      self->main_stream = self->secondary_stream;
      self->fade = -67.0f;
      resume_music_streamw(self->main_stream);
      set_music_volumew(self->main_stream,1.0f);
      update_music_streamw( self->main_stream );
      return;
  }
  self->fade -= self->fade_speed;
  if (self->fade > 0.0f) {
      pause_music_streamw(self->secondary_stream);
      resume_music_streamw(self->main_stream);
      set_music_volumew(self->main_stream,self->fade);
      update_music_streamw( self->main_stream );
      return;
  }
  if (self->fade < 0.0f) {
      pause_music_streamw(self->main_stream);
      resume_music_streamw(self->secondary_stream);
      set_music_volumew(self->secondary_stream, -self->fade);
      update_music_streamw( self->secondary_stream );
      return;
  }
  update_music_streamw( self->main_stream );
  update_music_streamw( self->secondary_stream);
}
static void music_player_fade_to(MusicPlayer* self, EnumMusic id) {
  pause_music_streamw(self->secondary_stream);
  self->secondary_stream = id;
  self->fade = 1.0f;
  resume_music_streamw(self->secondary_stream);
}



// :iUI
static UI ui_init() {
  UI ui = {0};
  ui.title = title_init();
  return ui;
}


// :iUnit
void unit_update(Unit* self) {}
void unit_draw(Unit self) {}
void unit_char_draw_controlled_overlay( Unit self ) {
}

// :iButton
static UIButton ui_button_new(int x, int y, int x_, int y_, const char* text) {
  UIButton self = {0};
  self.rec = (Rectangle){x,y,x_,y_};
  self.back_color = BLACK;
  self.text_color = WHITE;
  self.text = text;

  Vector2 text_dim = measure_text_default(text, 32.0f);
  self.text_offset = (Vector2){(self.rec.width - text_dim.x)/2.0f, (self.rec.height - text_dim.y)/2.0f};
  return self;
}
void button_draw(UIButton self) {
  Color c = self.back_color;
  if (self.is_hovered) {
    c = fade(c, 0.75);
  }
  if (self.is_down) {
     c = fade(c, 0.5);
  }
  draw_rectangle_rounded(self.rec, 0.4f, 12, c);
  draw_text_default(self.text, self.rec.x+self.text_offset.x, self.rec.y+self.text_offset.y, 32.0f, self.text_color);
}
void button_update(UIButton* self) {
  if ( check_collision_point_rec(input.pointer.position, self->rec) ) {
    self->is_hovered = true;
    if (self->was_pressed) {
      self->is_released = input.pointer.is_released;
      if (self->is_released) {
        self->was_pressed = false;
      }
    } else {
      self->is_released = false;
    }
    self->is_down = input.pointer.is_down;
    self->is_pressed = input.pointer.is_pressed;
    if (self->is_pressed) {
      self->was_pressed = true;
    }
    self->was_in_range = true;
  } else {
    self->is_hovered  = false;
    self->is_pressed  = false;
    self->is_down     = false;
    self->is_released = false;
    self->was_in_range = false;
  }
}
// :iErrorBanner
ErrorBanner error_banner_init() {
  ErrorBanner self = {0};
  self.text     = "";
  self.anim_end = 10.0f;
  self.anim_speed= 0.1f;
  self.anim     = self.anim_end;
  self.opacity  = 0.0f;
  self.rec = (Rectangle){ 0.0f, 0.0f, 720.0f, 200.0f };
  return self;
}
void error_banner_update(ErrorBanner *self) {
  float stage_0 = 0.0f;
  float stage_1 = self->anim_end / 3;
  float stage_2 = (self->anim_end) * 2 / 3;
  float stage_end = self->anim_end;
  if (self->anim < stage_end) {
    self->anim += self->anim_speed;
  }
  float progress = anim_get_stage_progress(self->anim, stage_0, stage_1);
  if (progress > 0.0f) {
    self->opacity = progress;
  }
  progress = anim_get_stage_progress(self->anim, stage_1, stage_2);
  if (progress > 0.0f) {
    self->opacity = 1.0f;
  }
  progress = anim_get_stage_progress(self->anim, stage_2, stage_end);
  if (progress > 0.0f) {
    self->opacity = 1.0f - progress;
  }

}
float anim_get_stage_progress(float overall_progress,float stage_start, float stage_end) {
  if (overall_progress > stage_start && overall_progress < stage_end) {
    return (overall_progress - stage_start) / (stage_end - stage_start); // 0.0f-1.0f
  }
  return -1.0f; //  not in that stage

}
void error_banner_draw(ErrorBanner self) {
  if (self.opacity > 0.2f) {
    draw_boxed_text_centered(self.rec, self.text, 0.0f, fade(BLACK, self.opacity), fade(RED, self.opacity));
  }
}
