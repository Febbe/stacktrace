# fbbe::stacktrace

This library is a backport of the c++23 stacktrace library for c++17.
It is usefull, when you can't use c++23 or if your awailable compiler 
does not support it yet.

# Comaptibility

Beside the spaceship operator (three way comparison) the library is fully compatible to 
std::stacktrace from the c++23 <stacktrace> header. 

just replace std::stacktrace with fbbe::stacktrace.

As soon you switch to c++23 with an implemented stacktrace library, fbbe::stacktrace falls back to std::stacktrace.
This will make it easy, to replace it later.
