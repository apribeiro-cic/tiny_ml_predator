#include "tflm_wrapper.h"

#include "modelo_predator.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
//#include "tensorflow/lite/version.h"

// Arena (ajuste se precisar)
// RP2040 tem 264KB RAM total. Usar 64KB para a arena deixa espaço para o resto do programa
static constexpr int kTensorArenaSize = 64 * 1024;
alignas(16) static uint8_t tensor_arena[kTensorArenaSize];

static const tflite::Model* model_ptr = nullptr;
static tflite::MicroInterpreter* interpreter_ptr = nullptr;
static TfLiteTensor* input_ptr = nullptr;
static TfLiteTensor* output_ptr = nullptr;

// Inicialização do TFLM
extern "C" int tflm_init(void) {
    printf("[TFLM] Iniciando inicialização...\n");
    
    model_ptr = tflite::GetModel(modelo_tflite);
    if (!model_ptr) {
        printf("[TFLM] ERRO: Falha ao obter modelo (GetModel retornou nullptr)\n");
        return 1;
    }
    printf("[TFLM] Modelo obtido com sucesso\n");

    if (model_ptr->version() != TFLITE_SCHEMA_VERSION) {
        printf("[TFLM] ERRO: Versão do schema mismatch (esperado %u, obtido %u)\n", 
               TFLITE_SCHEMA_VERSION, model_ptr->version());
        return 2;
    }

    // Resolver com mais operadores (suporta mais tipos de modelos)
    static tflite::MicroMutableOpResolver<16> resolver;
    resolver.AddConv2D();
    resolver.AddMean();
    resolver.AddFullyConnected();
    resolver.AddSoftmax();
    resolver.AddReshape();
    resolver.AddQuantize();
    resolver.AddDequantize();
    resolver.AddAdd();
    resolver.AddMul();
    resolver.AddPack();
    resolver.AddUnpack();
    resolver.AddSqueeze();
    resolver.AddExpandDims();
    resolver.AddMaxPool2D();
    resolver.AddAveragePool2D();
    
    printf("[TFLM] Resolver inicializado\n");

    // Interpreter estático (evita new em alguns cenários)
    static tflite::MicroInterpreter static_interpreter(
        model_ptr, resolver, tensor_arena, kTensorArenaSize
    );
    interpreter_ptr = &static_interpreter;

    TfLiteStatus alloc_status = interpreter_ptr->AllocateTensors();
    if (alloc_status != kTfLiteOk) {
        printf("[TFLM] ERRO: AllocateTensors falhou (status=%d)\n", (int)alloc_status);
        printf("[TFLM] Arena disponível: %d bytes, arena usada: %d bytes\n", 
               kTensorArenaSize, (int)interpreter_ptr->arena_used_bytes());
        return 3;
    }
    printf("[TFLM] Tensores alocados. Arena usada: %d bytes\n", 
           (int)interpreter_ptr->arena_used_bytes());

    input_ptr  = interpreter_ptr->input(0);
    output_ptr = interpreter_ptr->output(0);
    if (!input_ptr || !output_ptr) {
        printf("[TFLM] ERRO: input_ptr=%p, output_ptr=%p\n", input_ptr, output_ptr);
        return 4;
    }

    printf("[TFLM] Input tensor: type=%d (int8=%d, float32=%d), bytes=%d\n", 
           input_ptr->type, kTfLiteInt8, kTfLiteFloat32, input_ptr->bytes);
    printf("[TFLM] Output tensor: type=%d (int8=%d, float32=%d), bytes=%d\n",
           output_ptr->type, kTfLiteInt8, kTfLiteFloat32, output_ptr->bytes);

    // PERMITIR float32 além de int8
    if (input_ptr->type != kTfLiteInt8 && input_ptr->type != kTfLiteFloat32) {
        printf("[TFLM] AVISO: Input tipo inesperado! (esperado int8 ou float32, obtido %d)\n", 
               input_ptr->type);
        return 5;
    }
    if (output_ptr->type != kTfLiteInt8 && output_ptr->type != kTfLiteFloat32) {
        printf("[TFLM] AVISO: Output tipo inesperado! (esperado int8 ou float32, obtido %d)\n", 
               output_ptr->type);
        return 6;
    }

    printf("[TFLM] Inicialização concluída com sucesso!\n");
    return 0;
}

extern "C" int8_t* tflm_input_ptr(int* nbytes) {
    if (!input_ptr) return nullptr;
    if (nbytes) *nbytes = input_ptr->bytes;
    return input_ptr->data.int8;
}

extern "C" int8_t* tflm_output_ptr(int* nbytes) {
    if (!output_ptr) return nullptr;
    if (nbytes) *nbytes = output_ptr->bytes;
    return output_ptr->data.int8;
}

extern "C" float tflm_input_scale(void) {
    return input_ptr ? input_ptr->params.scale : 0.0f;
}
extern "C" int tflm_input_zero_point(void) {
    return input_ptr ? input_ptr->params.zero_point : 0;
}
extern "C" float tflm_output_scale(void) {
    return output_ptr ? output_ptr->params.scale : 0.0f;
}
extern "C" int tflm_output_zero_point(void) {
    return output_ptr ? output_ptr->params.zero_point : 0;
}

extern "C" int tflm_invoke(void) {
    if (!interpreter_ptr) return 1;
    return (interpreter_ptr->Invoke() == kTfLiteOk) ? 0 : 2;
}

extern "C" int tflm_arena_used_bytes(void) {
    if (!interpreter_ptr) return -1;
    return (int)interpreter_ptr->arena_used_bytes();
}
