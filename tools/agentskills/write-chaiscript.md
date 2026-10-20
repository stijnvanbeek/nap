---
name: write-chaiscript
description: |
  Write or fix a ChaiScript (.chai) file, especially a nap engine GUI/component script class. Triggers on "write a chai script", "fix this .chai", "implement draw()/init() in the chai class", "ChaiScript grammar/syntax".
---

# Write ChaiScript

ChaiScript is a scripting language designed for embedding in C++. This skill covers the grammar of a `.chai` file and, where relevant, the contract imposed by the host (the nap engine's `nap::gui::ChaiScriptGui`). Use it when writing or fixing a `.chai` file.

## When to use
- Writing a new `.chai` script from scratch.
- Fixing a `.chai` file that fails to load or throws at runtime.
- Implementing a method (e.g. `draw()`) on an existing ChaiScript class.

## When NOT to use
- Editing C++ that *binds* things into ChaiScript (`chai.add(...)`, `ImGui_ChaiScript.h`, etc.). That's C++; only consult the bindings to learn which names/functions are callable from the script.

## Core grammar

### Variables
```
var i;              // uninitialized, any type on first assignment
auto j;             // same as var
var k = 5;          // integer literal
var f = 1.0;        // float literal (note the decimal point!)
var s = "hello";    // string
var v = [1, 2, 3];  // vector/array
var m = ["a": 1];   // map
var &ref = k;       // reference
```
Prefer explicit literals so the boxed type is obvious. An integer literal like `100` is an int; `100.0` is a float. Mixing them in arithmetic can matter when a bound function expects a specific numeric type.

### Globals
```
global g = 5;      // create/set a global (visible everywhere, incl. inside functions)
GLOBAL g2;         // uppercase also accepted
```
Globals persist across scopes and are visible from within class methods. In the nap engine, `scale` is provided as a global by the C++ side — do not declare it yourself, just read it.

### Functions / methods
Functions and methods are equivalent; a method is just syntactic sugar. The last statement in a body is the return value, or use `return`.
```
def add(x, y) { x + y; }            // implicit return
def sub(x, y) { return x - y; }     // explicit return
def typed(x, int y) { x + y; }      // optional param types
```

### Lambdas (anonymous functions)
A lambda is only parsed when it starts with the `fun` keyword — a bare `[](x){...}` is NOT a lambda (it parses as an inline container and then fails; see Pitfalls).
```
var f = fun (x) { print(x); };       // anonymous function value
fun [this](value) { this.m(value); } // bind list: captures are bound BY VALUE
```
A lambda has no ambient object: to call methods on the current class instance from inside one, bind `this` explicitly with `fun[this](...)`.

### Classes (block syntax) — the preferred form
```
class Rectangle {
	var width
	var height

	def Rectangle(w, h) {           // constructor: def <ClassName>(...)
		this.width = w;
		this.height = h;
	}

	def Rectangle() {                // overloaded no-arg ctor
		this.width = 0;
		this.height = 0;
	}

	def area() {                     // method
		return this.width * this.height;
	}
}

var r = Rectangle(3, 4);
print(r.area());                    // 12
```
Key points:
- Attributes are declared with `var` (also `attr` or `auto`; interchangeable). Declare each member once at the top of the block.
- A constructor is `def <ExactClassName>(params) {...}`. Match the class name exactly.
- Access members via `this.member`. Assignments to undeclared members fail unless the object is non-explicit (see Pitfalls).
- Instantiate with `ClassName(args)` — no `new`.

### Open syntax (outside the block)
Equivalent to adding members without re-opening the class, using the `Type::` prefix:
```
attr Circle::radius
def Circle::Circle(r) { this.radius = r; }
def Circle::circumference() { 2.0 * 3.14159 * this.radius; }
// extend an already-defined class later:
def Rectangle::perimeter() { 2 * (this.width + this.height); }
```

### Calling one method from another method
Inside a method, call sibling methods with `this.methodName(...)` to be safe. Bare `methodName(...)` may resolve to a global function instead of the current object's method depending on scope; `this.` removes the ambiguity.

## Nap engine GUI component contract
When the script backs a `nap::gui::ChaiScriptGui` (entry in objects.json with `"Type":"nap::gui::ChaiScriptGui"`), the C++ driver imposes a strict contract:
- The class MUST define both `init()` and `draw()`. Missing either breaks init/draw.
- The instance variable is created by C++ as `<stripped-mID>` (namespace removed from the object's `mID`), then `.<id>.init()` is called once and `.<id>.draw()` every frame. So your class name should match the `ClassName` field in objects.json.
- `scale` is set as a global by C++ before every `draw()` — read it, don't declare it.
- On a `draw()` failure, C++ logs the error and sets an error flag that stops further draws. A silent log means success.

Minimal working GUI class:
```
class MyGui {
	var size

	def MyGui() {
		this.size = 100.0;
	}

	def init() {}

	def draw() {
		var pos = ImVec2(0, 0);
		var corner = pos + this.size * scale;
		var col = ImGui_IM_COL32(255, 0, 0, 128);
		var dl = ImGui_GetWindowDrawList();
		dl.AddLine(pos, corner, col, 1.0);
		dl.AddRect(pos, corner, ImGui_IM_COL32(0, 255, 0, 255));
		ImGui_SetCursorPos(pos);
		ImGui_Text("MyGui");
	}
}
```

## Nap signals and slots
NAP signals (e.g. a `ParameterFloat`'s `valueChanged`) are connected to script callbacks through slot objects. For floats, `ChaiScriptCoreDefines.cpp` registers `FloatSlot` (wraps a script function taking a float into a `nap::Slot<float>`) and `FloatSignal` (its `connect` accepts a `FloatSlot`).

The pattern:
```
class MyComponent
{
	var valueSlot;

	def MyComponent()
	{
		this.valueSlot = FloatSlot(fun[this](value){ this.valueChanged(value); });
	}

	def init()
	{
		this.MyParameter.valueChanged.connect(this.valueSlot);
	}

	def valueChanged(value)
	{
		print(value);
	}
}
```
Rules that make this work:
- The lambda MUST be `fun`-syntax, and MUST use the `[this]` bind so the callback can call methods on the class instance.
- The slot lives in a declared member (`var valueSlot`) and is built once in the constructor — it must outlive the `connect` call, and keeping the reference allows disconnect/reconnect later.
- `connect` happens in `init()`, never in the constructor: `nap::ChaiClassComponent` injects linked Resources/Components as attributes on the instance (`this.MyParameter`) only *after* the default constructor has run. (`nap::ChaiScriptComponent` injects the same links as plain globals — no `this.` — but the slot logic is identical.)
- Errors thrown by the callback are caught inside the slot and logged as `Error in chai slot: ...`; they never propagate into the emitting signal.

Minimal GUI-less component class for `ChaiClassComponent` (JSON fields `"Script"`, `"ClassName"`, `"Resources"`, `"Components"`): define `init()` and `update(deltaTime)`; linked resources are reached as `this.<ResourceID>`.

## Pitfalls (learned the hard way)

1. **Invalid typed-local declaration → "Two expressions missing line separator".** Do NOT write `var ImVec2 p1 = ImVec2(0, 0);`. ChaiScript locals are `var name = expr;` with no type token between `var` and the name. Putting a type there makes the parser see two adjacent expressions and emit `Error: "Two expressions missing line separator" in '<file>' at (line, col)`. Correct: `var p1 = ImVec2(0, 0);`.

2. **Bare `[](){}` lambda → "Incomplete function call".** The bundled ChaiScript parser's `Lambda()` (chaiscript_parser.hpp) requires the `fun` keyword. Passing `FloatSlot([](v){...})` or `FloatSlot(def(v){...})` as an argument fails with `Error: "Incomplete function call" in '<file>' at (<line>, <col>)` pointing at the opening `(` — it looks like a dispatch error but is a parse error. Correct: `FloatSlot(fun [](v){...})`, and inside a class method use `fun[this](v){ this.m(v); }` so `this` is bound.

3. **Mixed-type operator precedence.** Expressions combining vectors and scalars (e.g. `pos + this.size * scale`) can be ambiguous if a bound operator isn't registered for the exact operand pair. If a draw/load error appears on such a line, factor it: compute the scalar first (`var s = this.size * scale;`), then combine (`var corner = pos + s;`). Keep ImVec2↔ImVec2 ops separate from ImVec2↔float ops.

4. **Undeclared member assignment.** Setting `this.foo` that was never declared as an attribute errors with "Unable to find appropriate '=' operator" on explicit objects. Always declare attributes up front. (A `method_missing(name, params)` handler can intercept unknown calls if needed.)

5. **Int vs float literals.** Pass `1.0`/`100.0` to functions expecting floats; a bare `1`/`100` is an int and may not convert where the binding demands a float.

6. **Verify against the live log, not assumptions.** For napstudio-driven scripts, run the app and watch its console/log after each edit. Load-time eval errors appear as `[warn] Failed to deserialize ... Error evaluating <file>: ...`; per-frame `draw()` failures are logged each frame. A clean log over several seconds of rendering is the real acceptance test.

## Style
- Tab-indent inside class blocks (matches existing nap `.chai` files).
- Semicolons after statements are recommended (they're separators, not optional punctuation to omit carelessly).
- One attribute declaration per line, grouped at the top of the class.
- Keep `draw()` self-contained where possible; avoid cross-method calls unless needed, to sidestep scope-resolution surprises.
