# The program
In this program I implemented two ways to evaluate a logical expression (a string) in cpp.

# Build and run
```bash
cmake -B build
cmake --build build
./build/expression_evaluator
```

# The issues
The original code was wrong in my opinion, because evaluating the following expressions lead to a "Uhm, please retry" output
- "(v0 == 2 || (v1 > 10 && v2 > 3)) && v3 == -15.000000001 && v4"
- (1 == 2  or  (15.55 > 10  and   -10 > 3))  and  -15.000000001 == -15.000000001  and  true
- true


- "((v0 == 2 || (v1 > 10 && v2 > 3)) && v3 == -15.000000001 && v4) && (v5 == !v4)"

I'd like to implement an early fail exit in the manual mode, but current volume of obligations will postpone the challenge