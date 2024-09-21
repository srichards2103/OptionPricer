#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <cmath>
#include <string>
#include "matplotlibcpp.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define GLAD_GLAPI_EXPORT
#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <stdexcept> // Include for std::runtime_error
#include <sstream>   // Include for std::stringstream

// Namespace for matplotlib-cpp
namespace plt = matplotlibcpp;

// Function declarations
double cumulativeNormal(double x);
double blackScholesPrice(bool isCall, double S, double K, double T, double r, double sigma);
void generateHeatmap(bool isCall, double S, double K, double T, double r, double sigma, const std::string &filename);
GLuint LoadTextureFromFile(const char *filename);

int main(int, char **)
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Set OpenGL attributes
    const char *glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
#if __APPLE__
    // GL 3.2 Core + GLSL 150
    glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with OpenGL context
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window *window = SDL_CreateWindow("Option Pricing Application", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // ImGuiIO &io = ImGui::GetIO(); (void)io;
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Variables for user inputs
    double S = 100.0;   // Stock Price
    double K = 100.0;   // Strike Price
    double T = 1.0;     // Time to Maturity (years)
    double r = 0.05;    // Risk-Free Rate
    double sigma = 0.2; // Volatility
    bool isCall = true; // Option Type

    double optionPrice = 0.0;
    std::string heatmapFilename = "heatmap.png";

    bool showHeatmap = false;
    GLuint heatmapTexture = 0;

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // GUI Layout
        {
            ImGui::Begin("Option Pricing - Black-Scholes Model");

            ImGui::Text("Enter Option Parameters:");

            ImGui::InputDouble("Stock Price (S)", &S, 1.0, 10.0, "%.2f");
            ImGui::InputDouble("Strike Price (K)", &K, 1.0, 10.0, "%.2f");
            ImGui::InputDouble("Time to Maturity (T in years)", &T, 0.1, 1.0, "%.2f");
            ImGui::InputDouble("Risk-Free Rate (r)", &r, 0.01, 0.05, "%.4f");
            ImGui::InputDouble("Volatility (σ)", &sigma, 0.01, 0.05, "%.4f");

            // Option Type
            if (ImGui::RadioButton("Call", isCall))
                isCall = true;
            ImGui::SameLine();
            if (ImGui::RadioButton("Put", !isCall))
                isCall = false;

            // Calculate Button
            if (ImGui::Button("Calculate Price"))
            {
                optionPrice = blackScholesPrice(isCall, S, K, T, r, sigma);
            }

            // Display Option Price
            ImGui::Separator();
            ImGui::Text("Option Price: $%.4f", optionPrice);

            // Generate Heatmap Button
            if (ImGui::Button("Generate Heatmap"))
            {
                generateHeatmap(isCall, S, K, T, r, sigma, heatmapFilename);
                showHeatmap = true;

                // Load the heatmap texture
                if (heatmapTexture)
                    glDeleteTextures(1, &heatmapTexture); // Delete previous texture if exists
                heatmapTexture = LoadTextureFromFile(heatmapFilename.c_str());

                if (heatmapTexture == 0)
                {
                    ImGui::OpenPopup("Error");
                }
            }

            // Display Heatmap
            if (showHeatmap && heatmapTexture)
            {
                ImGui::Separator();
                ImGui::Text("Heatmap (Stock Price vs. Volatility):");
                ImGui::Image((void *)(intptr_t)heatmapTexture, ImVec2(400, 400));
            }

            // Handle Error Popup
            if (ImGui::BeginPopup("Error"))
            {
                ImGui::Text("Failed to load heatmap image.");
                if (ImGui::Button("OK"))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    if (heatmapTexture)
        glDeleteTextures(1, &heatmapTexture);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

// Implementation of cumulative normal distribution function
double cumulativeNormal(double x)
{
    return 0.5 * (1.0 + std::erf(x / std::sqrt(2.0)));
}

// Implementation of Black-Scholes formula
double blackScholesPrice(bool isCall, double S, double K, double T, double r, double sigma)
{
    double d1 = (std::log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));
    double d2 = d1 - sigma * std::sqrt(T);

    if (isCall)
    {
        return S * cumulativeNormal(d1) - K * std::exp(-r * T) * cumulativeNormal(d2);
    }
    else
    {
        return K * std::exp(-r * T) * cumulativeNormal(-d2) - S * cumulativeNormal(-d1);
    }
}

// Function to generate heatmap using matplotlib-cpp
void generateHeatmap(bool isCall, double S, double K, double T, double r, double sigma, const std::string &filename)
{
    // Define ranges for Stock Price and Volatility
    int S_steps = 100;
    int sigma_steps = 100;
    double S_min = S * 0.5;
    double S_max = S * 1.5;
    double sigma_min = sigma * 0.5;
    double sigma_max = sigma * 1.5;

    std::vector<std::vector<float>> heatmap(S_steps, std::vector<float>(sigma_steps, 0.0f));

    // Populate heatmap data
    for (int i = 0; i < S_steps; ++i)
    {
        double current_S = S_min + (S_max - S_min) * i / (S_steps - 1);
        for (int j = 0; j < sigma_steps; ++j)
        {
            double current_sigma = sigma_min + (sigma_max - sigma_min) * j / (sigma_steps - 1);
            heatmap[i][j] = static_cast<float>(blackScholesPrice(isCall, current_S, K, T, r, current_sigma));
        }
    }

    // Flatten the 2D heatmap into a 1D array
    std::vector<float> flat_heatmap;
    flat_heatmap.reserve(S_steps * sigma_steps);
    for (const auto &row : heatmap)
        flat_heatmap.insert(flat_heatmap.end(), row.begin(), row.end());

    // Plot heatmap using matplotlib-cpp
    plt::figure_size(800, 600);

    // Declare a PyObject* variable to capture the image object
    PyObject *img = nullptr;

    // Format the 'extent' as a string "[xmin, xmax, ymin, ymax]"
    std::stringstream extent_ss;
    extent_ss << "[" << S_min << ", " << S_max << ", " << sigma_min << ", " << sigma_max << "]";
    std::string extent_str = extent_ss.str();

    // Call imshow with &img to capture the returned PyObject*
    plt::imshow(flat_heatmap.data(), S_steps, sigma_steps, 1, {{"extent", extent_str}, {"origin", "lower"}, {"aspect", "auto"}, {"cmap", "viridis"}}, &img);

    if (img)
    {
        // Pass the image object to colorbar
        plt::colorbar(img);
    }
    else
    {
        // Handle the error appropriately
        throw std::runtime_error("Failed to create image for heatmap.");
    }

    plt::xlabel("Stock Price (S)");
    plt::ylabel("Volatility (σ)");
    std::string optionType = isCall ? "Call" : "Put";
    plt::title("Option Price Heatmap (" + optionType + " Option)");
    plt::save(filename);
    plt::close();
}

// Function to load texture from file
GLuint LoadTextureFromFile(const char *filename)
{
    int width, height, channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 4);
    if (data == NULL)
        return 0;

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return texture;
}
