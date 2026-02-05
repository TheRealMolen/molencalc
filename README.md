# molencalc

<p align="center">
    <img width=50% src=https://github.com/TheRealMolen/molencalc/blob/main/assets/molencalc.jpg?raw=true>
</p>


A little [PicoCalc](https://www.clockworkpi.com/product-page/picocalc) calculator meant to be useful for engineering.
It has niceties like exponent suffixes (p,n,u,m,k,M,G) and enough built-in functions for everyday use.

For ease of development, there is an SDL-based version at (https://github.com/TheRealMolen/mcalc)


## installing

The releases have .uf2 files that work for original Pico boards (`molencalc-pico.uf2`) as well
as ones that work on Pico2 boards (`molencalc-pico2.uf2`). As long as your picocalc has one of
those boards, you can just load the image on there however you normally load .uf2 files.

#### `bye`

Inspired by the picocalc-text-starter, entering `bye` at the molencalc prompt will reboot the
pico in BOOTSEL mode, ready to drop another `.uf2` on. Handy for upgrades, as well as when you
fancy having a play with some other fun picocalc image!


## usage

### expressions

For "normal" expression use, just type your expression at the `>` prompt and press Enter.
The computed result will be printed out on the next line, for instance:
```
> 2 ^ pi
  = 8.8249778
```

Molencalc is pretty relaxed about whitespace, and respects operator precedence:
```
> 1+2 * 3!
  = 13
```

Because I originally wanted to use molencalc for electronics calculations, it has scale
suffixes built-in - so `2.2k` means `2,200` and `4.7m` means `0.0047`.
The suffixes it knows are: `G M k m u n p` - from 10<sup>9</sup> down to 10<sup>-12</sup>.

Molencalc features some of the core functions you might expect - `sin`, `cos`, `atan`, `ln`
and so on. It also knows about `pi` and `e`. Feel free to raise issues on github if there's
a constant/function you're missing, and hopefully it'll be in the next release.
```
> sin(ln(e))
  = 0.84147098
```
**Note that functions always need `( )` parentheses around their arguments**

### variables

For convenience, you can store values in named variables with `let`:
```
> let x=3/2
> let y=pi
> sin(x*y)
  = -1.000

> let y=-pi
> sin(x*y)
  = 1.000
```

### user functions

You can also define your own named functions using a "mapping" notation:
```
> myfunc: x -> sin(x^2)
> myfunc(2)
  = -0.75680250
```

**Note: at the moment, all user functions map exactly one number to another - you can't use 
  multiple function parameters**


### graphing

Once you've defined a function, you can graph it with the `g` command: 
`g <func_name> [axis [, axis]]`:

![g f](https://github.com/TheRealMolen/molencalc/blob/main/assets/graph1.png?raw=true)

You can optionally specify axis ranges for the x and/or y axes with the format
`lo < axis < hi`. For instance:

![g f -20<x<20](https://github.com/TheRealMolen/molencalc/blob/main/assets/graph2.png?raw=true)
![g f -20<x<20 -0.5<y<1.2](https://github.com/TheRealMolen/molencalc/blob/main/assets/graph3.png?raw=true)


## internals

The libcalc folder is the heart of the calculator, featuring a recursive-descent parser (which means we can give nice error messages).

Most of the development for this lib happens outside the PicoCalc for faster iteration, over at https://github.com/TheRealMolen/mcalc. That's a little SDL-based harness with enough of the same
platform APIs as this picocalc version to make porting trivial.


### Thanks

Big thanks to Blair Leduc for the wonderful [PicoCalc Text Starter](https://github.com/BlairLeduc/picocalc-text-starter), which certainly made
this project a lot easier to get cracking with!
