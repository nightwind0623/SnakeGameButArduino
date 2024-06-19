#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h> //取自https://github.com/wonho-maker/Adafruit_SH1106

#define OLED_RESET -1
//定義腳位
const byte X_PIN = 0;
const byte Y_PIN = 1;
const byte Z_PIN = 2;
const byte BUTTON_PIN = 13;
//定義遊戲畫面大小
const byte CELL_SIZE = 2;
const byte WIDTH = 128;
const byte HEIGHT = 64;
const byte ROW = HEIGHT / CELL_SIZE;
const byte COLUMN = WIDTH / CELL_SIZE;

Adafruit_SH1106 screen(OLED_RESET);

/*************************************
自訂義的類鏈結串列結構節點
我將蛇的身體分成很連在一起的幾個格子
每個格子的位置都儲存在這種結構
當蛇的長度變長可以比較容易的修改
**************************************/
class Node{
  private:
    int x;  //節點x, y座標
    int y;
    Node* next; //指向下個節點的指標
  public:
    /***************
    getter & setter
    ****************/
    int getX(){return x;}
    int getY(){return y;}
    Node* getNext(){return next;}
    void setNext(Node* node){next = node;}
    /***************
    constructor
    ****************/
    Node(int x, int y) : x(x), y(y), next(nullptr){}
    Node() : x(0), y(0), next(nullptr){}
    /***************
    destructor
    ****************/
    ~Node(){
      x = 0;
      y = 0;
    }
};

class Snake{
  public:
    //各種方向的列舉
    enum Direction {
      LEFT,
      RIGHT,
      UP,
      DOWN
    };
  private:
    short bodySize; //蛇的長度
    Direction heading;  //蛇的移動方向
    Node* headNode; //指向蛇的頭節點的指標
  public:
    /***********************************
    接下來這兩個是實現蛇移動的重要方法

    addNode()的示意:
      原本的鏈結串列:
        頭節點:節點1
        節點1->節點2->節點3
      後來的鏈結串列:
        頭節點:新節點
        新節點->節點1->節點2->節點3
    ***********************************/
    void addNode(Node* newHead){  //傳入一個要作為新的頭部的節點指標
      newHead->setNext(headNode); //將舊的頭變成新的頭的下一個節點 
      headNode = newHead; //把新的頭更新到變數裡
      bodySize++;
    }
    /***********************************
    removeNode()這個有點複雜
    他會一直往串鏈的結尾去找
    直到找到最後一個節點
    接著把上一個節點指向他的指標砍掉
    最後把它刪掉

    總之就是將最尾巴的節點刪掉
    示意:
      原本的鏈結串列:
        節點1->節點2->節點3->節點4
      下一步:
        節點1->節點2->節點3  節點4
      最後:
        節點1->節點2->節點3
      
    ***********************************/
    void removeNode(){
      if(!headNode || headNode->getNext() == nullptr){  
        return; //如果蛇沒有或只有一個節點就直接回傳
      }
      Node* lastNode;
      Node* currentNode = headNode;
      while(currentNode->getNext() != nullptr){
        lastNode = currentNode;
        currentNode = currentNode->getNext();
      }
      lastNode->setNext(nullptr);
      delete currentNode; //釋放儲存節點的記憶體 等同於呼叫了節點的destructor
      bodySize--;
    }

    /**********************
    這個call了就會把蛇畫出來
    ***********************/
    void drawSnake(Adafruit_SH1106& screen){
      Node* currentNode = headNode;
      for(short i = 0; i < bodySize; i++){  //這層loop會遍歷所有節點
        int x = currentNode->getX();
        int y = currentNode->getY();
        for(byte j = x; j < x + CELL_SIZE; j++){  //這兩層就是看節點的座標然後一格多大就畫多大
          for(byte k = y; k < y + CELL_SIZE; k++){
            screen.drawPixel(j, k, WHITE);
          }
        }
        currentNode = currentNode->getNext();
      }
    }
    /*******************************
    根據搖桿輸入更改蛇的移動方向
    ********************************/
    void changeDirection(byte X_PIN, byte Y_PIN){
      short inputX = map(analogRead(X_PIN), 0, 1023, 0, 2);  
      short inputY = map(analogRead(Y_PIN), 0, 1023, 0, 2);
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

    /***************
    getter & setter
    ****************/
    void setHeading(Direction newDirection){
      if(newDirection >= LEFT && newDirection <= DOWN){
        heading = newDirection;
      }
    }
    char getHeading(){return heading;}
    short getBodySize(){return bodySize;}
    Node* getHeadNode(){return headNode;}
    /***************
    constructor
    ****************/
    Snake(int posX, int posY){
      headNode = new Node(posX, posY);
      bodySize = 1;
      heading = RIGHT;
    }
    /***************
    destructor
    ****************/
    ~Snake(){
      Node* currentNode = headNode;
      while (currentNode != nullptr) {  //銷毀所有節點
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
    byte x; //食物的座標
    byte y;
    /******************************************
    遍歷蛇的所有節點檢查食物有沒有和蛇重疊
    *******************************************/
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
    /*****************
    刷新食物的位置
    ******************/
    void setNewLocation(Snake* snake){
      byte newX;
      byte newY;
      bool overlapping;
      do {
          newX = random(0, COLUMN)*CELL_SIZE; //隨機生成新座標
          newY = random(0, ROW)*CELL_SIZE;
          overlapping = checkOverlapping(newX, newY, snake);  //檢查新位置有沒有跟蛇重疊
      } while (overlapping);
      x = newX;
      y = newY;
    }
    /*********************
    call了就在螢幕上畫出食物
    **********************/
    void drawFruit(Adafruit_SH1106& screen){
      for(byte j = x; j < x + CELL_SIZE; j++){
        for(byte k = y; k < y + CELL_SIZE; k++){
          screen.drawPixel(j, k, WHITE);
        }
      }
    }
    /***************
    getter & setter
    ****************/
    byte getX(){return x;}
    byte getY(){return y;}
    /***************
    constructor
    ****************/
    Fruit(){
      x = 0;
      y = 0;
    }
};

/********
重製遊戲
*********/
void restart(Snake*& oldSnake, Fruit* oldFruit){
  delete oldSnake;  //銷毀蛇
  oldSnake = new Snake(50, 16); //重新生成蛇
  oldSnake->addNode(new Node(50, 18));  
  oldSnake->addNode(new Node(50, 20));
  oldFruit->setNewLocation(oldSnake); //刷新食物位置  
}
/***************
檢查死了沒
****************/
bool isGameover(Snake* snake){
  Node* currentNode = snake->getHeadNode()->getNext();
  Node* headNode = snake->getHeadNode();
  for(short i = 1; i < snake->getBodySize(); i++){  //遍歷節點檢查有沒有撞到自己
    if(headNode->getX() == currentNode->getX() && headNode->getY() == currentNode->getY()){
      return true;
    }
    currentNode = currentNode->getNext();
  }
  //檢查有沒有撞牆
  if(headNode->getX() < 0 || headNode->getY() < 0 || headNode->getX() > WIDTH || headNode->getY() > HEIGHT){
    return true;
  }
  return false;
}

/*************
宣告食物和蛇
**************/
Snake* snake;
Fruit* fruit;


void setup()   {    
  pinMode(BUTTON_PIN, INPUT);

  snake = new Snake(50, 16);  //實例化蛇的物件
  snake->addNode(new Node(50, 18));
  snake->addNode(new Node(50, 20));     
  
  fruit = new Fruit();  //實例化食物的物件
  fruit->setNewLocation(snake); //刷新位置

  Serial.begin(9600); //以下是設定螢幕
  screen.begin(SH1106_SWITCHCAPVCC, 0x3C);
  screen.display();
  delay(1000);
  screen.clearDisplay();

}

void loop() {
  if(!isGameover(snake)){ //蛇如果還活著
    screen.clearDisplay();  //清空螢幕畫面

    snake->changeDirection(X_PIN, Y_PIN); //根據輸入改變方向
    /*************************************
    拿取蛇頭的座標
    接著依據朝向來修改新的頭部節點的座標
    *************************************/
    int newX = snake->getHeadNode()->getX();
    int newY = snake->getHeadNode()->getY();
    switch(snake->getHeading()){
      case Snake::RIGHT:
        newX += CELL_SIZE;
        break;
      case Snake::LEFT:
        newX -= CELL_SIZE;
        break;
      case Snake::UP:
        newY += CELL_SIZE;
        break;
      case Snake::DOWN:
        newY -= CELL_SIZE;
        break;
    }
    snake->addNode(new Node(newX, newY));

    //如果蛇吃到食物了就不會變短
    if(snake->getHeadNode()->getX() == fruit->getX() && snake->getHeadNode()->getY() == fruit->getY()){
      fruit->setNewLocation(snake);
    }else{
      snake->removeNode();
    }
    //畫出新的蛇和食物
    snake->drawSnake(screen);
    fruit->drawFruit(screen);
    //展示螢幕畫面
    screen.display();
    //延遲
    delay(200);
  }else{  //很不幸的蛇掛了
    screen.clearDisplay();  //清除螢幕畫面
    //設置文字:尺寸 顏色 字體 顯示位置
    screen.setTextSize(1);  
    screen.setTextColor(WHITE);
    screen.setCursor(0, 0);
    //打印出分數
    screen.print("press botton to \nrestart.\n\nyour score:");
    screen.print(snake->getBodySize());
    //展示螢幕畫面
    screen.display();
    if(digitalRead(BUTTON_PIN) == 1){ //按下按鈕重新開始
      restart(snake, fruit);
    }
  }
}

