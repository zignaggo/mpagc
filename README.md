# MPAGC - Guia Rápido de Instalação

## Arquivos do Projeto

- `index.html` - Interface web completa (não formatada)
- `demo_standalone.html` - Demo interativa para testar sem hardware
- `esp8266_server.ino` - Código do ESP8266 (HTML já embutido)
- `README.md` - Documentação completa

## Instalação Rápida

### 1. Preparar Arduino IDE
- Instalar suporte ESP8266
- Instalar biblioteca **ArduinoJson** (versão 6.x)
- Instalar biblioteca **HX711** (se usar balança)

### 2. Configurar WiFi
Editar em `esp8266_server.ino`:
```cpp
const char* ssid = "SEU_WIFI";
const char* password = "SUA_SENHA";
```

### 3. Fazer Upload
- Conectar ESP8266 via USB
- Selecionar a placa e porta
- Clicar em Upload
- Abrir Serial Monitor (115200 baud) para ver o IP

### 4. Acessar Interface
Abrir navegador e acessar o IP mostrado no Serial Monitor


## Conexões dos Sensores

```
ESP8266  →  Sensores
------------------------
A0       →  MQ-2/MQ-5 (Sensor de gás)
D5       →  HX711 DT (Balança)
D6       →  HX711 SCK (Balança)
3.3V     →  VCC dos sensores
GND      →  GND dos sensores
```

## Calibração Básica

### Sensor de Gás
1. Ligar por 24-48h antes do uso (pré-aquecimento)
2. Ajustar limiar no código se necessário:
```cpp
int gasThreshold = 400;  // Aumentar/diminuir conforme necessário
```

### Balança
1. Implementar leitura do HX711 usando a biblioteca
2. Calibrar com peso conhecido
3. Ajustar fator de calibração

## Uso da Interface

1. **Configurar Peso Inicial**: Digite o peso total do botijão cheio (ex: 13kg para P13)
2. **Monitorar**: A interface atualiza automaticamente a cada 300ms
3. **Alertas**: Vazamento e nível baixo são alertados automaticamente


## Endpoints da API

Se quiser integrar com outros sistemas:

- `GET /api/status` - Retorna status atual do sistema
- `GET /api/settings` - Retorna configurações atuais
- `POST /api/settings` - Atualiza configurações
- `POST /api/weight` - Salva o peso atual como peso inicial

### GET /api/status

Retorna o status atual do sistema:

```json
{
  "gasLeak": false,
  "gasLevel": 75.5,
  "currentWeight": 11.2,
  "gasValue": 250,
  "gasThreshold": 400,
  "timestamp": 123456
}
```

- `gasLeak`: boolean - Indica se há vazamento detectado
- `gasLevel`: float - Porcentagem de gás restante (0-100)
- `currentWeight`: float - Peso atual lido pela balança (kg)
- `gasValue`: int - Valor lido do sensor ADC (0-1023)
- `gasThreshold`: int - Limiar configurado para detecção de vazamento
- `timestamp`: unsigned long - Timestamp em millis()

### GET /api/settings

Retorna as configurações atuais:

```json
{
  "initialWeight": 13.0,
  "gasThreshold": 400
}
```

### POST /api/settings

Atualiza as configurações. Body JSON:

```json
{
  "initialWeight": 13.0,
  "gasThreshold": 400
}
```

Resposta:
```json
{
  "success": true
}
```

### POST /api/weight

Salva o peso atual lido pela balança como peso inicial. Não requer body.

Resposta:
```json
{
  "success": true,
  "weight": 13.0
}
```

## Observações Importantes

1. **Memória**: O HTML está minificado e embutido diretamente no código
2. **WiFi**: Funciona apenas em redes 2.4GHz (ESP8266 não suporta 5GHz)
3. **Sensor de Gás**: Necessita pré-aquecimento de 24-48h para estabilizar
4. **Balança**: Implementação do HX711 deve ser adicionada conforme seu hardware
