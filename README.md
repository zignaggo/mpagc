# MPAGC - Guia Rápido de Instalação

## Arquivos do Projeto

- `index.html` - Interface web completa (formatada, para referência)
- `mpagc-esp8266.ino` - Código do ESP8266 (HTML já embutido no código)
- `README.md` - Documentação completa

## Instalação Rápida

### 1. Preparar Arduino IDE
- Instalar suporte ESP8266
- Instalar biblioteca **ArduinoJson** (versão 6.x)
- Instalar biblioteca **Adafruit_GFX** (para display OLED)
- Instalar biblioteca **Adafruit_SSD1306** (para display OLED)
- Instalar biblioteca **UrlEncode** (opcional, para codificação de URLs)

### 2. Configurar WiFi
Editar em `mpagc-esp8266.ino`:
```cpp
const char* WIFI_SSID = "SEU_WIFI";
const char* WIFI_PASSWORD = "SUA_SENHA";
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
ESP8266  →  Sensores/Componentes
------------------------
A0       →  MQ-2/MQ-5 (Sensor de gás)
D12      →  HX711 SCK (Balança - Clock)
D14      →  HX711 DT (Balança - Data)
D4       →  OLED SDA (Display I2C)
D5       →  OLED SCL (Display I2C)
D16      →  Buzzer (Alerta de vazamento)
3.3V     →  VCC dos sensores e display
GND      →  GND dos sensores e display
```

**Nota**: O display OLED usa comunicação I2C no endereço 0x3C.

## Calibração Básica

### Sensor de Gás
1. Ligar por 24-48h antes do uso (pré-aquecimento)
2. Ajustar limiar no código se necessário:
```cpp
int gasThreshold = 400;  // Aumentar/diminuir conforme necessário
```

### Balança
1. A função `readScale()` atualmente retorna um valor fixo (20 kg) - implementar leitura do HX711
2. Calibrar com peso conhecido
3. Ajustar fator de calibração na função `readScale()`

### Peso Inicial do Botijão
Configurar no código:
```cpp
float emptyGasWeight = 14.0;  // Peso do botijão vazio (kg)
float fullGasWeight = 13.0;   // Peso do botijão cheio (kg)
```
**Nota**: O cálculo do nível de gás usa a diferença entre `emptyGasWeight` e `fullGasWeight`.

## Uso da Interface

1. **Monitorar**: A interface atualiza automaticamente a cada 500ms
2. **Salvar Peso**: Use o botão "Salvar Peso Atual como Peso Inicial" para calibrar (funcionalidade em desenvolvimento)
3. **Alertas**: 
   - Vazamento detectado: Alerta visual e buzzer ativado
   - Nível baixo (< 20%): Alerta visual em vermelho
   - Nível médio (20-50%): Alerta visual em laranja

## Display OLED

O sistema inclui um display OLED 128x64 que exibe:
- Status de vazamento (SIM/NÃO)
- Nível de gás em porcentagem
- Barra de progresso visual
- Valores do sensor ADC e limiar configurado


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
  "emptyGasWeight": 14.0,
  "gasThreshold": 400
}
```

- `emptyGasWeight`: float - Peso do botijão vazio (kg)
- `gasThreshold`: int - Limiar configurado para detecção de vazamento

### POST /api/settings

Atualiza as configurações. Body JSON:

```json
{
  "emptyGasWeight": 14.0,
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

**Nota**: Esta funcionalidade está atualmente comentada no código e não está operacional. Quando implementada, salvará o peso atual lido pela balança como peso inicial.

Resposta esperada (quando implementada):
```json
{
  "success": true,
  "weight": 13.0
}
```

## Observações Importantes

1. **Memória**: O HTML está minificado e embutido diretamente no código como string PROGMEM
2. **WiFi**: Funciona apenas em redes 2.4GHz (ESP8266 não suporta 5GHz)
3. **Sensor de Gás**: Necessita pré-aquecimento de 24-48h para estabilizar
4. **Balança**: A função `readScale()` retorna valor fixo (20 kg) - implementar leitura do HX711 conforme seu hardware
5. **Display OLED**: Requer bibliotecas Adafruit_GFX e Adafruit_SSD1306, conectado via I2C (SDA=D4, SCL=D5)
6. **Buzzer**: Conectado no pino D16, ativado automaticamente quando vazamento é detectado
7. **Debug Mode**: Sistema possui modo debug ativado por padrão (`debugMode = true`) - desative para reduzir logs no Serial Monitor
8. **Cálculo de Nível**: O nível de gás é calculado usando `(peso_atual - emptyGasWeight) / (fullGasWeight - emptyGasWeight) * 100`
