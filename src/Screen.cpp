// 0327 deprecated :: 
#include "LiquidCrystal_I2C.h"

class Button;
class Screen;

class Button
{
  private :
    int posX, posY;
    Button* prevAction;
    Button* nextAction;
    Screen* setAction;
    Screen* backAction;
    String* text;

  public :
    Button(int posX, int posY, String* text)
    {
      this->posX = posX;
      this->posY = posY;
      this->text = text;
    }

    void setPosX(int posX)
      {this->posX = posX;}
    int getPosX()
      {return this->posX;}
    
    void setPosY(int posY)
      {this->posY = posY;}
    int getPosY()
      {return this->posY;}

    void setPrevAction(Button* prevAction)
      {this->prevAction = prevAction;}
    Button* getPrevAction()
      {return this->prevAction;}

    void setNextAction(Button* nextAction)
      {this->nextAction = nextAction;}
    Button* getNextAction()
      {return this->nextAction;}

    void setSetAction(Screen* setAction)
      {this->setAction = setAction;}
    Screen* getSetAction()
      {return this->setAction;}

    void setBackAction(Screen* backAction)
      {this->backAction = backAction;}
    Screen* getBackAction()
      {return this->backAction;}

    void setText(String* text)
      {this->text = text;}
    String* getText()
      {return this->text;}
};

class Screen
{
  private : 
    Button* activeButton;
    Button* buttons;
    int buttonCount;
    String* description;
    LiquidCrystal_I2C* lcd;
  public :
    Screen(LiquidCrystal_I2C* lcd, Button* buttons, int buttonCount, String* description)
    {
      this->lcd = lcd;
      this->buttons = buttons;
      this->buttonCount = buttonCount;
      this->description = description;
      this->activeButton = buttons;
    }

    enum action
    {
      PREV,
      NEXT,
      SET,
      BACK
    };
    void screenAction(int action)
    {
      if(action == this->PREV)
      {
        int cursorX = this->activeButton->getPrevAction()->getPosX();
        int cursorY = this->activeButton->getPrevAction()->getPosY();
        this->lcd->setCursor(cursorX, cursorY);
      }
      else if(action == this->NEXT)
      {
        int cursorX = this->activeButton->getNextAction()->getPosX();
        int cursorY = this->activeButton->getNextAction()->getPosY();
        this->lcd->setCursor(cursorX, cursorY);
      }
      else if(action == this->SET)
        {this->activeButton->getSetAction()->show();}
      else
        {this->activeButton->getBackAction()->show();}
    }

    void show()
    {
      //TODO : New Screen Update
      this->lcd->clear();
      lcd->setCursor(0, 0);
      lcd->print(*this->description);
      for(int i = 0 ; i < this->buttonCount ; i++)
      {
        int x = (buttons + i)->getPosX();
        int y = (buttons + i)->getPosY();
        lcd->setCursor(x, y);
        lcd->print(*(buttons + i)->getText());
      }
      int x = this->activeButton->getPosX();
      int y = this->activeButton->getPosY();
      lcd->setCursor(x, y);
      lcd->blink();
    }
};



