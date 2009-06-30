function sprint(s : string) = #1
function nprint(v : float ) = #2
function vprint(v : vector) = #3

function foo(a) =
{
    nprint(a);
}

function main() =
{
    sprint("------------");
    sprint("Hello World!");
    sprint("------------");
    
    vprint('1 2 3' + '4 5 6');

    var x; x = 1;

    do {
      foo(x);
      x = x + 1;
    }
    while (x <= 5);
}

