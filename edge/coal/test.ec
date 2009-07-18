// 
// Test script for Coal
//

function sprint(s : string) = native
function nprint(v : float ) = native
function vprint(v : vector) = native

function foo(a) =
{
    nprint(a)
}

function bar(b) =
{
    var y; y = 1 | 4 | 16
    b = b & y

    nprint(b)
    if (b <= 4)
        b = 1
    else
        sprint("JACKPOT!")
}

function factorial(n) : float =
{
    if (n <= 1)
        return n

    return factorial(n - 1) * n
}

function main() =
{
    sprint("------------")
    sprint("Hello World!")
    sprint("------------")
    
    nprint(factorial(5))
    nprint(factorial(4) + factorial(3) + factorial(2))

    vprint('1 2 3' + '4 5 6')
    vprint('1 2 3' / 4  )

    sprint("")

    var x

    x = 1; repeat { foo(x); x = x + 1; } until (x > 6)

    sprint("")

    x = 1; while (x <= 6) { bar(x); x = x + 1 }
}

