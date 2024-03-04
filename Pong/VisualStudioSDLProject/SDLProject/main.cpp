#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum Coordinate
{
    x_coordinate,
    y_coordinate
};

#define LOG(argument) std::cout << argument << '\n'

/* BALL VARIABLES*/

// STRUCT - was gonna do extra credit but ur girl needs sleep
struct Ball {
    glm::mat4 model_matrix;
    glm::vec3 position_vec = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 movement_vec = glm::vec3(0.0f, 0.0f, 0.0f);
    bool x_flip = false, y_flip = false;
};

//our first and sadly the only ball
Ball g_first_ball = Ball();

/* BALL CONSTANTS */
const float BALL_SPEED = 4.0f, PLAYER_SPEED = 2.0f;
glm::vec3 BALL_SIZE(0.5f, 0.5f, 0.0f);

bool bot_flip = false;

/* WINDOW CONSTANTS*/
const int WINDOW_WIDTH = 900,
    WINDOW_HEIGHT = 600;

const float WINDOW_BOUND = 2.4f;

const float BG_RED = 0.0f,
    BG_BLUE = 0.0f,
    BG_GREEN = 0.0f,
    BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
    VIEWPORT_Y = 0,
    VIEWPORT_WIDTH = WINDOW_WIDTH,
    VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
    F_SHADER_PATH[] = "shaders/fragment_textured.glsl";


/* TEXTURE CONSTANTS */
const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero

/* FILEPATHS */
const char PLAYER_SPRITE_FILEPATH[] = "lightsabers.png";
const char BALL_SPRITE_FILEPATH[] = "ball.png";
const char GAME_OVER_FILEPATH[] = "game_over.png";

/* TEXTURE IDS */
GLuint g_player_texture_id;// will use same texture for both players.
GLuint g_ball_texture_id;
GLuint g_game_over_texture_id;


/* GAME CONSTANTS */
SDL_Window* g_display_window;
bool g_game_is_running = true;
bool g_is_growing = true;
bool g_game_over = false;

/* DELTA TIME*/
const float MILLISECONDS_IN_SECOND = 1000.0;
float g_previous_ticks = 0.0f;

bool g_npc = false;

/* SHADER */
ShaderProgram g_shader_program;

/* MATRICES */
glm::mat4 view_matrix, 
        g_player_one_model_matrix, 
        g_player_two_model_matrix,
        g_projection_matrix, 
        g_trans_matrix;

/* PLAYER VARIABLES*/
/* JOYSTICKS*/
SDL_Joystick* g_player_one_controller; 
SDL_Joystick* g_player_two_controller;

/* CONSTANTS */
glm::vec3 PLAYER_SIZE(0.5f, 3.0f, 0.0f);

// position vectors
glm::vec3 g_player_one_position = glm::vec3(4.0f, 0.0f, 0.0f);
glm::vec3 g_player_two_position = glm::vec3(-4.0f, 0.0f, 0.0f);

// movement vectors
glm::vec3 g_player_one_movement = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_player_two_movement = glm::vec3(0.0f, 0.0f, 0.0f);

/* functions */
void shutdown();
void game_over();


float get_screen_to_ortho(float coordinate, Coordinate axis)
{
    switch (axis) {
    case x_coordinate:
        return ((coordinate / WINDOW_WIDTH) * 10.0f) - (10.0f / 2.0f);
    case y_coordinate:
        return (((WINDOW_HEIGHT - coordinate) / WINDOW_HEIGHT) * 7.5f) - (7.5f / 2.0);
    default:
        return 0.0f;
    }
}

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        LOG(filepath);
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

    // Open the first controller found. Returns null on error
    g_player_one_controller = SDL_JoystickOpen(0);
    g_player_two_controller = SDL_JoystickOpen(0);

    g_display_window = SDL_CreateWindow("Saber Wars",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.Load(V_SHADER_PATH, F_SHADER_PATH);

    g_player_one_model_matrix = glm::mat4(1.0f);
    g_player_two_model_matrix = glm::mat4(1.0f);
    g_first_ball.model_matrix = glm::mat4(1.0f);

    view_matrix = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.

    g_shader_program.SetProjectionMatrix(g_projection_matrix);
    g_shader_program.SetViewMatrix(view_matrix);
    // Notice we haven't set our model matrix yet!

    glUseProgram(g_shader_program.programID);

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_player_texture_id = load_texture(PLAYER_SPRITE_FILEPATH);
    g_ball_texture_id = load_texture(BALL_SPRITE_FILEPATH);
    g_game_over_texture_id = load_texture(GAME_OVER_FILEPATH);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.SetModelMatrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void check_paddle_collision(Ball& ball) {

    if (g_player_one_position.y - 3.5f < ball.position_vec.y && ball.position_vec.y < g_player_one_position.y + 3.5f &&
        (ball.position_vec.x > 8.1f)) {
        ball.x_flip = !ball.x_flip;
        ball.y_flip = !ball.y_flip;
    }
    if (g_player_two_position.y - 3.5f < ball.position_vec.y && ball.position_vec.y < g_player_two_position.y + 3.5f &&
       (ball.position_vec.x < -8.0f)) {
        ball.x_flip = !ball.x_flip;
        ball.y_flip = !ball.y_flip;
    }
}

void check_wall_collision(Ball& ball) {
    //end game if collide with side
    if (ball.position_vec.x > 9.5f) {
        game_over();
    }
    else if (ball.position_vec.x < -9.5f) {
        game_over();
    }

    //bounce if collide with top or bottom
    if (ball.position_vec.y > 7.0f) {
        ball.y_flip = true;
    }
    else if (ball.position_vec.y < -7.0f) {
        ball.y_flip = false;
    }

    //set the new speed
    if (ball.x_flip && ball.y_flip) {
        ball.movement_vec = glm::vec3(-BALL_SPEED, -BALL_SPEED, 0.0f);
    }
    else if (g_first_ball.x_flip) {
        ball.movement_vec = glm::vec3(-BALL_SPEED, BALL_SPEED, 0.0f);
    }
    else if (g_first_ball.y_flip) {
        ball.movement_vec = glm::vec3(BALL_SPEED, -BALL_SPEED, 0.0f);
    }
    else {
        ball.movement_vec = glm::vec3(BALL_SPEED, BALL_SPEED, 0.0f);
    }
}

void process_input()
{
    g_player_one_movement = glm::vec3(0.0f);
    g_player_two_movement = glm::vec3(0.0f);

    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_WINDOWEVENT_CLOSE:
        case SDL_QUIT:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                g_game_is_running = false;
                break;
            case SDLK_t: // the auto bot key
                g_npc = !g_npc;
                break;
            default:
                break;
            }
        default:
            break;
        }
    }

    const Uint8* key_states = SDL_GetKeyboardState(NULL); // array of key states [0, 0, 1, 0, 0, ...]

    //control up and down for player one
    if (key_states[SDL_SCANCODE_UP] && g_player_one_position.y < WINDOW_BOUND)
    {
        g_player_one_movement.y = PLAYER_SPEED;
    }
    else if (key_states[SDL_SCANCODE_DOWN] && g_player_one_position.y > -WINDOW_BOUND)
    {
        g_player_one_movement.y = -PLAYER_SPEED;
    }

    if (glm::length(g_player_one_movement) > PLAYER_SPEED)
    {
        g_player_one_movement = glm::normalize(g_player_one_movement);
    }
    if (!g_npc) {
        //control up and down for player two
        if (key_states[SDL_SCANCODE_W] && g_player_two_position.y < WINDOW_BOUND)
        {
            g_player_two_movement.y = PLAYER_SPEED;
        }
        else if (key_states[SDL_SCANCODE_S] && g_player_two_position.y > -WINDOW_BOUND)
        {
            g_player_two_movement.y = -PLAYER_SPEED;
        }

        if (glm::length(g_player_two_movement) > PLAYER_SPEED)
        {
            g_player_two_movement = glm::normalize(g_player_two_movement);
        }
    }
    else {
        g_player_two_movement = glm::vec3(0.0f, PLAYER_SPEED, 0.0f);
        if (g_player_two_position.y > WINDOW_BOUND || g_player_two_position.y < -WINDOW_BOUND) {
            bot_flip = !bot_flip;
        }
        if (bot_flip) {
            g_player_two_movement = glm::vec3(0.0f, -PLAYER_SPEED, 0.0f);
        }
    }


    check_paddle_collision(g_first_ball);

    check_wall_collision(g_first_ball);

}

void game_over() {

    g_game_over = true;
    glClear(GL_COLOR_BUFFER_BIT);

    float end_vertices[] = {
        -3.0f, -2.0f, 3.0f, -2.0f, 3.0f, 2.0f,  // triangle 1
        -3.0f, -2.0f, 3.0f, 2.0f, -3.0f, 2.0f   // triangle 2
    };
    float game_over_texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,  // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f   // triangle 2
    };

    glVertexAttribPointer(g_shader_program.positionAttribute, 2, GL_FLOAT, false, 0, end_vertices);
    glEnableVertexAttribArray(g_shader_program.positionAttribute);

    glEnableVertexAttribArray(g_shader_program.texCoordAttribute); // enable texture coordinate attribute

    // bind texture and draw player one
    glVertexAttribPointer(g_shader_program.texCoordAttribute, 2, GL_FLOAT, false, 0, game_over_texture_coordinates);
    glm::mat4 g_end_screen_mm = glm::mat4(1.0f);
    g_end_screen_mm = glm::translate(g_end_screen_mm, glm::vec3(0.0f));
    draw_object(g_end_screen_mm, g_game_over_texture_id);

    SDL_GL_SwapWindow(g_display_window);
}


void update()
{
    /* DELTA TIME */
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;

    // update player positions
    g_player_one_position += g_player_one_movement * delta_time * 1.0f;
    g_player_two_position += g_player_two_movement * delta_time * 1.0f;
    g_first_ball.position_vec += g_first_ball.movement_vec * delta_time * 1.0f;
    
    g_player_one_model_matrix = glm::mat4(1.0f);
    g_player_one_model_matrix = glm::translate(g_player_one_model_matrix, g_player_one_position);

    g_player_two_model_matrix = glm::mat4(1.0f);
    g_player_two_model_matrix = glm::translate(g_player_two_model_matrix, g_player_two_position);

    g_first_ball.model_matrix = glm::mat4(1.0f);
    g_first_ball.model_matrix += glm::translate(glm::mat4(1.0f), g_first_ball.position_vec);
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    if (!g_game_over) {
        // Vertices
        float paddle_vertices[] = {
            -1.0f, -1.5f, 1.0f, -1.5f, 1.0f, 1.5f,  // triangle 1
            -1.0f, -1.5f, 1.0f, 1.5f, -1.0f, 1.5f   // triangle 2
        };
        float ball_vertices[] = {
            -0.25f, -0.25f, 0.25f, -0.25f, 0.25f, 0.25f,  // triangle 1
            -0.25f, -0.25f, 0.25f, 0.25f, -0.25f, 0.25f   // triangle 2
        };
        // Textures
        float player_one_texture_coordinates[] = {
            0.0f, 1.0f, 0.5f, 1.0f, 0.5f, 0.0f,     // triangle 1
            0.0f, 1.0f, 0.5f, 0.0f, 0.0f, 0.0f     // triangle 2
        };

        float player_two_texture_coordinates[] = {
            0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
            0.5f, 1.0f, 1.0f, 0.0f, 0.5f, 0.0f
        };
        float ball_texture_coordinates[] = {
            0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,  // triangle 1
            0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f   // triangle 2
        };

        glVertexAttribPointer(g_shader_program.positionAttribute, 2, GL_FLOAT, false, 0, paddle_vertices);
        glEnableVertexAttribArray(g_shader_program.positionAttribute);

        glEnableVertexAttribArray(g_shader_program.texCoordAttribute); // Enable texture coordinate attribute

        // Bind texture and draw player one
        glVertexAttribPointer(g_shader_program.texCoordAttribute, 2, GL_FLOAT, false, 0, player_one_texture_coordinates);
        draw_object(g_player_one_model_matrix, g_player_texture_id);

        // Bind texture and draw player two
        glVertexAttribPointer(g_shader_program.texCoordAttribute, 2, GL_FLOAT, false, 0, player_two_texture_coordinates);
        draw_object(g_player_two_model_matrix, g_player_texture_id);

        glDisableVertexAttribArray(g_shader_program.positionAttribute);
        glDisableVertexAttribArray(g_shader_program.texCoordAttribute);

        //draw ball
        glVertexAttribPointer(g_shader_program.positionAttribute, 2, GL_FLOAT, false, 0, ball_vertices);
        glEnableVertexAttribArray(g_shader_program.positionAttribute);

        glVertexAttribPointer(g_shader_program.texCoordAttribute, 2, GL_FLOAT, false, 0, ball_texture_coordinates);
        glEnableVertexAttribArray(g_shader_program.texCoordAttribute);

        //if(!lost)
        draw_object(g_first_ball.model_matrix, g_ball_texture_id);
        //else{ draw game over texture }

        glDisableVertexAttribArray(g_shader_program.positionAttribute);
        glDisableVertexAttribArray(g_shader_program.texCoordAttribute);

        SDL_GL_SwapWindow(g_display_window);
    }
    else {

        game_over();
    }
}

void shutdown()
{
    SDL_JoystickClose(g_player_one_controller);
    SDL_JoystickClose(g_player_two_controller);
    SDL_Quit();
}

/**
 Start here—we can see the general structure of a game loop without worrying too much about the details yet.
 */
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
