# SIL

Sapo Input Language (SIL) is a language designed to provide an input model for Sapo.
It consists in a list of statements, each belonging to one of the following categories:

- [Header](#header)
- [Symbol definitions](#symdef)
- [Directions and template](#matdef)
- [SAPO Options](#footer)


## <a name="header">Header
It consists in

### problem definition (required)

```C++
problem: reachability | synthesis;
```

This statement determines if sapo will compute the reachable sets or perform parameter synthesis.


### number of iterations (required)

```C++
iterations: <num>;
```

Determines how many reachable steps are performed.

### maximum number of parameter splits (optional)

```C++
max_parameter_splits: <num>;
```

When parameter synthesis returns an empty set, the result may be due to over-approximation. 
Since the approximation also directly depends on the size of the parameter set, in order to decrease it, 
the initial parameter set can be split in subsets and the computation can be repeated for each them.
The `max_parameter_splits` option declares the maximum number of splits that Sapo can perform 
searching for a non-empty set satisfying the specification.

### presplits for parameters (optional)

```C++
presplit_parameters: ON | OFF;
```

### bundle magnitude (optional)

```C++
max_bundle_magnitude: <num>;
```



## <a name="symdef">Symbol definitions

Statements of this kind are used to define the symbols of the model, like variables and parameters.
They can appear in any order, as long as each symbol is defined before is used.

### Variables
Variables can be defined with their initial value bounds. Multiple variables can be defined in the same statement. In this case, all variables get the same bounds.

```C++
var v1 in [a,b];
var v2, v3 in [c,d];
```
Otherwise, just the name is provided. Multiple variables can be defined in the same command.

```C++
var v1, v2;
```
Variable names follow the C++ rule for identifiers.

### Parameters

They follow the syntax of variable definition.

```C++
param p1, p2 in [0.01, 0.02];
```
```C++
param p1;
```

### Constants
They define a map between identifiers and numerical values.
The usual rules hold for constant names, and the corresponding expression must be numeric only.

```C++
const c1 = 1.2;
const c2 = 34 * 0.78;
const c3 = c1 + 2*c2;
```

### Definitions
They are used to name expressions that can be used as sub-expressions. They can contain variables, parameters, constants and other definitions.

```C++
define ex = v + c*p/2;
```

### Dynamics

They represent the evolution of variable values. 
`sapo` supports both discrete-time dynamics and 
continous-time dynamics specified by means of an ODE.

The discrete-time dynamic specification has the following syntax
```C++
next(<variable name>) = <polynomial expression> ;
```
where `<polynomial expression>` is a polynomial expression such that:
- all symbols occurring in it must be already defined
- it must be linear with regard to parameters

The ODE specification use a similar syntax:
```C++
<variable name>' = <polynomial expression> ;
```
but it requires **its solution** to be linear with respect to the parameters. Non-linear solutions with respect to the parameters are not supported yet.

Each variable must have a corresponding dynamic defined.

### Specification
If problem is `synthesis`, a STL specification must be provided. If the problem is `reachability`, this statement is ignored.

+ An atomic formula is of the form
	
	```C++
	ex1 ~ ex2
	```
	where

	- `ex1` and `ex2` are expression involving numbers, variables and constants
	- `~` is one of `<`, `<=`, `>`, `>=` or `=` 

+ Negation is written
	
	```C++
	! f
	```

+ Conjunction is written
	
	```C++
	f1 && f2
	```

+ Disjunction is written
	
	```C++
	f1 || f2
	```

+ Until is written
	
	```C++
	f1 U[a,b] f2
	```
	
	This formula holds at time `t` if there exists a time point `t'` in `[t+a, t+b]` such that `f2` hlds at time `t'` and `f1` hold in all time points from `t` to `t'`.

+ Eventually is written
	
	```C++
	F[a,b] f
	```
	
	This means that there exists a point in `[t+a, t+b]` where `f` holds.

+ Globally is written
	
	```C++
	G[a,b] f
	```
	
	This means that at all time points in '[t+a, t+b]` the formula `f` holds

Finally, the specification is provided as

```C++
spec: STLformula;
```

### Assumption
An assumption is a way to restrict valid states to those satisfying a given formula.
That is, during reachability steps, the states violating the assumptions are discarded.

```C++
assume x + y >= 0;
```

This must be an atomic formula which expression is linear in the variables and contains no parameters.
Multiple assumptions are treated like conjunctions.

Assumptions are currently supported only for `reachability`, not for `synthesis`.

### Expressions
Expressions are used to define constants, definitions and dynamics.
Legal operations are:

+ sum: ```v + 5```.
+ difference: ```3 - c1```.
+ product: ```alpha * x```.
+ division: ```x / 3```. Notice that the divisor must be a number or a numeric expression.
+ unary minus: ```-c```.
+ exponentiation: ```x^2```. Notice that the exponent must be an integer, or a numeric expression with integer value.

We have that `+`, `-`, `*` and `/` are left associative, while `^` is right associative.
`^` has precedence w.r.t. unary minus, which has precedence w.r.t `*` and `/`, which have precedence w.r.t. `+` and `-`.
Thus, the expression

```
-x^2 + y^3 * 4
```
is evaluated as

```
(-(x^2)) + ((y^3) * 4)
```

## <a name="matdef">Directions and template
These statements provide the definition of the polytopes used to approximate reachable sets and parameters.

### Directions of initial set
A valid direction is any expression which is linear w.r.t. variables and does not contain parameters.

```C++
direction v1 + 2*v2 - v3/2 in [a, b];
```
It is possible to define named directions

```C++
direction sum: v1 + v2 + v3 in [0,10];
```

It is also possible to define fixed directions

```C++
direction x + y - z = 0;
// this is equivalent to 
// direction x + y - z in [0, 0];
```

To the directions defined in this way we must add the implicit directions defined while declaring variables with bound. For example, the definition
`var x in [a, b];`
is equivalent to 

```C++
var x;
.
.
.
direction default_x: x in [a, b];
```

Notice that the implicit direction defined during variable declaration is always named `default_<var_name>`.

#### The `around` shortcut

The reserved word `around` is a shortcut to define intervals. `around(<value>, <length ratio>)` produced the interval centered in `<value>` having length `<value>*<length ratio>`.

```C++
const c = 0.1;
const v = 12.2;

var x in around(v/2, 2*c);  // x in [5.49, 6.71]
```

#### Adaptive directions

*Adaptive directions* are variable directions which change during the computation according to the dynamic law. Their use does not guarantee the minimization of the flow-pipe volume, but they may reduce the approximation with respect to the non-adaptive directions (*static directions*) at the price of a reasonable computational overhead.

To set a direction to be adaptive, use the reserved word `adaptive` at the end of its definition being either a `direction` or `var` statement.

The following example defines a 2D model whose dynamic lets the initial set rotate around the axis origin. The initial set is defined by two directions, `d1` and `d2`, that, at the beginning of the computation, correspond to the axis `x` and `y` respectively. The direction `d2` is a static direction and does not change, while `d1` is adaptive and rotates as the considered set rotate around the axis origin. 

```C++
iterations: 130;

var x, y;

const delta = 0.01;

next(x) = x - y*delta;
next(y) = y + x*delta;

direction d1: x in [-0.06, 0.05] adaptive;
direction d2: y in [1.0, 1.1];
```

### Template for parallelotope bundle
In order to represent polytopes, Sapo uses parallelotope bundles that are intersection of parallelotopes. A bundle consists in a vector of directions and a set of templates. Each template corresponds to a non-singular parallelotope and it is a set indices for the direction vector such that the corresponding directions are a basis for the bundle space.

```C++
template = {
	{num_1, ..., num_n},
	{id_1, num_2, ..., id_n},
	.
	.
	.
	{num_1, ..., num_n}
}
```
Each row represent a parallelotope by referencing its directions.
This can be done giving the name, if provided in the definition, or by their number. In fact, each direction has an associated number depending on the order of definition (`0` the first, `1` the second and so on).

The user must pay attention in describing bounded parallelotopes.

If the template is not provided, then it is automatically generated.
If the template is partial, in that it does not contain all defined directions, a new template is computed that contains all parallelotopes explicitly defined.

### Directions for parameter set
The directions for parameter work in the same way as those for variables

```C++
parameter_direction p1 - p2 in [a, b];

parameter_direction d1: p1 + 4*p3 in [0,1];
```

Again, parameter directions can be implicitly defined in parameter declaration.
For example, `param p in [-1, 1];` is equivalent to

```C++
param p;
...
parameter_direction default_p: p in [-1, 1];
```

Polytopes are not supported for parameter sets, so there can be at most as many directions as the number of parameters (also counting the implicitly defined ones)


## <a name="footer">Options
These statements provide some options to tune the behavior of Sapo.
All of them are optional.

We can define
- bundle image computation approach (optional)
	
	```option transformation [AFO | OFO];```
	In general, `AFO` (*all-for-one*,) gives more accurate results but `OFO` is faster.
	The default value is ```AFO```.

- set an approximation technique for set join in `k`-induction invariant proof.

	```option k_induction_join [listing|packaging|merging];```

	The default value is `listing`.

- avoid Bernstein coefficients caching during computation. If this option is selected, Bernstein coefficients are numerically evaluated on-the-fly at 
each epoch instead of being symbolically computed for a generic parallelotope with specific generators and cached once and then recovered 
and instantiated when needed. The default behaviour is more convenient when the time overhead due to the symbolic computation is balanced by 
the savings due to using the same coefficients for many evolution epochs. 

	```option no_caching;``` 
- set all the bundle directions to be adaptive

	```option all_dirs_adaptive;```
- set the ODE intergrator (Euler method is used by default)
	
	```option integrator [euler|runge_kutta4];```
- set the ODE integration step

	```option integration_step <numeric value>;```

## Comments
SIL understands C/C++ like comments, both sigle and multi-line

```C++
...
// single line comment
...
/*
multi
line
comment
*/
...
``` 

# Examples

### SIR
Here is a SIL input file for a SIR epidemic model

```C++
problem: reachability;

iterations: 10;
max_parameter_splits: 0;

var s in [0.2, 0.3];
var i in [0.001, 0.1];
var r in [0.7, 0.8];

param beta in [0.055, 0.1];
param mu in [0.00001, 0.001];
param gamma in [0.0027, 0.0055];
param alpha in [0.05, 0.07];

next(s) = s - beta*s*i - mu*s + gamma*r;
next(i) = i + beta*s*i - alpha*i;
next(r) = r + mu*s - gamma*r + alpha*i;

spec: r > 0.4;

assume(s <= 1);
assume(i <= 1);
assume(r <= 1);

assume(s >= 0);
assume(i >= 0);
assume(r >= 0);


assume(s + i + r <= 1);
assume(s + i + r >= 0);

option transformation OFO;
```

### Van der Pol oscillator

```C++
problem: reachability;

iterations: 30;

var x in [0, 0.01];
var y in [1.99, 2];

next(x) = x + (y)*0.02;
next(y) = y + (0.5*(1-x^2)*y - x)*0.02;

direction diff: y - x in [-10, 10];
direction sum: x + y in [-10, 10];

template = {
	{default_x, default_y},
	{default_x, diff},
	{default_x, sum},
	{default_y, diff},
	{default_y, sum},
	{diff, sum}
}

### A simple ODE system

problem: reachability;

iterations: 65;

var x, y;

x' = -y;
y' = x;

direction x in [-0.5, 0.5];
direction y in [5.5, 6.5];

option no_caching; 
option all_dirs_adaptive;
option integration_step 0.1;
option integrator runge_kutta4;
```
