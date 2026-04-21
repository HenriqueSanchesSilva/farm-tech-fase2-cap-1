#include <DHTesp.h>

// -------------------- PINOS --------------------
#define PIN_BOTAO_N   12
#define PIN_BOTAO_P   13
#define PIN_BOTAO_K   14
#define PIN_LDR       34
#define PIN_DHT       15
#define PIN_RELE      26

DHTesp dht;

// -------------------- ESTADO GLOBAL --------------------
int culturaSelecionada = 0;
bool bombaLigada = false;
bool chuvaExterna = false;

// -------------------- PARÂMETROS POR CULTURA --------------------
struct ParametrosCultura {
  const char* nome;
  float phMin;
  float phMax;
  float umidadeMin;
  bool precisaN;
  bool precisaP;
  bool precisaK;
};

ParametrosCultura culturas[2] = {
  { "Milho", 5.5, 7.0, 60.0, true,  true,  true  },
  { "Cafe",  5.0, 6.5, 65.0, true,  false, false }
};

// -------------------- PROTÓTIPOS --------------------
void lerComandosSerial();
float mapearLDRpH(int ldrBruto);
bool verificarIrrigacao(ParametrosCultura cultura, bool N, bool P, bool K, float pH, float umidade, bool chuvaPrevista);
void exibirParametros(int cultura);
void exibirStatus(ParametrosCultura cultura, bool N, bool P, bool K, int ldrBruto, float pH, float umidade, float temperatura, bool bomba, bool chuvaPrevista);

// -------------------- SETUP --------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIN_BOTAO_N, INPUT_PULLUP);
  pinMode(PIN_BOTAO_P, INPUT_PULLUP);
  pinMode(PIN_BOTAO_K, INPUT_PULLUP);

  pinMode(PIN_RELE, OUTPUT);
  digitalWrite(PIN_RELE, LOW);

  dht.setup(PIN_DHT, DHTesp::DHT22);

  Serial.println("======================================");
  Serial.println(" FarmTech Solutions - Fase 2");
  Serial.println(" Sistema de Irrigacao Inteligente");
  Serial.println("======================================");
  Serial.println("Selecione a cultura no Serial Monitor:");
  Serial.println("  1 -> Milho");
  Serial.println("  2 -> Cafe");
  Serial.println("--------------------------------------");
  Serial.println("Comandos opcionais:");
  Serial.println("  C -> chuva prevista ON");
  Serial.println("  S -> chuva prevista OFF");
  Serial.println("--------------------------------------");
}

// -------------------- LOOP --------------------
void loop() {
  lerComandosSerial();

  if (culturaSelecionada == 0) {
    delay(400);
    return;
  }

  bool N = !digitalRead(PIN_BOTAO_N);
  bool P = !digitalRead(PIN_BOTAO_P);
  bool K = !digitalRead(PIN_BOTAO_K);

  int ldrBruto = analogRead(PIN_LDR);
  float pH = mapearLDRpH(ldrBruto);

  TempAndHumidity dados = dht.getTempAndHumidity();
  float umidade = dados.humidity;
  float temperatura = dados.temperature;

  if (isnan(umidade) || isnan(temperatura)) {
    Serial.println("[ERRO] Falha na leitura do DHT22.");
    delay(2000);
    return;
  }

  ParametrosCultura cultura = culturas[culturaSelecionada - 1];

  bool decisaoBomba = verificarIrrigacao(cultura, N, P, K, pH, umidade, chuvaExterna);

  if (decisaoBomba != bombaLigada) {
    bombaLigada = decisaoBomba;
    digitalWrite(PIN_RELE, bombaLigada ? HIGH : LOW);
  }

  exibirStatus(cultura, N, P, K, ldrBruto, pH, umidade, temperatura, bombaLigada, chuvaExterna);

  delay(3000);
}

// -------------------- FUNÇÕES --------------------
void lerComandosSerial() {
  while (Serial.available() > 0) {
    char entrada = Serial.read();

    if (entrada == '\n' || entrada == '\r') continue;

    Serial.print("[DEBUG] Recebi: ");
    Serial.println(entrada);

    if (entrada == '1') {
      culturaSelecionada = 1;
      Serial.println("[INFO] Cultura selecionada: MILHO");
      exibirParametros(culturaSelecionada);

    } else if (entrada == '2') {
      culturaSelecionada = 2;
      Serial.println("[INFO] Cultura selecionada: CAFE");
      exibirParametros(culturaSelecionada);

    } else if (entrada == 'C' || entrada == 'c') {
      chuvaExterna = true;
      Serial.println("[INFO] Chuva prevista ativada.");

    } else if (entrada == 'S' || entrada == 's') {
      chuvaExterna = false;
      Serial.println("[INFO] Chuva prevista desativada.");
    }
  }
}

float mapearLDRpH(int ldrBruto) {
  float pH = (ldrBruto / 4095.0) * 14.0;
  if (pH < 0.0) pH = 0.0;
  if (pH > 14.0) pH = 14.0;
  return pH;
}

bool verificarIrrigacao(ParametrosCultura cultura,
                        bool N, bool P, bool K,
                        float pH, float umidade,
                        bool chuvaPrevista) {
  bool phOk = (pH >= cultura.phMin && pH <= cultura.phMax);
  bool umidadeBaixa = (umidade < cultura.umidadeMin);

  bool nutrientesOk = true;
  if (cultura.precisaN && !N) nutrientesOk = false;
  if (cultura.precisaP && !P) nutrientesOk = false;
  if (cultura.precisaK && !K) nutrientesOk = false;

  if (chuvaPrevista) return false;

  return (umidadeBaixa && phOk && nutrientesOk);
}

void exibirParametros(int cultura) {
  ParametrosCultura c = culturas[cultura - 1];

  Serial.println("--------------------------------------");
  Serial.print("Parametros ideais para: ");
  Serial.println(c.nome);

  Serial.print("pH ideal: ");
  Serial.print(c.phMin, 1);
  Serial.print(" a ");
  Serial.println(c.phMax, 1);

  Serial.print("Umidade minima: ");
  Serial.print(c.umidadeMin, 1);
  Serial.println("%");

  Serial.print("Nutrientes criticos: ");
  if (c.precisaN) Serial.print("N ");
  if (c.precisaP) Serial.print("P ");
  if (c.precisaK) Serial.print("K ");
  Serial.println();

  Serial.println("--------------------------------------");
}

void exibirStatus(ParametrosCultura cultura,
                  bool N, bool P, bool K,
                  int ldrBruto, float pH,
                  float umidade, float temperatura,
                  bool bomba, bool chuvaPrevista) {
  Serial.println("======================================");
  Serial.print("Cultura: ");
  Serial.println(cultura.nome);

  Serial.println("--- Sensores ---");
  Serial.print("N: ");
  Serial.print(N ? "PRESENTE" : "AUSENTE");
  Serial.print(" | P: ");
  Serial.print(P ? "PRESENTE" : "AUSENTE");
  Serial.print(" | K: ");
  Serial.println(K ? "PRESENTE" : "AUSENTE");

  Serial.print("LDR bruto: ");
  Serial.print(ldrBruto);
  Serial.print(" -> pH simulado: ");
  Serial.println(pH, 1);

  Serial.print("Umidade: ");
  Serial.print(umidade, 1);
  Serial.println("%");

  Serial.print("Temperatura: ");
  Serial.print(temperatura, 1);
  Serial.println(" C");

  Serial.print("Chuva externa: ");
  Serial.println(chuvaPrevista ? "SIM" : "NAO");

  Serial.println("--- Decisao ---");
  Serial.print("Bomba d'agua: ");

  if (bomba) {
    Serial.println("LIGADA [IRRIGANDO]");
  } else {
    bool phOk = (pH >= cultura.phMin && pH <= cultura.phMax);
    bool umidadeBaixa = (umidade < cultura.umidadeMin);
    bool nutrientesOk = true;

    if (cultura.precisaN && !N) nutrientesOk = false;
    if (cultura.precisaP && !P) nutrientesOk = false;
    if (cultura.precisaK && !K) nutrientesOk = false;

    Serial.print("DESLIGADA | Motivo: ");
    if (chuvaPrevista) Serial.print("chuva prevista ");
    if (!umidadeBaixa) Serial.print("umidade adequada ");
    if (!phOk) Serial.print("pH fora da faixa ");
    if (!nutrientesOk) Serial.print("nutrientes insuficientes ");
    Serial.println();
  }

  Serial.println("======================================");
}