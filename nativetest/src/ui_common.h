#pragma once

struct sRME_State;

struct sUiBindings {
    const char* name;
    /// @brief Initialise the UI
    void (*init)(void);
    /// @brief Clean up before exiting the emulator
    void (*deinit)(void);
    /// @brief The emulator has halted, prompt for input to continue/exit
    void (*halted)(const char* message);
    /// @brief Poll for new input events, updating the keyboard (or redrawing the screen)
    void (*poll_events)(struct sRME_State* State);
};

#ifdef ENABLE_SDL
extern const struct sUiBindings cUiBindings_Sdl;
#endif
extern const struct sUiBindings cUiBindings_Tty;