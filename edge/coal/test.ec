.float bar;

function sprint(s : string) = #1
function nprint(v : float ) = #2

function foo(a : float) =
{
    nprint(a);
}

function main() =
{
    sprint("Hello World!");

//    local entity y;
//    y.bar = 128;
//    nprint(y.bar);
  
    local float x; x = 1;
    
    do {
      foo(x);
      x = x + 1;
    }
    while (x <= 5);
}

