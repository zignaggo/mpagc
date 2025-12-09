#include <HX711.h>
// RobTillaart/HX711
const int PIN_SCALE_DT = 12;   // GPIO12 (D6)
const int PIN_SCALE_SCK = 13;  // GPIO13 (D7)

HX711 scale;

float medida = 0;

void setup() {
  Serial.begin(115200);

  scale.begin(PIN_SCALE_DT, PIN_SCALE_SCK);
  scale.set_scale();

  delay(2000);
  scale.tare(); // ZERANDO A BALANÇA PARA DESCONSIDERAR A MASSA DA ESTRUTURA

  Serial.println("Balança Zerada");
}

void loop() {

  medida = scale.get_units(5); // SALVANDO NA VARIAVEL O VALOR DA MÉDIA DE 5 MEDIDAS
  Serial.println(medida, 3); // ENVIANDO PARA MONITOR SERIAL A MEDIDA COM 3 CASAS DECIMAIS

  scale.power_down();
  delay(1000);
  scale.power_up();
}
