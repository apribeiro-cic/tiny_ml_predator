
# PredaGuard: Monitoramento Diferencial de Anomalias em Computadores

## 📋 Descrição

**PredaGuard** é um sistema embarcado de detecção de anomalias (obstruções, superaquecimento) em computadores usando **Machine Learning** em tempo real. O projeto utiliza sensores de temperatura e umidade para calcular diferenciais entre dois pontos (exaustão vs. ambiente) e classifica o estado do sistema em três categorias:

- **IDLE**: Sistema em repouso
- **GAMING**: Sistema sob carga normal
- **ANOMALIA**: Possível obstrução ou falha térmica

## 🎯 Características

- ✅ Inferência de IA executada em **Raspberry Pi Pico** (microcontrolador com 264 KB de RAM)
- ✅ Modelo TensorFlow Lite otimizado para edge computing
- ✅ Física baseada em diferenciais (ΔT e ΔU) para robustez
- ✅ Normalização Z-score idêntica ao treino
- ✅ Feedback visual com LEDs (azul, verde, vermelho)
- ✅ Debounce/Hysteresis para evitar oscilações
- ✅ Monitoramento a cada 500ms

## 📁 Estrutura do Projeto

```
tiny_ml_predator/
├── main.c                          # Código principal do Pico
├── tflm_wrapper.h/c               # Interface com TensorFlow Lite Micro
├── modelo_predator.h              # Modelo exportado (gerado)
├── lib/
│   └── sensors/
│       └── sensors.h/c            # Drivers de sensores (AHT20, DHT22)
├── Notebooks/
│   ├── Pré_Processamento_delta.ipynb    # ETL e normalização
│   ├── Treinamento_delta.ipynb          # Treino do modelo
│   └── Data/
│       ├── gaming_differential_bruto.csv
│       ├── idle_differential_bruto.csv
│       ├── obstrucao_differential_bruto.csv
│       └── dataset_pronto_treino.csv
└── CMakeLists.txt                 # Build para Pico SDK
```

## 🔧 Hardware

| Componente | Modelo | Pino |
|-----------|--------|------|
| Microcontrolador | Raspberry Pi Pico | - |
| Sensor Temp/Umidade 1 (Exaustão) | AHT20 | I2C0 |
| Sensor Temp/Umidade 2 (Ambiente) | DHT22 | I2C1 |
| LED IDLE | - | GPIO 12 (Azul) |
| LED GAMING | - | GPIO 11 (Verde) |
| LED ANOMALIA | - | GPIO 13 (Vermelho) |

## 🚀 Como Usar

### 1. Pré-processamento dos Dados
```bash
jupyter notebook Notebooks/Pré_Processamento_delta.ipynb
```
Normaliza os dados brutos com Z-score e gera `dataset_pronto_treino.csv`.

### 2. Treinamento do Modelo
```bash
jupyter notebook Notebooks/Treinamento_delta.ipynb
```
Treina rede neural e exporta para `modelo_predator.h` (TFLite quantizado).

### 3. Compilação para Pico
```bash
mkdir build && cd build
cmake ..
make
```

### 4. Deploy
```bash
cp main.uf2 /path/to/pico
```

## 📊 Pipeline de AI

```
Sensores → Cálculo Diferencial → Normalização → Inferência TFLite → Classificação → LEDs
```

**Constantes de Normalização:**
- ΔT: μ=15.65°C, σ=6.99°C
- ΔU: μ=-38.53%, σ=12.27%

## 🎓 Referências

- [TensorFlow Lite Micro](https://www.tensorflow.org/lite/microcontrollers)
- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
- [AHT20 Datasheet](https://asairsensors.com/aht20/)
