// 
// Test script for Coal
//

function sprint(s : string) = #1
function nprint(v : float ) = #2
function vprint(v : vector) = #3

function foo(a) =
{
    nprint(a);
}

function bar(b) =
{
    var y; y = 1 | 4 | 16;
    b = b & y;

    nprint(b);
    if (b <= 4)
        b = 1;
    else
        sprint("JACKPOT!");
}

function main() =
{
    sprint("------------");
    sprint("Hello World!");
    sprint("------------");
    
    vprint('1 2 3' + '4 5 6');
    vprint('1 2 3' / 4  );

    var x;
    var y;

    x = 1; do { foo(x); x = x + 1; } while (x <= 6);

    sprint("");

    x = 1; while (x <= 6) { bar(x); x = x + 1; }
}

