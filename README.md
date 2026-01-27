# molencalc

A little [PicoCalc](https://www.clockworkpi.com/product-page/picocalc) calculator meant to be useful for engineering.
It has niceties like exponent suffixes (p,n,u,m,k,M,G) and enough built-in functions for everyday use.

For ease of development, there is an SDL-based version at (https://github.com/TheRealMolen/mcalc)


## internals

The libcalc folder is the heart of the calculator, featuring a recursive-descent parser (which means we can give nice error messages)


### Thanks

Big thanks to Blair Leduc for the wonderful [PicoCalc Text Starter](https://github.com/BlairLeduc/picocalc-text-starter), which certainly made
this project a lot easier to get cracking with!
