
// BASICS

function sprint(s : string) = native
function nprint(n : float)  = native
function vprint(v : vector) = native

function sys_assert(n : float) = { }

// MATH

function math_floor(n : float) : float = { return n; }
function math_random() : float = { return 1; }

// STRING

function string_format  (fmt : string, n : float) : string = { return fmt; }
function string_concat  (a:string, b:string) : string = { return a; }
function string_concat3 (a:string, b:string, c:string) : string = { return a; }

