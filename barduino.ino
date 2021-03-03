#include <PulseSensorPlayground.h>

//----- INITIALISATION DES VARIABLES -----

#define USE_ARDUINO_INTERRUPTS true    // Configure des interrupts de bas niveau pour plus de précision lors du calcul de BPM
const int PulseWire = 0;
const int LED13 = 13;
int Threshold = 550;           // Sensibilité de détection d'un battement. Défaut = 550
int BPM_final;

#define PIN_VITESSE 7
short etat = 0;
const String etatString[3] = {"ATTENTE", "LECTURE DE BPM ET CALCUL DES DOSES", "DISTRIBUTION"};
bool changementEtat = false;

#define TEMPS_LECTURE 1000
unsigned long tempsDepart = 0;
int totalTemps;

PulseSensorPlayground pulseSensor;

struct Moteur {
  String nom;
  int BPMmin, BPMmax, pin, ledPin, temps, tempsMin, tempsMax;
  bool working;
};
Moteur m1 = { .nom = "Rhum blanc", .BPMmin = 50, .BPMmax = 100, .pin = 2, .ledPin = 12, .temps = 0, .tempsMin = 20, .tempsMax = 0, .working = false};
Moteur m2 = { .nom = "Limonade", .BPMmin = 75, .BPMmax = 125, .pin = 3, .ledPin = 11, .temps = 0, .tempsMin = 0, .tempsMax = 50, .working = false};
Moteur m3 = { .nom = "Rhum brun", .BPMmin = 100, .BPMmax = 150, .pin = 4, .ledPin = 10, .temps = 0, .tempsMin = 20, .tempsMax = 0, .working = false};
Moteur m4 = { .nom = "Soda", .BPMmin = 125, .BPMmax = 175, .pin = 5, .ledPin = 9, .temps = 0, .tempsMin = 0, .tempsMax = 50, .working = false};
#define NB_MOTEURS 4
struct Moteur arrayMoteurs[NB_MOTEURS] = {m1, m2, m3, m4};


//----- SETUP -----

void setup() {
  pinMode(m1.pin , OUTPUT);
  pinMode(m2.pin , OUTPUT);
  pinMode(m3.pin , OUTPUT);
  pinMode(m4.pin , OUTPUT);
  pinMode(PIN_VITESSE, OUTPUT);
  pinMode(m1.ledPin, OUTPUT);
  pinMode(m2.ledPin, OUTPUT);
  pinMode(m3.ledPin, OUTPUT);
  pinMode(m4.ledPin, OUTPUT);

  // Initialisation du capteur cardiaque
  pulseSensor.analogInput(PulseWire);
  pulseSensor.blinkOnPulse(LED13);       //auto-magically blink Arduino's LED with heartbeat.
  pulseSensor.setThreshold(Threshold);
  // Double-check the "pulseSensor" object was created and "began" seeing a signal.
  if (pulseSensor.begin()) {
    Serial.println("Capteur de battement initialisé");
  }

  Serial.begin(9600);
}

//----- PROGRAMME PRINCIPAL -----

void loop() {

  affichageEtat();

  switch (etat) {
    case 0:
      if (pulseSensor.getBeatsPerMinute()) {
        etat = 1;
        changementEtat = true;
        tempsDepart = millis();
      };
      break;
    case 1:
      checkPulse();
      if (millis() - tempsDepart >= TEMPS_LECTURE) {
        lectureEtCalcul(checkPulse()); // Lecture unique. Retourne le nombre de BPM
        etat = 2;
        changementEtat = true;
        tempsDepart = millis();
      }
      break;
    case 2:
      if (distribution()) {
        etat = 0;
        changementEtat = true;
        reinitialisation();
      }
      break;
    default:
      Serial.println("Erreur switch etat");
      break;
  }
}

void affichageEtat() {
  if (changementEtat) {
    changementEtat = false;
    Serial.println(etatString[etat] + "...");
  }
}

int checkPulse() {
  int myBPM = pulseSensor.getBeatsPerMinute();
  if (pulseSensor.sawStartOfBeat()) {         
    Serial.print("BPM actuel : "); 
    Serial.println(myBPM);
    return myBPM;
  }
}

void lectureEtCalcul(int myBPM) {
  BPM_final = myBPM;
  Serial.print("Terminé! Votre poul est de ");
  Serial.print(BPM_final);
  Serial.println(" battements par minute ♥");
  calculTemps();
}

void calculTemps() {
  for (int i = 0; i < NB_MOTEURS; i++) {
    struct Moteur mTemp = arrayMoteurs[i];
    int temps;
    if (BPM_final >= mTemp.BPMmin && BPM_final <= mTemp.BPMmax) {
      temps = map(BPM_final, mTemp.BPMmin, mTemp.BPMmax, mTemp.tempsMin, mTemp.tempsMax); // Convertit le nombre de BPM en secondes d'opération pour le moteur
    } else {
      temps = 0;
    }
    arrayMoteurs[i].temps = temps * 1000; // *1000 car on veut le temps en millisecondes
    totalTemps += temps;
    Serial.print("Le ");
    Serial.print(mTemp.nom);
    Serial.print(" coulera pendant : ");
    Serial.print(temps);
    Serial.println(" seconde(s)");
  }
}

bool distribution() {
  bool distribution_done = false;
  short nbMoteursTermines = 0;
  for (int i = 0; i < NB_MOTEURS; i++) {
    struct Moteur mTemp = arrayMoteurs[i];
    if (millis() - tempsDepart < mTemp.temps) {
      digitalWrite(mTemp.ledPin, HIGH);
      mTemp.working = true;
    } else {
      digitalWrite(mTemp.ledPin, LOW);
      mTemp.working = false;
      nbMoteursTermines++;
    }
  }
  if (nbMoteursTermines == NB_MOTEURS) {
    distribution_done = true;
    Serial.println("La concentration de votre liquide est de ");
    for (int i = 0; i < NB_MOTEURS; i++) {
      struct Moteur mTemp = arrayMoteurs[i];
      Serial.print(mTemp.nom + ": ");
      Serial.print((float)(mTemp.temps / totalTemps));
      Serial.print("%   |   ");
    }
    Serial.println("\n Bonne dégustation! :)");
  }
  return distribution_done;
}

void reinitialisation() {
  totalTemps = 0;
  for (int i = 0; i < NB_MOTEURS; i++) {
    struct Moteur mTemp = arrayMoteurs[i];
    mTemp.temps = 0;
  }
}
