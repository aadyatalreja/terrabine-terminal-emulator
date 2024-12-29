#include <SDL2/SDL.h>
#include <stdio.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define FONT_SIZE 20
#define MAX_OUTPUT_LINES 30

// Function to execute shell commands and capture output
void execute_shell_command(const char *command, char *output, size_t output_size) {
    FILE *fp;
    char buffer[1024];

    fp = popen(command, "r");
    if (fp == NULL) {
        snprintf(output, output_size, "Error: Failed to run command\n");
        return;
    }

    output[0] = '\0'; // Initialize the output buffer to an empty string
    while (fgets(buffer, sizeof(buffer) - 1, fp) != NULL) {
        strncat(output, buffer, output_size - strlen(output) - 1);
    }

    fclose(fp);
}

// Function to handle text rendering and return SDL_Texture*
SDL_Texture* render_text(SDL_Renderer *renderer, const char *text, TTF_Font *font, SDL_Color color, SDL_Rect *rect) {
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
    if (surface == NULL) {
        printf("Error rendering text: %s\n", SDL_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        printf("Error creating texture: %s\n", SDL_GetError());
    }

    SDL_FreeSurface(surface);
    SDL_QueryTexture(texture, NULL, NULL, &rect->w, &rect->h); // Get the width and height of the text
    return texture;
}

// Callback to handle when user presses Enter
void handle_user_input(const char *input, char *output, size_t output_size) {
    if (input != NULL && input[0] != '\0') {
        execute_shell_command(input, output, output_size);
    }
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL Initialization failed: %s\n", SDL_GetError());
        return -1;
    }

    if (TTF_Init() == -1) {
        printf("TTF Initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("SDL2 Terminal Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Load font for text rendering
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/jetbrains-mono/JetBrainsMono-Regular.ttf", FONT_SIZE);
    if (font == NULL) {
        printf("Error loading font: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    // Define text colors
    SDL_Color textColor = {255, 255, 255, 255}; // White text
    SDL_Rect textRect = {10, 10, 0, 0};

    char inputBuffer[256] = ""; // User input buffer
    char outputBuffer[2048] = ""; // Command output buffer

    int quit = 0;
    SDL_Event event;

    // Main loop
    while (!quit) {
        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
        SDL_RenderClear(renderer);

        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            } else if (event.type == SDL_TEXTINPUT) {
                // Handle user text input (update input buffer)
                strncat(inputBuffer, event.text.text, sizeof(inputBuffer) - strlen(inputBuffer) - 1);
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_RETURN) {
                    // Handle Enter key (execute the command)
                    handle_user_input(inputBuffer, outputBuffer, sizeof(outputBuffer));
                    inputBuffer[0] = '\0'; // Clear the input buffer after execution
                } else if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(inputBuffer) > 0) {
                    // Handle backspace key
                    inputBuffer[strlen(inputBuffer) - 1] = '\0';
                }
            }
        }

        // Render the input buffer (command)
        SDL_Texture *inputTexture = render_text(renderer, inputBuffer, font, textColor, &textRect);
        if (inputTexture) {
            SDL_RenderCopy(renderer, inputTexture, NULL, &textRect);
            SDL_DestroyTexture(inputTexture); // Clean up texture after rendering
        }

        // Render the command output
        SDL_Rect outputRect = {10, 40, 0, 0}; // Adjust y-position to avoid overlap with input
        SDL_Texture *outputTexture = render_text(renderer, outputBuffer, font, textColor, &outputRect);
        if (outputTexture) {
            SDL_RenderCopy(renderer, outputTexture, NULL, &outputRect);
            SDL_DestroyTexture(outputTexture); // Clean up texture after rendering
        }

        // Present the renderer
        SDL_RenderPresent(renderer);

        // Delay to avoid high CPU usage
        SDL_Delay(16);
    }

    // Clean up and exit
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
