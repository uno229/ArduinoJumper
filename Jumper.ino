// Библиотеки для работы с жидко-кристаллическим индикатором (ЖКИ)
#include <Wire.h>
#include "LiquidCrystal_I2C.h"

// Номера пользовательским символов
#define HEAD 0 // Верхняя часть Джампера
#define BODY 1 // Нижняя часть Джампера
#define BIG_LEAVES 2 // Листья больших деревьев
#define BIG_WOOD 3 // Стволы больших деревьев
#define SMALL_LEAVES 4 // Листья кустов
#define BIRD 5 // Птицы

// Пины для подключения
#define BUTTON 2 // Кнопка
#define BUZZER 3 // Буззер
#define BUZZER_POWER 4 // Питание буззера, чтобы не использовать бредбоард

// Создание ЖКИ
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Пользовательские символы
byte head[] =         {0b00000, 0b01110, 0b01110, 0b01110, 0b00100, 0b01110, 0b10101, 0b10101};
byte body[] =         {0b10101, 0b00100, 0b01110, 0b01010, 0b01010, 0b01010, 0b11011, 0b00000};
byte bigLeaves [] =   {0b00000, 0b00000, 0b00100, 0b01110, 0b11111, 0b11111, 0b11111, 0b11111};
byte bigWood[] =      {0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000};
byte smallLeaves[] =  {0b00100, 0b01110, 0b11111, 0b11111, 0b11111, 0b00100, 0b00100, 0b00000};
byte bird [] =        {0b00000, 0b00000, 0b00101, 0b01111, 0b11111, 0b00100, 0b00000, 0b00000};

// Координаты расположения Джампера
int manX = 3;
int manY = 2;

long jumpTime; // Время совершения прыжка
bool isJump = false; // Признак прыжка
int jumpSpeed = 1500; // Время висения в воздухе

long startTime; // Время начала игры для отсчета длительности игры

// Описание деревьев
typedef struct
{
  int treeX; // Координата дерева
  long treeTime; // Время появления дерева
  int treeSpeed;// Скорость движения дерева, мс (время между перемещениями)
  long treeMoveTime; // Время последнего перемещения дерева
  bool treeVisible; // Признак видимости дерева
  bool treeSize; // Размер дерева (false - куст, true - дерево)
} Tree;

// Массив деревьев (одновременно на экране может быть три)
Tree trees[3];

// Описание птицы
int birdX; // Координата птицы
long birdTime;// Время появления птицы
int birdSpeed;// Скорость движения птицы, мс (время между перемещениями)
long birdMoveTime;// Время последнего щения птицы
bool birdVisible; // Признак видимости птицы

bool updateScreen = true;  // Признак обновления экрана ЖКИ
bool gameOver = false;  // Признак конца игры

void setup() {
  pinMode(BUTTON, INPUT_PULLUP);  // Кнопка - цифровой вход с подтяжкой
  pinMode(BUZZER, OUTPUT);  // Буззер - выходное устройство
  pinMode(BUZZER_POWER, OUTPUT); // Питание буззера
  digitalWrite(BUZZER, HIGH); // Выключение буззера (буззер работает от уровня LOW)
  digitalWrite(BUZZER_POWER, HIGH); // Включение питания буззера

  randomSeed(analogRead(A0)); // Запуск генератора случайных чисел

  // Запуск ЖКИ и включение подстветки
  lcd.init();
  lcd.backlight();

  // Создание пользовательских символов в ЖКИ
  lcd.createChar(HEAD, head);
  lcd.createChar(BODY, body);
  lcd.createChar(BIG_LEAVES, bigLeaves);
  lcd.createChar(BIG_WOOD, bigWood);
  lcd.createChar(SMALL_LEAVES, smallLeaves);
  lcd.createChar(BIRD, bird);

  // Очистка ЖКИ
  lcd.clear();
}

// Основной цикл игры
void loop()
{
  // Начальный экран
  drawStartScreen();

  // Создание новой игры
  createGame();
  gameOver = false;

  // Пока игра не закончилась
  while (!gameOver)
  {
    moveMan(); // Прыжок Джампера при необходимости
    moveTrees(); // Перемещение деревьев при необходимости
    moveBird(); // Перемещение птицы при необходимости

    drawGame(); // Перерисовка экрана ЖКИ при необходимости
    updateScreen = false; // Экран перерисован
    gameOver = checkGameOver(); // Проверка конца игры (Джампер врезался в дерево или птицу)
  }
  drawGameOver(); // Экран конца игры
}

// Начальный экран
void drawStartScreen()
{
  // Вывод текста начального экрана
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Game Jumper, v.1.0");
  lcd.setCursor(5, 1);
  lcd.print("Created by");
  lcd.setCursor(4 ,2);
  lcd.print("UNO & Yarik.");
  lcd.setCursor(4, 3);
  lcd.print("PRESS START!");

  while (digitalRead(BUTTON) == HIGH); // Ждем нажатия кнопки
  delay(100);
  while (digitalRead(BUTTON) == LOW);  // Ждем отпускания кнопки
  delay(100);
  lcd.clear();
}

// Создание новой игры
void createGame()
{
  // Создание деревьев
  for (int i = 0; i < 3; i++)
  {
    createTree(i);
  }

  // Создание птицы
  createBird();

  // Начало отсчета времени игры
  startTime = millis();

  // Обновление экрана ЖКИ
  updateScreen = true;
}

// Создание дерева с номером number
void createTree(int number)
{
  trees[number].treeVisible = false; // В начале дерево невидимое
  trees[number].treeX = 19; // Начальная координата дерева (правый край экрана)
  trees[number].treeSpeed = 300; // Время между перемещением дерева
  trees[number].treeTime = millis() + random(5000); // Время между перемещениями дерева
  trees[number].treeSize = random(2); // Случайный размер дерева
}

// Создание птицы
void createBird()
{
  birdVisible = false; // В начале птица невидимая
  birdX = 19; // Начальная координата птицы
  birdSpeed = 150; // Время между перемещением птицы
  birdTime = millis() + random(5000); // Время между перемещениями птицы
}

// Прыжок Джампера
void moveMan()
{
  if (digitalRead(BUTTON) == LOW) // Кнопка нажата?
  {
    if (manY == 2 && isJump == false) // Джампер еще не прыгает?
    {
      manY = 0; // Совершаем прыжок
      jumpTime = millis(); // Запоминаем время начала прыжка
      updateScreen = true; // Необходимо обновить экран
      isJump = true; // Джампер прыгает
      
      // Играем звук прыжка
      tone(BUZZER, 800);
      delay(25);
      noTone(BUZZER);
      digitalWrite(BUZZER, HIGH);
    }
  }

  if (millis() > jumpTime + jumpSpeed) // Пришло время опуститься вниз
  {
    if (digitalRead(BUTTON) == HIGH) // Кнопка отпущена (защита от читерства в виде удержания кнопки)?
    {
      delay(10);
      if (digitalRead(BUTTON) == HIGH) // Повторное считывание кнопки для подавления дребезга
      {
        isJump = false; // Прыжок закончен
      }
    }

    // Если Джампер находился в прыжке
    if (manY == 0)
    {
      manY = 2; // Джампер падает вниз
      updateScreen = true; // Необходимо перерисовать экран
    }
  }
}

// Перемещение деревьев
void moveTrees()
{
  for (int i = 0; i < 3; i++)
  {
    if (trees[i].treeVisible == false) // Дерево невидимо?
    {
      if (millis() > trees[i].treeTime) // Дереву пора появиться?
      {
        trees[i].treeVisible = true; // Дерево стало видимым
      }
    }
    else // Дерево уже видимо
    {
      if (millis() > trees[i].treeMoveTime + trees[i].treeSpeed) // Дереву пора переместится
      {
        updateScreen = true; // Необходимо обновить экран
        trees[i].treeMoveTime = millis(); // Запоминамем время последнего перемещения дерева
        trees[i].treeX--; // Изменение координаты дерева
        if (trees[i].treeX < 0) // Дерево дошло до левого края экрана?
        {  
          createTree(i); // Создание нового дерева
        }
      }
    }
  }
}

// Перемещение птицы
void moveBird()
{
  if (birdVisible == false) // Птица невидима?
  {
    if (millis() > birdTime)//Птице пора появиться?
    {
      birdVisible = true; // Птица стала видимой
    }
  }
  else // Дерево уже видимо
  {
    if (millis() > birdMoveTime + birdSpeed) // Птице пора переместится
    {
      updateScreen = true; // Необходимо обновить экран
      birdX--; // Изменение координаты птицы
      birdMoveTime = millis(); // Запоминамем время последнего перемещения птицы 
      if (birdX < 0) // Птица дошла до левого края экрана?
      {
        createBird();  // Создание новой птицы    
      }
    }
  }
}

// Перерисовка экрана игры
void drawGame()
{
  if (updateScreen == true) // Необходимо перерисовать экран?
  {
    lcd.clear(); // Очистка экрана
    drawMan(); // Рисование Джампера
    drawTrees(); // Рисование деревьев
    drawBird(); // Рисование птицы
  }
}

// Рисование Джампера
void drawMan()
{
  lcd.setCursor(manX, manY); // Перемещаем курсор в знакоместо, где находится голова
  lcd.print((char)HEAD); // Рисуем голову
  lcd.setCursor(manX, manY + 1); // Перемещаем курсор в знакоместо, где находится тело
  lcd.print((char)BODY); // Рисуем тело
}

// Рисование деревьев
void drawTrees()
{
  for (int i = 0; i < 3; i++) // Для всех деревьев
  {
    if (trees[i].treeVisible == true) // Дерево видимо?
    {
      if (trees[i].treeSize == true) // Дерево большое (состоит из двух символов)?
      {
        lcd.setCursor(trees[i].treeX, 2); // Перемещаем курсор в знакоместо, где находится листва дерева
        lcd.print((char)BIG_LEAVES); // Рисуем листву
        lcd.setCursor(trees[i].treeX, 3); // Перемещаем курсор в знакоместо, где находится ствол дерева
        lcd.print((char)BIG_WOOD); // Рисуем ствол
      }
      else // Дерево маленькое?
      {
        lcd.setCursor(trees[i].treeX, 3); // Перемещаем курсор в знакоместо, где находится дерево
        lcd.print((char)SMALL_LEAVES); // Рисуем дерево
      }
    }
  }
}

// Рисование птицы
void drawBird ()
{
  if (birdVisible == true) // Птица видима?
  {
    lcd.setCursor(birdX, 0);  // Перемещаем курсор в знакоместо, где находится птица
    lcd.print((char)BIRD); // Рисуем птицу
  }
}

// Проверка конца игры
bool checkGameOver()
{
  if (isJump == true) // Джампер в прыжке?
  {
    if (manX == birdX) // Джампер попал в птицу?
    {
      return true; // Конец игры наступил
    }
  }
  else // Джампер на земле?
  {
    for (int i = 0; i < 3; i++) // Проверяем все деревья
    {
      if (manX == trees[i].treeX) // Джампер попал в дерево?
      {
        return true; // Конец игры наступил
      }
    }
  }

  return false; // Игра продолжается
}

// Экран конца игры
void drawGameOver()
{
  // Вывод текста конца игры
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("GAME OVER!!!");

  // Вывод времени игры
  lcd.setCursor(3, 1);
  lcd.print("Time: ");
  lcd.print((millis() - startTime) / 1000);
  lcd.print(" sec.");

  lcd.setCursor(4, 3);
  lcd.print("PRESS START!");
  while (digitalRead(BUTTON) == HIGH); // Ждем нажатия кнопки
  delay(100);
  while (digitalRead(BUTTON) == LOW);  // Ждем отпускания кнопки
  delay(100);
  lcd.clear();
}






