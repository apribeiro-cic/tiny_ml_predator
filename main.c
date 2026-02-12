#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/sensors/sensors.h"
#include "tflm_wrapper.h"
#include "modelo_predator.h" // Onde estará o seu novo modelo exportado

// ============================================================
// CONSTANTES DE NORMALIZAÇÃO (Z-SCORE)
// ============================================================
const float MEAN_DELTA_T = 15.65084995f;
const float STD_DELTA_T  = 6.99135624f;
const float MEAN_DELTA_U = -38.53349043f;
const float STD_DELTA_U  = 12.27171964f;
int main() {
    stdio_init_all();
    sleep_ms(2000); // Delay para abrir o monitor serial

    gpio_init(12); gpio_set_dir(12, GPIO_OUT); // LED azul (IDLE)
    gpio_init(11); gpio_set_dir(11, GPIO_OUT); // LED verde
    gpio_init(13); gpio_set_dir(13, GPIO_OUT); // LED vermelho (ANOMALIA)

    // 1. Inicialização do Hardware
    init_i2c_sensor(); // Deve inicializar I2C0 (Exaustão) e I2C1 (Ambiente)
    init_aht20();
    gpio_put(11, 1); // LED verde ON para indicar que o modelo foi carregado com sucesso+
    // 2. Inicialização da IA
    if (tflm_init() != 0) {
        printf("Erro ao carregar o modelo TFLite!\n");
        return -1;
    }
    
    printf("PredaGuard iniciado: Monitoramento Diferencial Ativo\n");

    int current_state = -1; // Estado estável atual exibido pelos LEDs
    int stable_count = 0;   // Contador de leituras consecutivas iguais
    const int STABLE_THRESHOLD = 3; // Número de leituras necessárias para aceitar uma mudança

    while (true) {
        // 3. Leitura dos Sensores
        SensorReadings data = get_sensor_readings();

        // 4. Cálculo do Diferencial (Física do Problema)
        float deltaT = data.aht_temp_1 - data.aht_temp_2;
        float deltaU = data.humidity_1 - data.humidity_2;


        // 5. Pré-processamento (Normalização idêntica ao Treino)
        float z_temp = (deltaT - MEAN_DELTA_T) / STD_DELTA_T;
        float z_umid = (deltaU - MEAN_DELTA_U) / STD_DELTA_U;
        // 6. Inferência
        float inputs[2] = {z_temp, z_umid};
        float outputs[3]; // [0]: IDLE, [1]: GAMING, [2]: ANOMALIA
        
        tflm_predict(inputs, outputs);

        // 7. Pós-processamento: Identifica a maior probabilidade
        int predicao = 0;
        float confiança = outputs[0];
        for (int i = 1; i < 3; i++) {
            if (outputs[i] > confiança) {
                confiança = outputs[i];
                predicao = i;
            }
        }

        // 8. Feedback via Serial
        printf("ΔT: %.2f°C | ΔU: %.2f%% | Trend ΔT: %.2f°C  -> ", deltaT, deltaU);
        if (predicao == 0) printf("Estado: IDLE (%.1f%%)\n", confiança*100);
        else if (predicao == 1) printf("Estado: GAMING (%.1f%%)\n", confiança*100);
        else if (predicao == 2) printf("⚠️ ALERTA: ANOMALIA/OBSTRUÇÃO! (%.1f%%)\n", confiança*100);

        // Hysteresis / debounce simples: exige N leituras consecutivas iguais antes de trocar o LED
        if (predicao != current_state) {
            stable_count++;
            if (stable_count >= STABLE_THRESHOLD) {
                current_state = predicao;
                stable_count = 0;
                // Atualiza TODOS os LEDs explicitamente para evitar estados residuais
                switch (current_state) {
                    case 0: // IDLE
                        gpio_put(12, 1); // azul ON
                        gpio_put(11, 0); // verde OFF
                        gpio_put(13, 0); // vermelho OFF
                        break;
                    case 1: // GAMING
                        gpio_put(12, 0); // azul OFF
                        gpio_put(11, 1); // verde ON
                        gpio_put(13, 0); // vermelho OFF
                        break;
                    case 2: // ANOMALIA
                        gpio_put(12, 0); // azul OFF
                        gpio_put(11, 0); // verde OFF
                        gpio_put(13, 1); // vermelho ON
                        break;
                }
            }
        } else {
            stable_count = 0; // mantém estado atual, zera contador
        }

        sleep_ms(500); // Monitoramento a cada 0.5 segundos
    }
    return 0;
}