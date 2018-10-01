# cpp-lazyimports

Cpp-lazyimports is a lazy loader for shared libraries.

Cross platform, it works on Windows and Linux (untested on MacOS yet).

The source is header only.

Format is `<shared library name>!<symbol name>`

Example :

```C
auto sum = "shared.so!sum"_lazy;
std::cout << sum.call<int>(1337, 42) << std::endl;
```

(See docs/example.cpp for more usage)

Todos :
* add some comments
* add more Catch unit tests