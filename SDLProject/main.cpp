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

#define LOG(argument) std::cout << argument << '\n'

//WINDOW CONSTANTS
const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

float BG_RED = 0.0f,
      BG_BLUE = 0.0f,
      BG_GREEN = 0.0f,
      BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
            VIEWPORT_Y = 0,
            VIEWPORT_WIDTH = WINDOW_WIDTH,
            VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const float DEGREES_PER_SECOND = 90.0f;
float g_previous_ticks = 0;

SDL_Window* g_display_window;
bool g_game_is_running = true;

//texture values
const int NUMBER_OF_TEXTURES = 1; 
const GLint LEVEL_OF_DETAIL = 0;  
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero

//sprite paths
const char EARTH_SPRITE_FILEPATH[] = "earth.png";
const char MOON_SPRITE_FILEPATH[] = "moon.png";

//scaling constants
float g_earth_scaling = 1.0f, factor = 1.0f, g_factor = 1.0f, earth_speed = 0.0f;

//transformation constants
//earth
float g_earth_x = 0.0f, g_earth_y = 0.0f;
const float G_BOUNCE_RADIUS = 1.0f, G_BOUNCE_SPEED = 2.0f;
//moon
float g_moon_x = 0.0f, g_moon_y = 0.0f, 
      g_moon_radians = 0, g_orbit_angle = 0.0f;
const float G_ORBIT_SPEED = 0.8f, G_ORBIT_RADIUS = 2.0f;

ShaderProgram g_program;

//matrices
glm::mat4 g_view_matrix,
          g_projection_matrix,
          g_earth_model_matrix,   //earths matrix
          g_moon_model_matrix;    //moons matrix

//textures
GLuint g_earth_texture_id;
GLuint g_moon_texture_id;

GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);

    return textureID;
}

//swaps the background color from purple to black.
void swap_background_color() {
    if (BG_BLUE == 0.0f) {
        BG_RED = 0.5f;
        BG_GREEN = 0.3f;
        BG_BLUE = 0.5f;
    }
    else {
        BG_RED = 0.0f;
        BG_GREEN = 0.0f;
        BG_BLUE = 0.0f;
    }
    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);
    glClear(GL_COLOR_BUFFER_BIT);
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Earth + Moon",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    g_earth_model_matrix = glm::mat4(1.0f);  
    g_moon_model_matrix = glm::mat4(1.0f);  
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.

    g_program.set_projection_matrix(g_projection_matrix);
    g_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_earth_texture_id = load_texture(EARTH_SPRITE_FILEPATH);
    g_moon_texture_id = load_texture(MOON_SPRITE_FILEPATH);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_game_is_running = false;
        }
    }
}

float g_previous_earth_y = 0.0f;
void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; //get current ticks
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    //earth 
    //bounce
    earth_speed += G_BOUNCE_SPEED * delta_time;
    g_earth_y = sin(earth_speed) * G_BOUNCE_RADIUS;

    //change background around middle
    if ((g_previous_earth_y <= 0 && g_earth_y > 0) || (g_previous_earth_y >= 0 && g_earth_y < 0)) {
        swap_background_color();
    }

    g_previous_earth_y = g_earth_y;

    //scaling
    if (g_earth_scaling > 1.2f) {
        g_factor = -factor;

    }
    else if (g_earth_scaling < 1.0f) {
        g_factor = factor;
    }
    g_earth_scaling += g_factor * delta_time;

    //moon
    //orbiting
    g_orbit_angle += G_ORBIT_SPEED * delta_time;
    g_moon_x = g_earth_x + cos(g_orbit_angle) * G_ORBIT_RADIUS;  //uses earth's cords to move in relation to it
    g_moon_y = g_earth_y + sin(g_orbit_angle) * G_ORBIT_RADIUS; 

    //rotation
    g_moon_radians += -130.0f * delta_time;

    //earth-- apply transformations
    g_earth_model_matrix = glm::mat4(1.0f);
    g_earth_model_matrix = glm::translate(g_earth_model_matrix, glm::vec3(g_earth_x, g_earth_y, 0.0f));
    g_earth_model_matrix = glm::scale(g_earth_model_matrix, glm::vec3(g_earth_scaling, g_earth_scaling, 1.0f));

    //moon-- apply transformations
    g_moon_model_matrix = glm::mat4(1.0f);
    g_moon_model_matrix = glm::translate(g_moon_model_matrix, glm::vec3(g_moon_x, g_moon_y, 0.0f));
    g_moon_model_matrix = glm::rotate(g_moon_model_matrix, glm::radians(g_moon_radians), glm::vec3(0.0f, 0.0f, 1.0f));
}


void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    g_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); 
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    g_program.set_model_matrix(g_moon_model_matrix);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_program.get_position_attribute());
    
    glVertexAttribPointer(g_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_program.get_tex_coordinate_attribute());

    draw_object(g_earth_model_matrix, g_earth_texture_id);
    draw_object(g_moon_model_matrix, g_moon_texture_id);

    glDisableVertexAttribArray(g_program.get_position_attribute());
    glDisableVertexAttribArray(g_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }


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
