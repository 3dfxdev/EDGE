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
}

function main() =
{
    sprint("------------");
    sprint("Hello World!");
    sprint("------------");
    
    vprint('1 2 3' + '4 5 6');

    var x;
    
    x = 1; do { foo(x); x = x + 1; } while (x <= 6);

    sprint("");

    x = 1; do { bar(x); x = x + 1; } while (x <= 6);
}

