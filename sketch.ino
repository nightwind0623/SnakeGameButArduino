#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h> //https://github.com/wonho-maker/Adafruit_SH1106

#define OLED_RESET -1
#define LEFT 0
#define RIGHT 1
#define UP 2
#define DOWN 3

const int CELL_SIZE = 2;
short xPin = 0;
short yPin = 1;
short zPin = 2;
byte width = 128;
byte height = 64;
byte row = height / CELL_SIZE;
byte column = width / CELL_SIZE;
Adafruit_SH1106 screen(OLED_RESET);

class Node{
  private:
    byte x;
    byte y;
    Node* next;
  public:
    byte getX(){return x;}
    byte getY(){return y;}
    Node* getNext(){return next;}
    void setNext(Node* node){next = node;}
    Node(byte x, byte y) : x(x), y(y), next(nullptr){}
    Node() : x(0), y(0), next(nullptr){}
    ~Node(){
      x = 0;
      y = 0;
    }
};

class Snake{
  private:
    short bodySize;
    char heading;
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
        byte x = currentNode->getX();
        byte y = currentNode->getY();
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
    char setHeading(char direction){
      if(direction < 4 && direction > -1){
        heading = direction;
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
    bool checkOverlapping(int x, int y, Snake* snake){
      Node* currentNode = snake->getHeadNode();
      for(int i = 0; i < snake->getBodySize(); i++){
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

Snake* snake;
Fruit* fruit;

void setup()   {    
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
  screen.clearDisplay();

  snake->changeDirection(xPin, yPin);
  switch(snake->getHeading()){
    case RIGHT:
      snake->addNode(new Node(snake->getHeadNode()->getX() + CELL_SIZE, snake->getHeadNode()->getY()));
      break;
    case LEFT:
      snake->addNode(new Node(snake->getHeadNode()->getX() - CELL_SIZE, snake->getHeadNode()->getY()));
      break;
    case UP:
      snake->addNode(new Node(snake->getHeadNode()->getX(), snake->getHeadNode()->getY() + CELL_SIZE));
      break;
    case DOWN:
      snake->addNode(new Node(snake->getHeadNode()->getX(), snake->getHeadNode()->getY() - CELL_SIZE));
      break;
  }
  if(snake->getHeadNode()->getX() == fruit->getX() && snake->getHeadNode()->getY() == fruit->getY()){
    fruit->setNewLocation(snake);
  }else{
    snake->removeNode();
  }

  snake->drawSnake(screen);
  fruit->drawFruit(screen);

  screen.display();
  delay(300);
}

