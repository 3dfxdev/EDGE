.float foo;

void(string s) sprint = #1;
void(float v) nprint = #2;

void() main =
{
    sprint("Hello World!");

//    local entity y;
//    y.foo = 128;
//    nprint(y.foo);
  
    local float x; x = 1;
    
    do {
      nprint(x);
      x = x + 1;
    }
    while (x <= 5);
};

