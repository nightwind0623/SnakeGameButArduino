#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h> //https://github.com/wonho-maker/Adafruit_SH1106

#define OLED_RESET -1

const byte CELL_SIZE = 2;
byte xPin = 0;
byte yPin = 1;
byte zPin = 2;
byte btnPin = 13;
byte width = 128;
byte height = 64;
byte row = height / CELL_SIZE;
byte column = width / CELL_SIZE;
Adafruit_SH1106 screen(OLED_RESET);

enum Direction {
  LEFT,
  RIGHT,
  UP,
  DOWN
};

class Node{
  private:
    int x;
    int y;
    Node* next;
  public:
    int getX(){return x;}
    int getY(){return y;}
    Node* getNext(){return next;}
    void setNext(Node* node){next = node;}
    Node(int x, int y) : x(x), y(y), next(nullptr){}
    Node() : x(0), y(0), next(nullptr){}
    ~Node(){
      x = 0;
      y = 0;
    }
};

class Snake{
  private:
    short bodySize;
    Direction heading;
    Node* headNode;
  public:
    void addNode(Node* newHead){
      newHead->setNext(headNode);
      headNode = newHead;
      bodySize++;
    }
    void removeNode(){
      if(!headNode || headNode->getNext() == nullptr){
        return;
      }
      Node* lastNode;
      Node* currentNode = headNode;
      while(currentNode->getNext() != nullptr){
        lastNode = currentNode;
        currentNode = currentNode->getNext();
      }
      lastNode->setNext(nullptr);
      delete currentNode;
      bodySize--;
    }
    void drawSnake(Adafruit_SH1106& screen){
      Node* currentNode = headNode;
      for(short i = 0; i < bodySize; i++){
        int x = currentNode->getX();
        int y = currentNode->getY();
        for(byte j = x; j < x + CELL_SIZE; j++){
          for(byte k = y; k < y + CELL_SIZE; k++){
            screen.drawPixel(j, k, WHITE);
          }
        }
        currentNode = currentNode->getNext();
      }
    }
    void changeDirection(byte xPin, byte yPin){
      short inputX = map(analogRead(xPin), 0, 1023, 0, 2);  
      short inputY = map(analogRead(yPin), 0, 1023, 0, 2);
      if(inputX == 2 && heading != LEFT){
        heading = RIGHT;
      }
      else if(inputX == 0 && heading != RIGHT){
        heading = LEFT;
      }
      else if(inputY == 2 && heading != DOWN){
        heading = UP;
      }
      else if(inputY == 0 && heading != UP){
        heading = DOWN;
      }
    }
    void setHeading(Direction newDirection){
      if(newDirection >= LEFT && newDirection <= DOWN){
        heading = newDirection;
      }
    }
    char getHeading(){return heading;}
    short getBodySize(){return bodySize;}
    Node* getHeadNode(){return headNode;}
    Snake(byte posX, byte posY){
      headNode = new Node(posX, posY);
      bodySize = 1;
      heading = RIGHT;
    }
    ~Snake(){
      Node* currentNode = headNode;
      while (currentNode != nullptr) {
        Node* nextNode = currentNode->getNext();
        delete currentNode;
        currentNode = nextNode;
      }
      headNode = nullptr;
      bodySize = 0;
      heading = 0;
    }
};

class Fruit{
  private:
    byte x;
    byte y;
    bool checkOverlapping(byte x, byte y, Snake* snake){
      Node* currentNode = snake->getHeadNode();
      for(short i = 0; i < snake->getBodySize(); i++){
        if(x == currentNode->getX() || y == currentNode->getY() ){
            return true;
        }
        currentNode = currentNode->getNext();
      }
      return false;
    }
  public:
    void setNewLocation(Snake* snake){
      byte newX;
      byte newY;
      bool overlapping;
      do {
          newX = random(0, column)*CELL_SIZE;
          newY = random(0, row)*CELL_SIZE;
          overlapping = checkOverlapping(newX, newY, snake);
      } while (overlapping);
      x = newX;
      y = newY;
    }
    void drawFruit(Adafruit_SH1106& screen){
      for(byte j = x; j < x + CELL_SIZE; j++){
        for(byte k = y; k < y + CELL_SIZE; k++){
          screen.drawPixel(j, k, WHITE);
        }
      }
    }
    byte getX(){return x;}
    byte getY(){return y;}
    Fruit(){
      x = 0;
      y = 0;
    }
};

void restart(Snake*& oldSnake, Fruit* oldFruit){
  delete oldSnake;
  oldSnake = new Snake(50, 16);
  oldSnake->addNode(new Node(50, 18));
  oldSnake->addNode(new Node(50, 20));
  oldFruit->setNewLocation(oldSnake);  
}

bool isGameover(Snake* snake){
  Node* currentNode = snake->getHeadNode()->getNext();
  Node* headNode = snake->getHeadNode();
  for(short i = 1; i < snake->getBodySize(); i++){
    if(headNode->getX() == currentNode->getX() && headNode->getY() == currentNode->getY()){
      return true;
    }
    currentNode = currentNode->getNext();
  }
  if(headNode->getX() < 0 || headNode->getY() < 0 || headNode->getX() > width || headNode->getY() > height){
    return true;
  }
  return false;
}

Snake* snake;
Fruit* fruit;


void setup()   {    
  pinMode(btnPin, INPUT);

  snake = new Snake(50, 16);
  snake->addNode(new Node(50, 18));
  snake->addNode(new Node(50, 20));     
  
  fruit = new Fruit();
  fruit->setNewLocation(snake);

  Serial.begin(9600);
  screen.begin(SH1106_SWITCHCAPVCC, 0x3C);
  screen.display();
  delay(1000);
  screen.clearDisplay();

}

void loop() {
  if(!isGameover(snake)){
    screen.clearDisplay();

    snake->changeDirection(xPin, yPin);
    int newX = snake->getHeadNode()->getX();
    int newY = snake->getHeadNode()->getY();
    switch(snake->getHeading()){
      case RIGHT:
        newX += CELL_SIZE;
        break;
      case LEFT:
        newX -= CELL_SIZE;
        break;
      case UP:
        newY += CELL_SIZE;
        break;
      case DOWN:
        newY -= CELL_SIZE;
        break;
    }
    snake->addNode(new Node(newX, newY));

    if(snake->getHeadNode()->getX() == fruit->getX() && snake->getHeadNode()->getY() == fruit->getY()){
      fruit->setNewLocation(snake);
    }else{
      snake->removeNode();
    }

    snake->drawSnake(screen);
    fruit->drawFruit(screen);
    screen.display();
    delay(200);
  }else{
    screen.clearDisplay();
    screen.setTextSize(1);
    screen.setTextColor(WHITE);
    screen.setCursor(0, 0);
    screen.print("press botton to \nrestart.\n\nyour score:");
    screen.print(snake->getBodySize());
    screen.display();
    if(digitalRead(btnPin) == 1){
      restart(snake, fruit);
    }
  }
}

