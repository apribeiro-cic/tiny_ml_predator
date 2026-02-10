import serial
import time
import numpy as np
import sys
import os

# ==============================================================================
# CONFIGURAÇÕES GERAIS
# ==============================================================================
SERIAL_PORT = '/dev/ttyACM0'  # Linux/Mac. No Windows use 'COM3', 'COM4', etc.
BAUD_RATE = 115200
TIMEOUT_READ = 5  
ARQUIVO_DADOS_NPZ = "dados_validacao.npz"

def conectar_serial():
    """Tenta estabelecer conexão com a placa e retorna o objeto serial."""
    print(f"Tentando conectar em {SERIAL_PORT}...")
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
        time.sleep(2)  # Espera o reset da placa (DTR)
        ser.reset_input_buffer()
        print("✅ Conexão estabelecida!")
        return ser
    except serial.SerialException:
        print(f"❌ ERRO: Porta {SERIAL_PORT} não encontrada ou ocupada.")
        return None

def ler_resposta_placa(ser):
    """Lê a serial até encontrar a predição ou estourar o tempo."""
    start_time = time.time()
    pred_placa = -1
    
    while (time.time() - start_time) < TIMEOUT_READ:
        if ser.in_waiting:
            try:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if not line: continue
                
                if "Predição:" in line:
                    partes = line.split(":")
                    if len(partes) > 1:
                        pred_placa = int(partes[1].strip())
                        return pred_placa
            except ValueError:
                pass
    return -1

# ==============================================================================
# MODO 1: VALIDAÇÃO AUTOMÁTICA (NPZ)
# ==============================================================================
def modo_validacao_automatico():
    print("\n=== MODO 1: Validação Automática (Colab vs RP2040) ===")
    
    # 1. Carregar dados
    if not os.path.exists(ARQUIVO_DADOS_NPZ):
        print(f"❌ ERRO: O arquivo '{ARQUIVO_DADOS_NPZ}' não foi encontrado nesta pasta.")
        return

    print(f"Carregando '{ARQUIVO_DADOS_NPZ}'...")
    try:
        dados = np.load(ARQUIVO_DADOS_NPZ)
        imagens = dados['imagens']
        preds_colab = dados['preds_colab']
        labels_reais = dados['labels_reais']
        print(f"Sucesso! {len(imagens)} amostras carregadas.")
    except Exception as e:
        print(f"Erro ao ler arquivo: {e}")
        return

    # 2. Conectar
    ser = conectar_serial()
    if not ser: return

    print("\n--- INICIANDO COMPARAÇÃO ---")
    print(f"{'IDX':<5} | {'REAL':<6} | {'COLAB':<7} | {'PLACA':<7} | {'STATUS'}")
    print("-" * 60)

    acertos = 0
    concordancias = 0

    try:
        for i in range(len(imagens)):
            img_bytes = imagens[i].flatten().tobytes()
            label_real = labels_reais[i]
            pred_colab = preds_colab[i]

            # Envia
            ser.write(img_bytes)
            ser.flush()

            # Recebe
            pred_placa = ler_resposta_placa(ser)

            # Analisa
            status = ""
            str_placa = "--"
            
            if pred_placa != -1:
                str_placa = str(pred_placa)
                # Comparação Placa vs Colab
                if pred_placa == pred_colab:
                    status = "✅ MATCH"
                    concordancias += 1
                else:
                    status = "⚠️ DIV"
                
                # Verifica acerto real
                if pred_placa == label_real:
                    acertos += 1
            else:
                status = "❌ TIMEOUT"

            print(f"{i:<5} | {label_real:<6} | {pred_colab:<7} | {str_placa:<7} | {status}")
            time.sleep(0.2) # Pausa curta

        print("-" * 60)
        print(f"Acurácia da Placa (vs Real): {acertos}/{len(imagens)} ({(acertos/len(imagens))*100:.1f}%)")
        print(f"Fidelidade ao Colab: {concordancias}/{len(imagens)} amostras coincidiram.")

    except KeyboardInterrupt:
        print("\nInterrompido pelo usuário.")
    finally:
        ser.close()
        print("Porta serial fechada.")

# ==============================================================================
# MODO 2: TESTE MANUAL (MNIST AO VIVO)
# ==============================================================================
def modo_teste_manual():
    print("\n=== MODO 2: Teste Manual Interativo ===")
    print("Importando TensorFlow (isso pode demorar alguns segundos)...")
    
    # Importação local para não atrasar quem só quer usar o Modo 1
    import tensorflow as tf 
    
    print("Carregando dataset MNIST...")
    (_, _), (x_test, y_test) = tf.keras.datasets.mnist.load_data()
    print(f"Dataset pronto. {len(x_test)} imagens disponíveis.")

    ser = conectar_serial()
    if not ser: return

    print("\nInstruções: Digite o índice da imagem (0-9999) para testar ou 'q' para sair.")
    
    try:
        while True:
            user_input = input("\n> Digite o ID da imagem: ")
            if user_input.lower() == 'q':
                break
            
            try:
                idx = int(user_input)
                if idx < 0 or idx >= len(x_test):
                    print("Índice fora do intervalo (0-9999).")
                    continue
            except ValueError:
                print("Entrada inválida.")
                continue

            img = x_test[idx]
            label_real = y_test[idx]
            
            print(f"--- Testando Imagem #{idx} (Dígito Real: {label_real}) ---")

            # Prepara imagem (Flatten -> Bytes)
            img_bytes = img.flatten().tobytes()

            if len(img_bytes) != 784:
                print(f"Erro de tamanho: {len(img_bytes)} bytes.")
                continue

            print(f"Enviando para o RP2040...")
            ser.write(img_bytes)
            ser.flush()

            # Logica de leitura específica para ver mensagens de debug se houver
            print("Aguardando resposta...")
            pred_placa = ler_resposta_placa(ser)
            
            if pred_placa != -1:
                if pred_placa == label_real:
                    print(f"✅ SUCESSO! A placa respondeu: {pred_placa}")
                else:
                    print(f"❌ ERRO! A placa respondeu: {pred_placa} (Esperado: {label_real})")
            else:
                print("⚠️ Sem resposta (Timeout).")

    except KeyboardInterrupt:
        print("\nSaindo...")
    finally:
        ser.close()
        print("Porta serial fechada.")

# ==============================================================================
# MENU PRINCIPAL
# ==============================================================================
def main():
    while True:
        print("\n" + "="*40)
        print("   INTERFACE DE TESTE TINYML (RP2040)   ")
        print("="*40)
        print("1. Validação Automática (Arquivo .npz vs Placa)")
        print("2. Teste Manual (Escolher imagem do MNIST)")
        print("3. Sair")
        
        opcao = input("\nEscolha uma opção (1-3): ")

        if opcao == '1':
            modo_validacao_automatico()
        elif opcao == '2':
            modo_teste_manual()
        elif opcao == '3':
            print("Encerrando aplicação.")
            sys.exit()
        else:
            print("Opção inválida, tente novamente.")

if __name__ == "__main__":
    main()
