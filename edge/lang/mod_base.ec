
// BASICS

void(string s) sprint = #1;
void(float v) nprint = #2;

void(float v) sys_assert = { };

// MATH

float(float v) math_floor = { return v; };
float() math_random = { return 1; };
float(float a, float b) math_mod = { return b - 1; };

// STRING

string(string fmt, float val) string_format = { return fmt; };
string(string a, string b) string_concat = { return a; };
string(string a, string b, string c) string_concat3 = { return a; };

