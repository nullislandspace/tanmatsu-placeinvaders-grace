#include <stdio.h>
#include <string.h>
#include "bsp/device.h"
#include "bsp/display.h"
#include "bsp/input.h"
#include "bsp/power.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "esp_log.h"
#include "hal/lcd_types.h"
#include "nvs_flash.h"

#include "game.h"
#include "render.h"
#include "audio.h"

static const char TAG[] = "placeinvaders";

static size_t                       display_h_res        = 0;
static size_t                       display_v_res        = 0;
static lcd_color_rgb_pixel_format_t display_color_format = LCD_COLOR_PIXEL_FORMAT_RGB888;
static lcd_rgb_data_endian_t        display_data_endian  = LCD_RGB_DATA_ENDIAN_LITTLE;
static QueueHandle_t                input_event_queue    = NULL;

void app_main(void) {
    // GPIO interrupt service
    gpio_install_isr_service(0);

    // Initialize NVS
    esp_err_t res = nvs_flash_init();
    if (res == ESP_ERR_NVS_NO_FREE_PAGES || res == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        res = nvs_flash_init();
    }
    ESP_ERROR_CHECK(res);

    // Initialize BSP (request RGB888 for direct framebuffer access)
    const bsp_configuration_t bsp_configuration = {
        .display = {
            .requested_color_format = LCD_COLOR_PIXEL_FORMAT_RGB888,
            .num_fbs = 1,
        },
    };
    ESP_ERROR_CHECK(bsp_device_initialize(&bsp_configuration));

    // Get display parameters
    ESP_ERROR_CHECK(bsp_display_get_parameters(
        &display_h_res, &display_v_res, &display_color_format, &display_data_endian));

    ESP_LOGI(TAG, "Display: %dx%d", display_h_res, display_v_res);

    // Get input event queue
    ESP_ERROR_CHECK(bsp_input_get_queue(&input_event_queue));

    // Allocate framebuffer for direct rendering
    size_t fb_size = display_h_res * display_v_res * 3;  // RGB888
    uint8_t* framebuffer = heap_caps_malloc(fb_size, MALLOC_CAP_SPIRAM);
    if (!framebuffer) {
        ESP_LOGE(TAG, "Failed to allocate framebuffer (%d bytes)", fb_size);
        return;
    }
    memset(framebuffer, 0, fb_size);

    // Initialize audio
    audio_init();

    // Initialize game
    game_t game;
    memset(&game, 0, sizeof(game));
    game.highscore = highscore_load();
    game.state = STATE_TITLE;
    game.frame = 0;

    // Set up vsync
    SemaphoreHandle_t te_semaphore = NULL;
    bsp_display_set_tearing_effect_mode(BSP_DISPLAY_TE_V_BLANKING);
    bsp_display_get_tearing_effect_semaphore(&te_semaphore);

    // Input state tracking
    bool prev_fire = false;

    ESP_LOGI(TAG, "Starting game loop");

    while (1) {
        // Drain event queue (catch power button, etc.)
        bsp_input_event_t event;
        while (xQueueReceive(input_event_queue, &event, 0) == pdTRUE) {
            if (event.type == INPUT_EVENT_TYPE_NAVIGATION &&
                event.args_navigation.key == BSP_INPUT_NAVIGATION_KEY_F1) {
                bsp_device_restart_to_launcher();
            }
        }

        // Poll input keys directly
        bool key_left = false, key_right = false, key_space = false, key_esc = false;
        bsp_input_read_navigation_key(BSP_INPUT_NAVIGATION_KEY_LEFT, &key_left);
        bsp_input_read_navigation_key(BSP_INPUT_NAVIGATION_KEY_RIGHT, &key_right);
        bsp_input_read_scancode(BSP_INPUT_SCANCODE_SPACE, &key_space);
        bsp_input_read_scancode(BSP_INPUT_SCANCODE_ESC, &key_esc);

        // Rising-edge fire detection
        bool fire = key_space && !prev_fire;
        prev_fire = key_space;

        // ESC returns to launcher
        if (key_esc) {
            bsp_device_restart_to_launcher();
        }

        // Game state machine
        switch (game.state) {
            case STATE_TITLE:
                game.frame++;
                if (fire) {
                    game_init(&game, game.highscore);
                }
                render_title_screen(framebuffer, &game);
                break;

            case STATE_PLAYING:
                game_update(&game, key_left, key_right, fire);
                render_game(framebuffer, &game);
                break;

            case STATE_PLAYER_DEATH:
                game.frame++;
                game.anim_timer--;
                if (game.anim_timer == 0) {
                    if (game.lives > 0) {
                        game.state = STATE_PLAYING;
                        game.player_x = PLAYER_START_X;
                        for (int i = 0; i < MAX_PLAYER_BULLETS; i++) game.player_bullets[i].active = false;
                        for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
                            game.alien_bullets[i].active = false;
                        }
                    } else {
                        game.state = STATE_GAME_OVER;
                        if (game.score > game.highscore) {
                            game.highscore = game.score;
                            highscore_save(game.score);
                        }
                    }
                }
                render_game(framebuffer, &game);
                break;

            case STATE_LEVEL_CLEAR:
                game.frame++;
                game.anim_timer--;
                if (game.anim_timer == 0) {
                    game.level++;
                    game_new_wave(&game);
                    game.state = STATE_PLAYING;
                }
                render_game(framebuffer, &game);
                break;

            case STATE_GAME_OVER:
                game.frame++;
                if (fire) {
                    game.state = STATE_TITLE;
                }
                render_game_over(framebuffer, &game);
                break;
        }

        // Wait for vsync and blit
        if (te_semaphore) {
            xSemaphoreTake(te_semaphore, pdMS_TO_TICKS(50));
        }
        bsp_display_blit(0, 0, display_h_res, display_v_res, framebuffer);

        // Target ~30fps if no vsync
        if (!te_semaphore) {
            vTaskDelay(pdMS_TO_TICKS(16));
        }
    }
}
