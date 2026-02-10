/*
 * cnn_mnist.c - Firmware para RP2040 (TinyML)
 * Recebe imagem 28x28 via USB Serial, classifica e retorna o resultado.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "tflm_wrapper.h"
#include "modelo_predator.h"
#include "lib/sensors/sensors.h"
#include "lib/buttons/buttons.h"

// Definições de tamanho do modelo Predator
#define INPUT_FEATURES 2
#define IMAGE_SIZE (INPUT_FEATURES)
#define NUM_CLASSES 3

// Valores de normalização gerados pelo seu treino (StandardScaler)
const float MEAN_TEMP = 47.55531255f;
const float STD_TEMP  = 4.63953885f;

const float MEAN_UMID = 37.40246446f;
const float STD_UMID  = 7.42324274f;

// Função auxiliar de quantização (Float -> Int8)
int8_t quantize_f32_to_i8(float x, float scale, int zero_point) {
    int32_t q = (int32_t)(x / scale + zero_point);
    if (q < -128) q = -128;
    if (q > 127) q = 127;
    return (int8_t)q;
}

// Função auxiliar para encontrar a classe com maior probabilidade
int argmax_i8(int8_t *data, int count) {
    int max_idx = 0;
    int8_t max_val = data[0];
    for (int i = 1; i < count; i++) {
        if (data[i] > max_val) {
            max_val = data[i];
            max_idx = i;
        }
    }
    return max_idx;
}

int main() {
    // 1. Inicialização do Hardware
    stdio_init_all();
    init_i2c_sensor();            
    init_aht20();
    init_buttons();
    
    gpio_init(12); gpio_set_dir(12, GPIO_OUT); // LED azul (IDLE)
    gpio_init(11); gpio_set_dir(11, GPIO_OUT); // LED verde
    gpio_init(13); gpio_set_dir(13, GPIO_OUT); // LED vermelho (ANOMALIA)

    sleep_ms(2000); // Aguarda conexão USB estabilizar
    
    printf("\n=== MNIST TinyML no RP2040 (USB Serial) ===\n");

    // 2. Inicialização do Modelo TensorFlow Lite Micro
    // O wrapper deve lidar com a alocação da Arena
    if (tflm_init() != 0) {
        printf("Erro: Falha ao inicializar TFLM!\n");
        while (1) tight_loop_contents();
    }
    gpio_put(12, 1); gpio_put(11, 0); gpio_put(13, 0);
    // Obter ponteiros e tamanhos dos tensores de entrada e saída
    int input_bytes = 0;
    int output_bytes = 0;
    int8_t* in_data = tflm_input_ptr(&input_bytes);
    int8_t* out_data = tflm_output_ptr(&output_bytes);

    gpio_put(12, 0); gpio_put(11, 1); gpio_put(13, 0);

    if (in_data == NULL || out_data == NULL || input_bytes <= 0 || output_bytes <= 0) {
        printf("Erro: Tensores invalidos.\n");
        while (1) tight_loop_contents();
    }

    if (input_bytes < IMAGE_SIZE) {
        printf("Erro: Tamanho do tensor de entrada inesperado (%d bytes).\n", input_bytes);
        while (1) tight_loop_contents();
    }

    // Parâmetros de quantização
    float in_scale = tflm_input_scale();
    int in_zp = tflm_input_zero_point();
    
    float out_scale = tflm_output_scale();
    int out_zp = tflm_output_zero_point();

    printf("Modelo carregado. Aguardando %d bytes...\n", IMAGE_SIZE);

    gpio_put(12, 0); gpio_put(11, 0); gpio_put(13, 1);

    // 3. Loop Principal
    while (true) {
        // 1. Ler o sensor
        // Função do driver do sensor
        SensorReadings readings = get_sensor_readings();

        readings.aht_temp = readings.aht_temp + (float)offset;

        if (lock_humidity) {
            readings.humidity = MEAN_UMID; // Força umidade para a média usada no treino se o lock estiver ativo
        }

        // Debug da leitura
        printf("\n--- Nova Leitura ---\n");
        printf("Sensor -> Temp: %.2f | Umid: %.2f\n", readings.aht_temp, readings.humidity);

        // 2. Pré-processamento (Normalização Matemática)
        float input_normalizado[2];
        input_normalizado[0] = (readings.aht_temp - MEAN_TEMP) / STD_TEMP;
        input_normalizado[1] = (readings.humidity - MEAN_UMID) / STD_UMID;

        // Mostra o dado normalizado 
        printf("Input Norm -> T: %.4f | U: %.4f\n", input_normalizado[0], input_normalizado[1]);

        // 3. Preparar a Entrada do Modelo
        // Convertendo o ponteiro void/int8 para float* para escrever corretamente
        float* input_buffer_as_float = (float*)in_data;
        
        input_buffer_as_float[0] = input_normalizado[0];
        input_buffer_as_float[1] = input_normalizado[1];

        // 4. Executar Inferência local no RP2040
        if (tflm_invoke() != 0) {
            printf("Erro: falha na inferência TFLM\n");
            sleep_ms(1000);
            continue;
        }

        // 5. Ler a Saída
        
        float output_probabilidades[NUM_CLASSES];
        
        // Verificação rápida: Se type=1 (float), não precisa desquantizar
        if (out_scale == 0.0f) {
             float* output_buffer_as_float = (float*)out_data;
             for (int i = 0; i < NUM_CLASSES; i++) {
                 output_probabilidades[i] = output_buffer_as_float[i];
             }
        } else {
            // Caminho quantizado (caso seu modelo tenha saída int8)
            for (int i = 0; i < NUM_CLASSES; i++) {
                output_probabilidades[i] = ((float)(out_data[i] - out_zp)) * out_scale;
            }
        }

        // 6. Decisão e Alerta (Demonstração prática)
        int classe_detectada = 0;
        float maior_prob = output_probabilidades[0];

        printf("Predicoes: [ ");
        for (int i = 0; i < NUM_CLASSES; i++) {
            printf("%.2f ", output_probabilidades[i]);
            if (output_probabilidades[i] > maior_prob) {
                maior_prob = output_probabilidades[i];
                classe_detectada = i;
            }
        }
        printf("]\n");

        // Nomeando para facilitar debug
        const char* nomes_classes[] = {"IDLE (Azul)", "GAMING (Verde)", "ANOMALIA (Vermelho)"};
        printf("Resultado: %s (Indice: %d)\n", nomes_classes[classe_detectada], classe_detectada);

        // 7. Saídas do Sistema (Alertas Visuais)
        if (classe_detectada == 0) { // IDLE
            gpio_put(12, 1); gpio_put(11, 0); gpio_put(13, 0);
        } else if (classe_detectada == 1) { // GAMING
            gpio_put(12, 0); gpio_put(11, 1); gpio_put(13, 0);
        } else { // OBSTRUÇÃO
            gpio_put(12, 0); gpio_put(11, 0); gpio_put(13, 1);
        }

        sleep_ms(1000);
    }

    return 0;
}