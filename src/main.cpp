/*
 * Код стабильно работает на Digispark Attiny85.
 * Для настройки крайних точек необходимо сервы вывести в нейтральное положение,
 * Предположим, что у вас тумблер 1-2-3 положения. Чтобы перейти в режим передней блокировки необходимо, находясь в режиме задней блокировки (положение 2), 
 * отностительно быстро вернуть переключатель в положение 1 затем снова в 2.
 */

#include <Arduino.h>
#include <SimpleServo.h>
// #include <SoftSerial.h>
// #include <TinyPinChange.h>

#define RX_PIN 4                    // Пин входящего сигнала (канал блокировок дифов)
#define REAR_LOCKING_SERVO_PIN 0    // Пин Сервы задней блокировки
#define FRONT_LOCKING_SERVO_PIN 1   // Пин Сервы передней блокировки

#define REAR_LOCKING_SERVO_EPA_LOCK 110    // Крайняя левая точка задней сервы (Заблокирован)
#define REAR_LOCKING_SERVO_EPA_UNLOCK 0   // Крайняя правая точка задней сервы (Разблокирован)
#define FRONT_LOCKING_SERVO_EPA_LOCK 0        // Крайняя левая точка передней сервы (Разблокирован)
#define FRONT_LOCKING_SERVO_EPA_UNLOCK 110    // Крайняя левая точка передней сервы (Заблокирован)

SimpleServo REAR_SERVO;
SimpleServo FRONT_SERVO;
// SoftSerial swSerial(2, 3); //(RX, TX)

/*
 * Теущее состояние, единственный источник правды для обработки сигналов:
 * 0 - Не инициализировано
 * 1 - Разблокировать оба дифа
 * 2 - Блокировка заднего дифа
 * 3 - Блокировка переднего и заднего дифов
 * 4 - ⚡️⚡️⚡️ Переключение задней блокировки на переднюю ⚡️⚡️⚡️
 */
int STATE = 0;

int lastState = 0;      // Для проверки отсечения 
int virtualState = 0;   // Костыль для сохранения последнего *активного* состояния

unsigned long lastStateChange = 0;
unsigned long lastDebounceTime = 0;

unsigned long debounceDelay = 50;
unsigned long advModeDelay = 1000; // Чувствительность быстрого переключения 2->1->2 (4ый режим)

void setup() {
  // swSerial.begin(9600);
  // swSerial.txMode();
  // swSerial.println("Firmware started");
  REAR_SERVO.attach(REAR_LOCKING_SERVO_PIN);
  FRONT_SERVO.attach(FRONT_LOCKING_SERVO_PIN);
}

void sendUpdates() {
  switch (STATE) {
    case 1:
      REAR_SERVO.write(REAR_LOCKING_SERVO_EPA_UNLOCK);
      FRONT_SERVO.write(FRONT_LOCKING_SERVO_EPA_UNLOCK);
      // swSerial.println("Mode 1: Unlock Both Diffs");
      break;
    case 2:
      REAR_SERVO.write(REAR_LOCKING_SERVO_EPA_LOCK);
      FRONT_SERVO.write(FRONT_LOCKING_SERVO_EPA_UNLOCK);
      // swSerial.println("Mode 2: Locking Rear Diff");
      break;
    case 3:
      REAR_SERVO.write(REAR_LOCKING_SERVO_EPA_LOCK);
      FRONT_SERVO.write(FRONT_LOCKING_SERVO_EPA_LOCK);
      // swSerial.println("Mode 3: Locking Both Diffs");
      break;
    case 4:
      REAR_SERVO.write(REAR_LOCKING_SERVO_EPA_UNLOCK);
      FRONT_SERVO.write(FRONT_LOCKING_SERVO_EPA_LOCK);
      // swSerial.println("Mode 4: Locking Front Diff");
      break;
  }
}

void setState() {
  int RC_RX_DATA = pulseIn(RX_PIN, HIGH, 25000); // Чтение импульса с канала приемника

  delay(100);

  int normalizedDataValue = 0;

  if(RC_RX_DATA >= 1200 && RC_RX_DATA < 1500) normalizedDataValue = 1;
  else if(RC_RX_DATA >= 1500 && RC_RX_DATA < 1750) normalizedDataValue = 2;
  else if(RC_RX_DATA >= 1750) normalizedDataValue = 3;

  if (normalizedDataValue != lastState) {
    // Считанное состояние отличается от предыдущего
    // либо шум, либо мы действительно что-то меняем
    // в любом случае актуализируем дебаунс таймер
    lastDebounceTime = millis();
  }
  // swSerial.print("Rx Data: ");
  // swSerial.print(RC_RX_DATA);
  // swSerial.print(" ---- Software Mode: ");
  // swSerial.println(normalizedDataValue);

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Какое-то время не было никаких изменений, считаем что состояние стабильно и мы можем начать обработку
    if (normalizedDataValue != STATE && normalizedDataValue != virtualState) {
      bool fastFingerPass = lastStateChange != 0 && (millis() - lastStateChange) < advModeDelay;
      bool historyPass = virtualState == 1 && normalizedDataValue == 2;
      bool doubleCheckPass = STATE != 4;
      // swSerial.println("New state differs from current, check for new value");
      
      if (fastFingerPass && historyPass && doubleCheckPass) {
        // Быстро щелкнули туда-сюда, переходим в 4ый режим!
        STATE = 4;
        // swSerial.println("Fast finger detected! Locking Front Diff");
      } else {
        // Больше advModeDelay? Значит просто сохраняем текущее значение как новое состояние
        STATE = normalizedDataValue; 
        // swSerial.println("Just old plain state change, no revealed secret modes for now...");
      }

      lastStateChange = millis();
      sendUpdates();
      virtualState = normalizedDataValue;
    }
  }

  lastState = normalizedDataValue;
}

void loop() {
  setState();
}